/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-12-05     Lorenzo-Zhong       the first version
 */

#include <rtthread.h>

#define DBG_TAG "correspondence"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#include "string.h"
#include "paho_mqtt.h"
#include "mqtt_client.h"
#include "cjson.h"
#include "stdlib.h"
#include "wifi.h"

/* 用于接收消息的信号量 */
static struct rt_semaphore rx_sem;
static rt_device_t serial;
#define TERMINAL_UART_NAME       "uart1"

#define DATA_MAX_SIZE 100
#define HEAD 0xAA
#define TAIL 0xBB
#define CTL  0x43
#define DATA 0x44

#define ssid  "Oneplus6"
#define passwd  "123456789"

typedef struct Msg
{
    uint8_t head;
    uint8_t type;
    uint8_t length;
    uint8_t data[DATA_MAX_SIZE];
    uint8_t sum;
    uint8_t tail;
} Msg;

uint8_t parse_ok = 0;
Msg serial_msg = { 0 };

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_sem_release(&rx_sem);
    return RT_EOK;
}
//状态机实现数据解析
static void state_machine(uint8_t data)
{
    static uint8_t state = 0;
    static uint8_t cnt = 0;
    //rt_kprintf("state:%d\r\n", state);
    switch (state)
    {
    case 0:
        if (data == HEAD)
        {
            serial_msg.head = data;
            ++state;
        }
        else
        {
            state = 0;
        }
        break;
    case 1:
        if (data == CTL || data == DATA)
        {
            serial_msg.type = data;
            ++state;
        }
        else if (data == HEAD)
        {
            state = 1;
        }
        else
        {
            state = 0;
        }
        break;
    case 2:
        serial_msg.length = data;
        ++state;
        break;
    case 3:
        serial_msg.data[cnt++] = data;
        if (cnt == serial_msg.length)
        {
            cnt = 0;
            ++state;
        }
        break;
    case 4:
        //TODO: 添加数据接收校验
        serial_msg.sum = data;
        ++state;
        break;
    case 5:
        if (data == TAIL)
        {
            serial_msg.tail = data;
            state = 0;
            parse_ok = 1;
        }
        else
        {
            memset(&serial_msg, 0, sizeof(serial_msg));
            state = 0;
            parse_ok = 0;
        }
        break;
    default:
        break;
    }
}
//数据打包
static char* data_pack(const Msg msg)
{
    cJSON* usr;
    cJSON* array;

    usr = cJSON_CreateObject();
    cJSON_AddNumberToObject(usr, "length", msg.length);

    array = cJSON_CreateArray();

    cJSON_AddNumberToObject(usr, "type", msg.type);

    for (int i = 0; i < msg.length; ++i)
    {
        cJSON_AddItemToArray(array, cJSON_CreateNumber(msg.data[i]));
    }

    cJSON_AddItemToObject(usr, "data", array);
    cJSON_AddNumberToObject(usr, "sum", msg.sum);
    char* s = cJSON_Print(usr);
    if (s)
    {
        rt_kprintf("%s\n", s);
        return s;
    }
    if (usr)
    {
        cJSON_Delete(usr);
    }
    return NULL;
}
//数据处理线程
static void serial_thread_entry(void *parameter)
{
    uint8_t ch[1];
    while (1)
    {
        /* 从串口读取一个字节的数据，没有读取到则等待接收信号量 */
        while (1)
        {
            /* 阻塞等待接收信号量，等到信号量后再次读取数据 */
            rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
            if (rt_device_read(serial, -1, &ch, 1) == 1)
            {
                state_machine(ch[0]); //状态机解析串口数据
            }

            if (parse_ok == 1)
            {
                parse_ok = 0;
                char* data = data_pack(serial_msg); //将数据打包成json格式
                mqtt_publish(MQTT_PUBTOPIC, data);  //发布MQTT话题
                free(data);
            }
        }

    }
}
//MQTT任务
static int mqtt_start_task(void)
{
    static int count = 3, time = 0;
    int ret = -1;
    while ((ret = mqtt_start()) && count--)
    {
        rt_thread_mdelay(500);
        rt_kprintf("attempt reconnect: %d!\n", ++time);
    }

    return ret;
}

int terminal_init(void)
{
    rt_err_t ret = RT_EOK;
    char uart_name[RT_NAME_MAX];
    char str[] = "hello RT-Thread!\r\n";

    rt_strncpy(uart_name, TERMINAL_UART_NAME, RT_NAME_MAX);

    serial = rt_device_find(uart_name);
    if (!serial)
    {
        rt_kprintf("find %s failed!\n", uart_name);
        return RT_ERROR;
    }

    /* 初始化信号量 */
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
    /* 以中断接收及轮询发送模式打开串口设备 */
    rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
    rt_device_set_rx_indicate(serial, uart_input);
    /* 发送字符串 */
    rt_device_write(serial, 0, str, (sizeof(str) - 1));

    wifi_init();
    wifi_connect(ssid, passwd);

    while (!wifi_is_ready())
    {
        rt_kprintf("wifi is not ready!\n");
        rt_thread_mdelay(1000);
    }
    rt_kprintf("wifi is ready!\n");
    mqtt_start_task(); //开启MQTT客户端

    rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 4096, 25, 10);
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        ret = RT_ERROR;
    }

    return ret;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(terminal_init, terminal task);
