/**
  ******************************************************************************
  * @file           : board_init.h
  * @brief          : F407 ZGT6 clock bring-up API.
  *
  * Board_ClockInit_F407 configures the 96 MHz PLL from the 8 MHz HSE, flash
  * latency + ART, and the 48 MHz USB clock. The app reaches it through the
  * board-agnostic Board_ClockInit() seam in board_clock.h. (The F407 has no
  * bootloader yet; the F411 bootloader calls Board_ClockInit_F411 by name.)
  ******************************************************************************
  */

#ifndef BOARD_INIT_H_
#define BOARD_INIT_H_

void Board_ClockInit_F407(void);

#endif /* BOARD_INIT_H_ */
