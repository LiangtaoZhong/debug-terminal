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

/* ���ڽ�����Ϣ���ź��� */
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

/* �������ݻص����� */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    /* ���ڽ��յ����ݺ�����жϣ����ô˻ص�������Ȼ���ͽ����ź��� */
    rt_sem_release(&rx_sem);
    return RT_EOK;
}
//״̬��ʵ�����ݽ���
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
        //TODO: ������ݽ���У��
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
//���ݴ��
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
//���ݴ����߳�
static void serial_thread_entry(void *parameter)
{
    uint8_t ch[1];
    while (1)
    {
        /* �Ӵ��ڶ�ȡһ���ֽڵ����ݣ�û�ж�ȡ����ȴ������ź��� */
        while (1)
        {
            /* �����ȴ������ź������ȵ��ź������ٴζ�ȡ���� */
            rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
            if (rt_device_read(serial, -1, &ch, 1) == 1)
            {
                state_machine(ch[0]); //״̬��������������
            }

            if (parse_ok == 1)
            {
                parse_ok = 0;
                char* data = data_pack(serial_msg); //�����ݴ����json��ʽ
                mqtt_publish(MQTT_PUBTOPIC, data);  //����MQTT����
                free(data);
            }
        }

    }
}
//MQTT����
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

    /* ��ʼ���ź��� */
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
    /* ���жϽ��ռ���ѯ����ģʽ�򿪴����豸 */
    rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
    rt_device_set_rx_indicate(serial, uart_input);
    /* �����ַ��� */
    rt_device_write(serial, 0, str, (sizeof(str) - 1));

    wifi_init();
    wifi_connect(ssid, passwd);

    while (!wifi_is_ready())
    {
        rt_kprintf("wifi is not ready!\n");
        rt_thread_mdelay(1000);
    }
    rt_kprintf("wifi is ready!\n");
    mqtt_start_task(); //����MQTT�ͻ���

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
/* ������ msh �����б��� */
MSH_CMD_EXPORT(terminal_init, terminal task);
