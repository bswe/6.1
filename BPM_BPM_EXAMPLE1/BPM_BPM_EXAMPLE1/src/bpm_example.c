/**
 * \file
 *
 * \brief BPM example.
 *
 * Copyright (c) 2012-2013 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

 /**
 * \mainpage BPM example
 * \section intro Introduction
 * This is the documentation for the data structures, functions, variables,
 * defines, enums, and typedefs for the Backup Power Manager (BPM) driver.
 * It also comes bundled with an application-example of usage.
 *
 * This example demonstrates how to use the BPM driver. It requires a
 * board monitor firmware version V1.3 or greater.
 *
 * <b>Operating mode: </b>The user can select the low power mode and power
 * scaling from the terminal. The example uses the terminal and the board
 * monitor to provide infomation about the current power save mode and actual
 * power consumption.
 *
 * \section files Main Files
 * - bpm.c: BPM driver;
 * - bpm.h: BPM driver header file;
 * - bpm_example.c: BPM example application.
 *
 * \section compilinfo Compilation Information
 * This software is written for GNU GCC and IAR Embedded Workbench
 * for Atmel. Other compilers may or may not work.
 *
 * \section deviceinfo Device Information
 * All SAM devices with an BPM and a USART module can be used.
 *
 * \section configinfo Configuration Information
 * This example has been tested with the following configuration:
 * - SAM4L_EK evaluation kit;
 * - CPU clock: 12 MHz;
 * - USART2 (on SAM4L_EK) abstracted with a USB CDC connection to a PC;
 * - USART0 (on SAM4L_EK) which connected to board monitor;
 * - PC terminal settings:
 *   - 115200 bps,
 *   - 8 data bits,
 *   - no parity bit,
 *   - 1 stop bit,
 *   - no flow control.
 *
 * \section contactinfo Contact Information
 * For further information, visit
 * <A href="http://www.atmel.com">Atmel</A>.\n
 * Support and FAQ: http://support.atmel.no/
 */

#include <asf.h>
#include "board_monitor.h"

/* Flag to use board monitor */
static bool ps_status = BPM_PS_1;

/* Current sleep mode */
static uint32_t current_sleep_mode = SLEEP_MODE_NA;

/* Power scaling value -> board monitor status */
power_scaling_t ps_statuses[] = {
	POWER_SCALING_PS0, POWER_SCALING_PS1
};

/**
 * EIC interrupt handler for push button interrupt
 */
static void eic_5_callback(void)
{
	sysclk_enable_peripheral_clock(EIC);
	if(eic_line_interrupt_is_pending(EIC, GPIO_PUSH_BUTTON_EIC_LINE)) {
		ast_write_counter_value(AST,0); /* Reset AST counter */
		eic_line_clear_interrupt(EIC, GPIO_PUSH_BUTTON_EIC_LINE);
	}
	sysclk_disable_peripheral_clock(EIC);
}

/**
 * AST interrupt handler
 */
static void ast_per_callback(void)
{
	ast_clear_interrupt_flag(AST, AST_INTERRUPT_PER);
}

/**
 * Initialize AST to generate 1Hz counter
 */
static void config_ast(void)
{
	struct ast_config ast_conf;

	/* Enable osc32 oscillator*/
	if (!osc_is_ready(OSC_ID_OSC32)) {
		osc_enable(OSC_ID_OSC32);
		osc_wait_ready(OSC_ID_OSC32);
	}

	/* Enable the AST. */
	ast_enable(AST);

	ast_conf.mode = AST_COUNTER_MODE;
	ast_conf.osc_type = AST_OSC_1KHZ;
	ast_conf.psel = AST_PSEL_32KHZ_1HZ;
	ast_conf.counter = 0;
	ast_set_config(AST, &ast_conf);

	/* Set periodic 0 to interrupt after 8 second in counter mode. */
	ast_clear_interrupt_flag(AST, AST_INTERRUPT_PER);
	ast_write_periodic0_value(AST, AST_PSEL_32KHZ_1HZ - 2);
	/* Set callback for periodic0. */
	ast_set_callback(AST, AST_INTERRUPT_PER, ast_per_callback,
		AST_PER_IRQn, 1);
}

