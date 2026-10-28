/**
  ******************************************************************************
  * @file           : board_init.h
  * @brief          : F407 ZGT6 clock bring-up API.
  *
  * Board_ClockInit_F407 configures the 96 MHz PLL from the 8 MHz HSE, flash
  * latency + ART, and the 48 MHz USB clock. The app reaches it through the
  * board-agnostic Board_ClockInit() seam in board_clock.h, and the shared F4
  * bootloader through BOARD_BOOT_CLOCK_INIT() below.
  ******************************************************************************
  */

#ifndef BOARD_INIT_H_
#define BOARD_INIT_H_

void Board_ClockInit_F407(void);

/* Clock-init entry point the shared F4 bootloader (bootloader/f411/Src/main.c)
 * calls before its boot decision. */
#define BOARD_BOOT_CLOCK_INIT()		Board_ClockInit_F407()

#endif /* BOARD_INIT_H_ */
