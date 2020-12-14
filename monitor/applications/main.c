/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-12-05     Lorenzo-Zhong    first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"

#define BLUE_LED_PIN GET_PIN(I, 8)
#define RED_LED_PIN GET_PIN(C, 15)

#define LCD_LED1 GET_PIN(B, 1)
#define LCD_LED2 GET_PIN(H, 2)
int main(void)
{
    rt_uint32_t count = 1;

    rt_pin_mode(RED_LED_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(BLUE_LED_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_LED1, PIN_MODE_OUTPUT);
    rt_pin_mode(LCD_LED2, PIN_MODE_OUTPUT);

    while(count++)
    {
        rt_thread_mdelay(500);
        rt_pin_write(BLUE_LED_PIN, PIN_HIGH);
        rt_pin_write(LCD_LED1, PIN_HIGH);
        rt_pin_write(LCD_LED2, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LCD_LED1, PIN_LOW);
        rt_pin_write(LCD_LED2, PIN_LOW);
        rt_pin_write(BLUE_LED_PIN, PIN_LOW);
    }
    return RT_EOK;
}

#include "stm32h7xx.h"
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);


