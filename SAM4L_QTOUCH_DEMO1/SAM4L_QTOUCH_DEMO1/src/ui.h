/**
 * \file
 *
 * \brief User Interface.
 *
 * Copyright (c) 2012 Atmel Corporation. All rights reserved.
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

#ifndef _UI_H
#define _UI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "sleepmgr.h"
#include "sysclk.h"
#include "board.h"
#include "c42364a.h"
#include "board_monitor.h"
#include "touch_api_sam4l.h"
#include "ioport.h"

void ui_set_mcu_status(power_scaling_t power_scaling, 
	sleep_mode_t sleep_mode, uint32_t cpu_freq, cpu_src_t cpu_src);
power_scaling_t ui_get_power_scaling_mcu_status(void);
void ui_set_power_scaling_mcu_status(power_scaling_t power_scaling);
sleep_mode_t ui_get_sleep_mode_mcu_status(void);
void ui_set_sleep_mode_mcu_status(sleep_mode_t sleep_mode);
void ui_bm_init(void);
void ui_bm_send_mcu_status(void);
void ui_lcd_init(void);
void ui_lcd_refresh_alphanum(bool ui_lcd_refresh, 
	int32_t event_qtouch_slider_position);
void ui_lcd_refresh_txt(void);

#endif  // _UI_H
