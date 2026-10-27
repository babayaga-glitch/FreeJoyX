/**
  ******************************************************************************
  * @file           : board_init.c
  * @brief          : F407 ZGT6 clock + bringup helpers.
  *
  * Clock tree for FreeJoyX on the FK407M2-ZGT6 V1.1 (limits from DS8626
  * Table 36 / section 6.3.1 and RM0090 section 6):
  *   HSE     8 MHz crystal (schematic OS1, no bypass; HSE range 4..26 MHz)
  *   PLL_M   4       (VCO input = 8/4 = 2 MHz, in [0.95..2.10] MHz)
  *   PLL_N   192     (VCO output = 2 x 192 = 384 MHz, in [100..432] MHz)
  *   PLL_P   4       (SYSCLK = 384/4 = 96 MHz, max 168)
  *   PLL_Q   8       (USB OTG FS clock = 384/8 = 48 MHz, exact)
  *   AHB     /1      (HCLK = 96 MHz)
  *   APB1    /4      (PCLK1 = 24 MHz, max 42; APB1 timer clock = 48 MHz)
  *   APB2    /2      (PCLK2 = 48 MHz, max 84; APB2 timer clock = 96 MHz)
  *
  * SYSCLK/HCLK stay at the F411's 96 MHz so SystemCoreClock, SysTick, the
  * DWT delay and the USB turnaround time match the F411 build. The F407's
  * lower APB limits force different bus prescalers, so every constant
  * derived from PCLK1, PCLK2 or the APB1 timer clock differs from F411.
  *
  * VOS scale 1 is the F407 reset default; it is set explicitly anyway.
  * Flash wait states = 3 for 90 < HCLK <= 120 MHz at 2.7..3.6 V
  * (RM0090 Table 11; the board runs at 3.3 V).
  ******************************************************************************
  */

#include "stm32f4xx.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_utils.h"

#include "board_init.h"
#include "board_clock.h"

/*============================================================================
 *        Bring-up diagnostic: bounded clock waits + PC13 fault blink
 *
 * Hardware-debugging aid for first power-on, not a production feature.
 * Without it a dead crystal or an unlocked PLL hangs the board silently
 * in Board_ClockInit_F407 before anything else (USB, LED, SysTick) runs.
 *
 * Time base: both waits happen while SYSCLK is still the 16 MHz HSI
 * (reset default; the switch to PLL comes after them), so SysTick is used
 * in polled mode with a 1 ms reload derived from HSI_VALUE. No interrupt
 * is enabled; SysTick_Init() in periphery.c reprograms SysTick later.
 *
 * PC13 is NOT initialised before this point (Board_ClockInit is the first
 * call in main()), so the fault path configures it itself. PC13 sits on the
 * backup-domain power switch: sink only, <= 3 mA, <= 2 MHz (DS8626 pin
 * table note 3). The FK407M2 LED is 3.3 V -> 1.5 k -> LED -> PC13 (~1 mA
 * sink), so the pin is driven open-drain at low speed: low = LED on,
 * released = LED off. It never sources current.
 *==========================================================================*/

/* HSE: DS8626 tSU(HSE) is 2 ms typical; 100 ms matches HSE_STARTUP_TIMEOUT
 * in stm32f4xx_hal_conf.h and leaves ample margin for a slow crystal. */
#define CLOCK_DIAG_HSE_TIMEOUT_MS		100U
/* PLL: DS8626 Table 36 tLOCK is 300 us max; 10 ms is > 30x margin. */
#define CLOCK_DIAG_PLL_TIMEOUT_MS		10U

/* Blink codes: N short flashes, then a long gap, repeated forever. */
#define CLOCK_DIAG_BLINKS_HSE			2U
#define CLOCK_DIAG_BLINKS_PLL			3U
#define CLOCK_DIAG_ON_MS				200U
#define CLOCK_DIAG_OFF_MS				300U
#define CLOCK_DIAG_GAP_MS				1500U

/* Last clock fault for SWD inspection: 0 = none (or not reached), otherwise
 * the blink count above (2 = HSE, 3 = PLL). Read the symbol
 * board_clock_fault with the ST-Link while the LED is blinking. */
volatile uint32_t board_clock_fault = 0;

static void ClockDiag_TickStart(void)
{
	SysTick->CTRL = 0;
	SysTick->LOAD = (HSI_VALUE / 1000U) - 1U;
	SysTick->VAL  = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

static void ClockDiag_TickStop(void)
{
	SysTick->CTRL = 0;
}

/* Count elapsed 1 ms SysTick periods. COUNTFLAG clears on read. */
static void ClockDiag_DelayMs(uint32_t ms)
{
	while (ms > 0U) {
		if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) {
			ms--;
		}
	}
}

/* Poll is_ready() for up to timeout_ms. Returns 1 if ready, 0 on timeout. */
static uint32_t ClockDiag_WaitReady(uint32_t (*is_ready)(void), uint32_t timeout_ms)
{
	uint32_t ready;

	ClockDiag_TickStart();
	while (!(ready = is_ready()) && timeout_ms > 0U) {
		if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) {
			timeout_ms--;
		}
	}
	ClockDiag_TickStop();
	return ready ? 1U : 0U;
}