static void config_buttons(void)
{
	/* Initialize EIC for button wakeup. */
	struct eic_line_config eic_line_conf = {
		EIC_MODE_EDGE_TRIGGERED,
		EIC_EDGE_FALLING_EDGE,
		EIC_LEVEL_LOW_LEVEL,
		EIC_FILTER_DISABLED,
		EIC_ASYNCH_MODE
	};
	eic_enable(EIC);
	eic_line_set_config(EIC, GPIO_PUSH_BUTTON_EIC_LINE, 
		&eic_line_conf);
	eic_line_set_callback(EIC, GPIO_PUSH_BUTTON_EIC_LINE, eic_5_callback,
		EIC_5_IRQn, 1);
	eic_line_enable(EIC, GPIO_PUSH_BUTTON_EIC_LINE);
}

/* configurations for backup mode wakeup */
static void config_backup_wakeup(void)
{
	/* EIC and AST can wakeup the device */
	bpm_enable_wakeup_source(BPM,
			(1 << BPM_BKUPWEN_EIC) | (1 << BPM_BKUPWEN_AST));
	/* EIC can wake the device from backup mode */
	bpm_enable_backup_pin(BPM, 1 << GPIO_PUSH_BUTTON_EIC_LINE);
	/**
	 * Retain I/O lines after wakeup from backup.
	 * Disable to undo the previous retention state then enable.
	 */
	bpm_disable_io_retention(BPM);
	bpm_enable_io_retention(BPM);
	/* Enable fast wakeup */
	bpm_enable_fast_wakeup(BPM);
}

/**
 *  Configure serial console.
 */
static void configure_console(void)
{
	const usart_serial_options_t uart_serial_options = {
		.baudrate = CONF_UART_BAUDRATE,
#ifdef CONF_UART_CHAR_LENGTH
		.charlength = CONF_UART_CHAR_LENGTH,
#endif
		.paritytype = CONF_UART_PARITY,
#ifdef CONF_UART_STOP_BITS
		.stopbits = CONF_UART_STOP_BITS,
#endif
	};

	/* Configure console. */
	stdio_serial_init(CONF_UART, &uart_serial_options);
}

/**
 * \brief Display the user menu on the terminal.
 */
static void display_menu(void)
{
	printf("Menu :\r\n"
			"  -- Select the action:\r\n"
			"  s: Switch Power scale. \r\n"
			"  0: Enter Sleep mode 0. \r\n"
			"  1: Enter Sleep mode 1. \r\n"
			"  2: Enter Sleep mode 2. \r\n"
			"  3: Enter Sleep mode 3. \r\n"
			"  4: Enter Wait mode. \r\n"
			"  5: Enter Retention mode. \r\n"
			"  6: Enter Backup mode. \r\n"
			"  h: Display menu \r\n"
			"  --Push button can also be used to exit low power mode--\r\n"
			"\r\n");
	printf("-- IMPORTANT: This example requires a board "
			"monitor firmware version V1.3 or greater.\r\n\r\n");
}

/**
 * \brief Application entry point for BPM example.
 *
 * \return Unused (ANSI-C compatibility).
 */
