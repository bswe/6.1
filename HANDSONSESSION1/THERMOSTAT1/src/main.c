/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
#include <asf.h>
#include <string.h>

#include "avr2025_mac.h"
#include "pal.h"
#include "temp_sensor.h"
#include "data_protocol.h"

#define PROTOCOL_ADDRESS 0x25

volatile bool radio_ready = false;

wpan_addr_spec_t dst_addr = {
	.AddrMode = WPAN_ADDRMODE_SHORT,
	.PANId = DESTINATION_PAN_ID,
	.Addr.short_address = DESTINATION_SHORT_ADDR,
};

enum protocol_channels {
	PROTOCOL_LIGHT,
};

enum app_state {
	APP_STATE_RADIO_TX,
	APP_STATE_DISPLAY_RESULT,
};
volatile uint16_t app_state_flags = 0;

#define APP_ADC_SAMPLES 1
uint16_t g_adc_sample_data[APP_ADC_SAMPLES];
struct adc_dev_inst g_adc_inst;

static void ast_callback(void);


static bool is_app_state_set(
enum app_state state)
{
	bool retval;
	cpu_irq_disable();
	if (app_state_flags & (1 << state)) {
		retval = true;
		} else {
		retval = false;
	}
	cpu_irq_enable();
	return retval;
}
static void set_app_state(
enum app_state state)
{
	cpu_irq_disable();
	/* Set corresponding flag */
	app_state_flags |= (1 << state);
	cpu_irq_enable();
}
static void clear_app_state(
enum app_state state)
{
	cpu_irq_disable();
	/* Clear corresponding flag */
	app_state_flags &= ~(1 << state);
	cpu_irq_enable();
}

static void ast_setup(void)
{
	ast_enable(AST);
	struct ast_config ast_conf;
	ast_conf.mode = AST_COUNTER_MODE;
	ast_conf.osc_type = AST_OSC_32KHZ;
	/* Prescaler that gives an output of 1kHz */
	ast_conf.psel = 4;
	ast_conf.counter = 0;
	/* Enable osc32 oscillator*/
	if (!osc_is_ready(OSC_ID_OSC32)) {
		osc_enable(OSC_ID_OSC32);
		osc_wait_ready(OSC_ID_OSC32);
	}
	/* Initialize the AST */
	if (!ast_set_config(AST, &ast_conf)) {
		ioport_set_pin_level(LED0_GPIO, LED0_ACTIVE_LEVEL);
		while(true) {
		}
	}
}

static void ast_callback_setup(void)
{
	/* Callback to send data over wireless every second */
	ast_write_alarm0_value(AST, 1024);
	ast_enable_counter_clear_on_alarm(AST, 0);
	ast_write_counter_value(AST, 0);
	ast_set_callback(AST, AST_INTERRUPT_ALARM, ast_callback,
	AST_ALARM_IRQn, 0);
}

static void ast_callback(void)
{
	ast_clear_interrupt_flag(AST, AST_INTERRUPT_ALARM);
	ioport_toggle_pin_level(LED0_GPIO);
	adc_start_software_conversion(&g_adc_inst);
}

static void adcife_read_conv_result(void)
{
	// Check the ADC conversion status
	if ((adc_get_status(&g_adc_inst) & ADCIFE_SR_SEOC) == ADCIFE_SR_SEOC){
		g_adc_sample_data[0] = adc_get_last_conv_value(&g_adc_inst);
		adc_clear_status(&g_adc_inst, ADCIFE_SCR_SEOC);
		set_app_state(APP_STATE_DISPLAY_RESULT);
	}
}

