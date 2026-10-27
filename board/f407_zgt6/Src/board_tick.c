/**
  ******************************************************************************
  * @file           : board_tick.c
  * @brief          : F407 ZGT6 main-tick implementation on TIM2 (LL).
  *
  * F407 clock plan (board_init.c::Board_ClockInit_F407):
  *   SYSCLK    96 MHz   (PLL: HSE 8 / M=4 -> 2 -> N=192 -> 384 / P=4)
  *   AHB       96 MHz
  *   APB1      24 MHz   (TIM2/3/4/5/12/13/14 timers fed by APB1*2 = 48 MHz)
  *   APB2      48 MHz
  *
  * TIM2 lives on APB1 -> 48 MHz timer clock (96 MHz on the F411, whose APB1
  * prescaler is /2 rather than /4). Same Board_TickInit /
  * Board_TickStop / Board_TickISR contract as F103, just LL flavoured
  * because the F411 driver layer is LL (locked decision in
  * F411_PORT_PLAN.md, 2026-04-27). The 200 kHz intermediate counter is
  * preserved for parity with the F103 implementation in
  * board/f103_bluepill/Src/board_tick.c, so behaviour is byte-equivalent
  * once application/Src/main.c is wired up in Phase 5.
  *
  * Pre-hardware build only -- no BlackPill in hand to verify the actual
  * tick frequency on a scope. Stays compile-clean alongside the existing
  * F103 build.
  ******************************************************************************
  */

#include "stm32f4xx.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_tim.h"
#include "board_tick.h"

void Board_TickInit(uint32_t freq_hz)
{
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

	LL_TIM_InitTypeDef tim_init;
	LL_TIM_StructInit(&tim_init);

	/* TIM2 input clock = 48 MHz (24 MHz PCLK1 x2, APB1 prescaler /4).
	 * Match F103 convention: prescale to 200 kHz counter, then ARR
	 * divides that down to the requested freq_hz. The 200 kHz
	 * intermediate keeps the same usable freq_hz range as F103
	 * (1 Hz to 100 kHz) and avoids per-board calculations leaking up
	 * into the application layer. */
	/* Divide the 48 MHz timer clock by 240 to reach the 200 kHz counter the
	 * ARR formula below assumes. NB: divide by 200000, NOT 100000 -- 48 MHz
	 * is the ALREADY-DOUBLED APB1 timer clock. F103's StdPeriph uses the
	 * un-doubled PCLK1 (36 MHz) / 100000 and relies on the APB1 timer x2 to
	 * reach 200 kHz; copying that /100000 here gave only 100 kHz, so TIM2 ran
	 * at freq_hz/2 (1 kHz, not 2 kHz) and every GetMillis()-based timing on
	 * F411 (debounce, exchange period, gesture windows) ran at 2x duration. */
	tim_init.Prescaler   = (48000000U / 200000U) - 1U;   /* 48 MHz / 240 = 200 kHz */
	tim_init.Autoreload  = (200000U / freq_hz) - 1U;
	tim_init.CounterMode = LL_TIM_COUNTERMODE_UP;
	tim_init.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	tim_init.RepetitionCounter = 0;

	LL_TIM_Init(TIM2, &tim_init);
	LL_TIM_EnableARRPreload(TIM2);

	LL_TIM_EnableIT_UPDATE(TIM2);
	NVIC_SetPriority(TIM2_IRQn, 3);
	NVIC_EnableIRQ(TIM2_IRQn);

	LL_TIM_EnableCounter(TIM2);
}

void Board_TickStop(void)
{
	NVIC_DisableIRQ(TIM2_IRQn);
}

void TIM2_IRQHandler(void)
{
	if (LL_TIM_IsActiveFlag_UPDATE(TIM2))
	{
		LL_TIM_ClearFlag_UPDATE(TIM2);
		Board_TickISR();
	}
}

/* Weak default so the F411 image links pre-Phase 5b: nothing calls
 * Board_TickInit yet (main_f411.c is a standalone blinky), but the
 * TIM2_IRQHandler above references Board_TickISR and the linker resolves
 * it eagerly. The real Board_TickISR ships with the application layer in
 * application/Src/stm32f10x_it.c (which the F411 build will pull in
 * during Phase 5b once the F1-only headers are gated out) and overrides
 * this stub. */
__attribute__((weak)) void Board_TickISR(void)
{
}
