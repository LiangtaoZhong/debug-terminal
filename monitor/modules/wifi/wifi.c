/*************************************************
Copyright (c) 2020
All rights reserved.
File name:     wifi.c
Description:   
History:
1. Version:    V1.0.0
Date:      2020-09-15
Author:    WKJay
Modify:
*************************************************/
#include <rtthread.h>
#include <rtdevice.h>
#include <cJSON.h>
#include <wlan_mgnt.h>
#include <netdev_ipaddr.h>
#include <netdev.h>

#include "wifi.h"

#define DBG_TAG "wifi"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define MAX_SSID_PASSWD_STR_LEN 50
#define BT_SEND_TIMES 1
#define BT_SEND_FAIL_RETRY 3

extern int bt_stack_blufi_send(uint8_t *string, uint32_t length);
extern int adb_socket_init(void);

char wifi_status_str[100];

struct _wifi
{
    char ssid[MAX_SSID_PASSWD_STR_LEN];
    char passwd[MAX_SSID_PASSWD_STR_LEN];
} wifi;


int wifi_connect(char* ssid, char* passwd) //更改入口参数为字符串而不是JSON格式
{
    if (ssid && passwd)
    {
        if (rt_strlen(ssid) > MAX_SSID_PASSWD_STR_LEN ||
            rt_strlen(passwd) > MAX_SSID_PASSWD_STR_LEN)
        {
            LOG_E("invalid ssid or passwd length,max %d", MAX_SSID_PASSWD_STR_LEN);
        }
        else
        {
            rt_memcpy(wifi.ssid, ssid, rt_strlen(ssid));
            rt_memcpy(wifi.passwd, passwd, rt_strlen(passwd));
            return rt_wlan_connect(wifi.ssid, wifi.passwd);
        }
    }
    else
    {
        LOG_E("cannot find ssid or password.");
    }

    return -1;
}

int wifi_is_ready(void)
{
    return rt_wlan_is_ready();
}

char *wifi_get_ip(void)
{
    struct netdev *dev = netdev_get_by_name(WIFI_DEVICE_NAME);
    if (dev)
    {
        return inet_ntoa(dev->ip_addr);
    }
    else
    {
        return NULL;
    }
}

char *wifi_status_get(void)
{
    rt_memset(wifi_status_str, 0, sizeof(wifi_status_str));
    uint8_t wifi_status = wifi_is_ready();
    char *wifi_ip = wifi_get_ip();
    rt_sprintf(wifi_status_str, "{wifi:'%s', url:'%s'}", wifi_status ? "on" : "off", wifi_ip);
    return wifi_status_str;
}

static void wifi_ready_handler(int event, struct rt_wlan_buff *buff, void *parameter)
{
    rt_kprintf(wifi_status_str);
}

int wifi_init(void)
{
    rt_memset(&wifi, 0, sizeof(wifi));
    rt_wlan_register_event_handler(RT_WLAN_EVT_READY, wifi_ready_handler, NULL);
    return 0;
}