static void adc_setup(void)
{
	struct adc_config adc_cfg = {
		/* System clock division factor is 16 */
		.prescal = ADC_PRESCAL_DIV16,
		/* The APB clock is used */
		.clksel = ADC_CLKSEL_APBCLK,
		/* Max speed is 150K */
		.speed = ADC_SPEED_150K,
		/* ADC Reference voltage is 0.625*VCC */
		.refsel = ADC_REFSEL_1,
		/* Enables the Startup time */
		.start_up = CONFIG_ADC_STARTUP
	};
	struct adc_seq_config adc_seq_cfg = {
		/* Select Vref for shift cycle */
		.zoomrange = ADC_ZOOMRANGE_0,
		/* Pad Ground */
		.muxneg = ADC_MUXNEG_1,
		/* Scaled Vcc, Vcc/10 */
		.muxpos = ADC_MUXPOS_6,
		/* Enables the internal voltage sources */
		.internal = ADC_INTERNAL_2,
		/* Disables the ADC gain error reduction */
		.gcomp = ADC_GCOMP_DIS,
		/* Disables the HWLA mode */
		.hwla = ADC_HWLA_DIS,
		/* 12-bits resolution */
		.res = ADC_RES_12_BIT,
		/* Enables the single-ended mode */
		.bipolar = ADC_BIPOLAR_SINGLEENDED
	};
	struct adc_ch_config adc_ch_cfg = {
		.seq_cfg = &adc_seq_cfg,
		/* Internal Timer Max Counter */
		.internal_timer_max_count = 60,
		/* Window monitor mode is off */
		.window_mode = 0,
		.low_threshold = 0,
		.high_threshold = 0,
	};
	adc_init(&g_adc_inst, ADCIFE, &adc_cfg);
	adc_enable(&g_adc_inst);
	adc_ch_set_config(&g_adc_inst, &adc_ch_cfg);
	adc_set_callback(&g_adc_inst, ADC_SEQ_SEOC, adcife_read_conv_result,
	ADCIFE_IRQn, 1);
}

static void alert(void)
{
	cpu_irq_disable();
	ioport_set_pin_level(LED0_GPIO, LED0_ACTIVE_LEVEL);
	while (true) {
		/* Do nothing */
	}
}

static void send_data(uint8_t *data, uint8_t size)
{
	static uint8_t msduHandle = 0;
	wpan_mcps_data_req (WPAN_ADDRMODE_SHORT, &dst_addr, size, 
	                    data, msduHandle++, WPAN_TXOPT_ACK);
}

int main (void)
{
	irq_initialize_vectors();
	board_init();				// Initialize all board settings (I/O, etc.)
	sysclk_init();				// Initialize clock system
	ast_setup();				// Initialize AST module
	ast_callback_setup();
	
	c42364a_init();
	LED_On(LCD_BL);
	adc_setup();
	
	//uint8_t const scrolling_str[] = "HELLO WORLD!";
	//c42364a_text_scrolling_start(scrolling_str,
	//strlen((char const *) scrolling_str));
	// Insert application code here, after the board has been initialized.
	
	ioport_set_pin_dir(AT86RFX_RST_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(AT86RFX_RST_PIN, IOPORT_PIN_LEVEL_HIGH);
	ioport_set_pin_dir(AT86RFX_SLP_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(AT86RFX_SLP_PIN, IOPORT_PIN_LEVEL_HIGH);	

	delay_init();
	sw_timer_init();
	
	if (wpan_init() != MAC_SUCCESS) {
		alert();
	}
	
	cpu_irq_enable();
	wpan_mlme_reset_req(true);
	while (!radio_ready) {
		wpan_task();
	}

	uint8_t string_buf[8];
	snprintf(string_buf, 8, "No 0x%2X", PROTOCOL_ADDRESS);
	c42364a_write_alphanum_packet(string_buf);	
	
	protocol_tx_init(send_data, PROTOCOL_ADDRESS);
	
	while(1)
	{
		wpan_task();
		
		if (is_app_state_set(APP_STATE_DISPLAY_RESULT)) {
			clear_app_state(APP_STATE_DISPLAY_RESULT);
			c42364a_show_numeric_dec(g_adc_sample_data[0]);
			
			protocol_set_channel_data(PROTOCOL_LIGHT, &g_adc_sample_data[0]);
			set_app_state(APP_STATE_RADIO_TX);
		}
		
		if (is_app_state_set(APP_STATE_RADIO_TX)) {
			clear_app_state(APP_STATE_RADIO_TX);
			protocol_send_packet();
		}
	}
}