int main(void)
{
	uint8_t key;

	/* Initialize the SAM system */
	sysclk_init();
	board_init();

	/* Initialize the console uart */
	configure_console();

	/* Output example information */
	printf("\r\n");
	printf("-- BPM Example --\r\n");
	printf("-- %s\r\n", BOARD_NAME);
	printf("-- Compiled: %s %s --\r\n", __DATE__, __TIME__);

	/* Initialize the board monitor  */
	bm_init();

	/* Configurate the AST to wake up */
	config_ast();

	/* Configurate the EIC */
	config_buttons();

	/* Configurate the backup wakeup source */
	config_backup_wakeup();

	/* Display menu */
	display_menu();

	while(1) {
		scanf("%c", (char *)&key);

		switch (key) {
		case 'h':
			display_menu();
			break;

		case 's':
			if (ps_status == BPM_PS_1) {
				printf("\r\n--Switch Power scale to 1.8V\r\n");
				ps_status = BPM_PS_0;
			} else {
				printf("\r\n--Switch Power scale to 1.2V\r\n");
				ps_status = BPM_PS_1;
			}
			/* Power scaling setup */
			bpm_configure_power_scaling(BPM, ps_status,
					BPM_PSCM_CPU_NOT_HALT);
			while((bpm_get_status(BPM) & BPM_SR_PSOK) == 0);
			bm_send_mcu_status(ps_statuses[ps_status], current_sleep_mode,
					12000000, CPU_SRC_RC4M);
			break;

		case '0':
			current_sleep_mode = SLEEP_MODE_0;
			bm_send_mcu_status(ps_statuses[ps_status], current_sleep_mode,
					12000000, CPU_SRC_RC4M);
			printf("\r\n--Enter Sleep mode 0.\r\n");
			ast_enable_wakeup(AST, AST_WAKEUP_PER);
			/* Wait for the printf operation to finish before
			setting the device in a power save mode. */
			delay_ms(30);
			bpm_sleep(BPM, BPM_SM_SLEEP_0);
			ast_disable_wakeup(AST, AST_WAKEUP_PER);
			printf("\r\n--Exit Sleep mode 0.\r\n");
			break;

		case '1':
			current_sleep_mode = SLEEP_MODE_1;
			bm_send_mcu_status(ps_statuses[ps_status], current_sleep_mode,
					12000000, CPU_SRC_RC4M);
			printf("\r\n--Enter Sleep mode 1.\r\n");
			ast_enable_wakeup(AST, AST_WAKEUP_PER);
			/* Wait for the printf operation to finish before
			setting the device in a power save mode. */
			delay_ms(30);
			bpm_sleep(BPM, BPM_SM_SLEEP_1);
			ast_disable_wakeup(AST, AST_WAKEUP_PER);
			printf("\r\n--Exit Sleep mode 1.\r\n");
			break;

		case '2':
			current_sleep_mode = SLEEP_MODE_2;
			bm_send_mcu_status(ps_statuses[ps_status], current_sleep_mode,
					12000000, CPU_SRC_RC4M);
			printf("\r\n--Enter Sleep mode 2.\r\n");
			ast_enable_wakeup(AST, AST_WAKEUP_PER);
			/* Wait for the printf operation to finish before
			setting the device in a power save mode. */
			delay_ms(30);
			bpm_sleep(BPM, BPM_SM_SLEEP_2);
			ast_disable_wakeup(AST, AST_WAKEUP_PER);
			printf("\r\n--Exit Sleep mode 2.\r\n");
			break;

		case '3':
			current_sleep_mode = SLEEP_MODE_3;
			bm_send_mcu_status(ps_statuses[ps_status], current_sleep_mode,
					12000000, CPU_SRC_RC4M);
			printf("\r\n--Enter Sleep mode 3.\r\n");
			ast_enable_wakeup(AST, AST_WAKEUP_PER);
			/* Wait for the printf operation to finish before
			setting the device in a power save mode. */
			delay_ms(30);
			bpm_sleep(BPM, BPM_SM_SLEEP_3);
			ast_disable_wakeup(AST, AST_WAKEUP_PER);
			printf("\r\n--Exit Sleep mode 3.\r\n");
			break;

		case '4':
			current_sleep_mode = SLEEP_MODE_WAIT;
			bm_send_mcu_status(ps_statuses[ps_status], current_sleep_mode,
					12000000, CPU_SRC_RC4M);
			printf("\r\n--Enter Wait mode.\r\n");
			ast_enable_wakeup(AST, AST_WAKEUP_PER);
			/* Wait for the printf operation to finish before
			setting the device in a power save mode. */
			delay_ms(30);
			bpm_sleep(BPM, BPM_SM_WAIT);
			ast_disable_wakeup(AST, AST_WAKEUP_PER);
			printf("\r\n--Exit Wait mode.\r\n");
			break;

		case '5':
			current_sleep_mode = SLEEP_MODE_RETENTION;
			bm_send_mcu_status(ps_statuses[ps_status], current_sleep_mode,
					12000000, CPU_SRC_RC4M);
			printf("\r\n--Enter Retention mode.\r\n");
			ast_enable_wakeup(AST, AST_WAKEUP_PER);
			/* Wait for the printf operation to finish before
			setting the device in a power save mode. */
			delay_ms(30);
			bpm_sleep(BPM, BPM_SM_RET);
			ast_disable_wakeup(AST, AST_WAKEUP_PER);
			printf("\r\n--Exit Retention mode.\r\n");
			break;

		case '6':
			current_sleep_mode = SLEEP_MODE_BACKUP;
			bm_send_mcu_status(ps_statuses[ps_status], current_sleep_mode,
					12000000, CPU_SRC_RC4M);
			printf("\r\n--Enter Backup mode.\r\n");
			ast_enable_wakeup(AST, AST_WAKEUP_PER);
			/* Wait for the printf operation to finish before
			setting the device in a power save mode. */
			delay_ms(30);
			bpm_sleep(BPM, BPM_SM_BACKUP);
			ast_disable_wakeup(AST, AST_WAKEUP_PER);
			printf("\r\n--Exit Backup mode.\r\n");
			break;

		default:
			break;
		}
	}
}