static uint32_t ClockDiag_HseReady(void) { return LL_RCC_HSE_IsReady(); }
static uint32_t ClockDiag_PllReady(void) { return LL_RCC_PLL_IsReady(); }

/* Never returns. RCC is left as it is so the failing state can be read
 * over SWD (RCC_CR HSEON/HSERDY, PLLON/PLLRDY); PA13/PA14 stay in SWD mode. */
static void ClockDiag_Fault(uint32_t blinks) __attribute__((noreturn));
static void ClockDiag_Fault(uint32_t blinks)
{
	board_clock_fault = blinks;

	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
	LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_13);			/* released = off */
	LL_GPIO_SetPinOutputType(GPIOC, LL_GPIO_PIN_13, LL_GPIO_OUTPUT_OPENDRAIN);
	LL_GPIO_SetPinSpeed(GPIOC, LL_GPIO_PIN_13, LL_GPIO_SPEED_FREQ_LOW);
	LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_13, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_13, LL_GPIO_MODE_OUTPUT);

	ClockDiag_TickStart();
	for (;;) {
		for (uint32_t i = 0; i < blinks; i++) {
			LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_13);	/* on */
			ClockDiag_DelayMs(CLOCK_DIAG_ON_MS);
			LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_13);		/* off */
			ClockDiag_DelayMs(CLOCK_DIAG_OFF_MS);
		}
		ClockDiag_DelayMs(CLOCK_DIAG_GAP_MS);
	}
}

void Board_ClockInit_F407(void)
{
	/* Power scaling first -- has to be at scale 1 to allow 96 MHz HCLK. */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
	LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);

	/* Flash latency for 96 MHz @ VOS scale 1: 3 wait states. */
	LL_FLASH_SetLatency(LL_FLASH_LATENCY_3);
	while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_3) { }

	/* ART accelerator: enable the instruction cache, data cache and prefetch
	 * so the 3 flash wait states are hidden. The F103 StdPeriph SetSysClock
	 * enables the F1 prefetch buffer automatically; the F4 CMSIS leaves this
	 * to user code, so without it the core stalls ~3 cycles per uncached
	 * fetch and runs at a fraction of its real throughput. Enable BEFORE the
	 * SYSCLK switch to PLL so the faster clock lands with caches already hot.
	 * ES0182 2.2.1: prefetch is unsupported on revision A silicon only. */
	LL_FLASH_EnableInstCache();
	LL_FLASH_EnableDataCache();
	LL_FLASH_EnablePrefetch();

	/* HSE on the FK407M2-ZGT6 is an 8 MHz crystal (no bypass). */
	LL_RCC_HSE_Enable();
	if (!ClockDiag_WaitReady(ClockDiag_HseReady, CLOCK_DIAG_HSE_TIMEOUT_MS)) {
		ClockDiag_Fault(CLOCK_DIAG_BLINKS_HSE);
	}

	/* PLL: HSE 8 -> M=4 -> 2 MHz -> N=192 -> 384 MHz VCO -> P=4 -> 96 MHz SYSCLK. */
	LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE,
	                            LL_RCC_PLLM_DIV_4,
	                            192,
	                            LL_RCC_PLLP_DIV_4);
	/* PLL_Q = 8 -> 48 MHz for USB OTG FS. M and N are shared with the SYS
	 * domain and must match the call above. */
	LL_RCC_PLL_ConfigDomain_48M(LL_RCC_PLLSOURCE_HSE,
	                            LL_RCC_PLLM_DIV_4,
	                            192,
	                            LL_RCC_PLLQ_DIV_8);

	LL_RCC_PLL_Enable();
	if (!ClockDiag_WaitReady(ClockDiag_PllReady, CLOCK_DIAG_PLL_TIMEOUT_MS)) {
		ClockDiag_Fault(CLOCK_DIAG_BLINKS_PLL);
	}

	/* Bus prescalers BEFORE switching SYSCLK so the buses don't briefly
	 * overshoot their 42/84 MHz limits at the transition. */
	LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
	LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
	LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);

	LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
	while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) { }

	LL_SetSystemCoreClock(96000000);
}

/* Board-agnostic early init seam (board_clock.h). The application calls this
 * as the first line of main() so the F407 app is self-sufficient regardless
 * of how it was launched -- it no longer relies on the bootloader having
 * configured the clock/latency/grouping (those are core state that survives
 * the jump but are NOT set on a cold SWD boot, and SystemCoreClock is the
 * app's own variable that the bootloader's copy never propagates into). */
void Board_ClockInit(void)
{
	Board_ClockInit_F407();

	/* NVIC priority grouping: the HAL pattern sets this in HAL_Init() (which
	 * the app does not call) to group 4 = all 4 implemented bits used for
	 * preemption, 0 for sub-priority. The app's NVIC_SetPriority(x, 2/3)
	 * calls assume that split; set it explicitly so the grouping is correct
	 * even on a cold boot rather than inherited from the bootloader. */
	NVIC_SetPriorityGrouping(3U);
}
