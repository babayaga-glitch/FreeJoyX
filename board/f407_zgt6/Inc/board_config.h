/**
  ******************************************************************************
  * @file           : board_config.h
  * @brief          : F407 ZGT6 flash storage layout (compile spike).
  *
  * Copied from board/f411_blackpill. STM32F407ZGT6 has 12 sectors:
  * S0..S3 = 16 KB each, S4 = 64 KB, S5..S11 = 128 KB each (1 MB total).
  * The F411 layout below uses only S0..S7, whose addresses and sizes are
  * the same on the F407 (RM0090 Table 5). Layout
  * (F411_PORT_PLAN.md "Pin map / wire-format compatibility"):
  *   S0           bootloader            16 KB
  *   S1..S3       reserved              48 KB
  *   S4           config storage        64 KB  @ 0x08010000
  *   S5..S7       application          384 KB  @ 0x08020000
  *
  * The constants below are sized to look like 1 "page" of 64 KB so the
  * existing application/Src/config.c erase loop fires once with the
  * right address. Phase 3 will provide the matching ConfigFlash_ErasePage
  * implementation that does an F4 sector erase under the hood.
  ******************************************************************************
  */

#ifndef BOARD_CONFIG_H_
#define BOARD_CONFIG_H_

/* BOARD_ID tag for this board. Value 3 is reserved for the F407 ZGT6 but
 * is NOT yet in application/Inc/common_defines.h: that header must match
 * the configurator's copy (header-sync.yml), and the configurator does not
 * know this board yet. Move it there as BOARD_ID_F407_ZGT6 together with
 * the configurator change. */
#define BOARD_ID						3

#define FLASH_PAGE_SIZE					(64 * 1024)							/* sector 4 size */
#define CONFIG_PAGE_COUNT				1									/* one sector for config */
#define CONFIG_ADDR						0x08010000UL						/* sector 4 start */
#define FLASH_PAGE_END_ADDR				(CONFIG_ADDR + FLASH_PAGE_SIZE)
#define MAX_PAGE						1									/* legacy alias; F411 thinks in sectors */

#endif /* BOARD_CONFIG_H_ */
