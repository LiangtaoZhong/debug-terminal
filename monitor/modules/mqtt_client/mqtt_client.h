/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-12-10     Lorenzo-Zhong       the first version
 */
#ifndef APPLICATIONS_MQTT_CLIENT_H_
#define APPLICATIONS_MQTT_CLIENT_H_

#include "paho_mqtt.h"

#define MQTT_URI                "tcp://129.211.87.210:1883"
#define MQTT_SUBTOPIC           "/python/sub"
#define MQTT_PUBTOPIC           "/python/mqtt"
#define MQTT_WILLMSG            "Goodbye!"
#define MQTT_USERNAME            "ternimal"
#define MQTT_PASSWORD            "Mjk2NTE3NDY5MTQ1OTg4OTg3MDAxNzQzNjk3NjUxMzAyNDA"

int mqtt_unsubscribe(int argc, char **argv);
int mqtt_subscribe(int argc, char **argv);
int mqtt_publish(char* topic, char* data);
int mqtt_stop(int argc, char **argv);
int mqtt_start(void);

#endif /* APPLICATIONS_MQTT_CLIENT_H_ */
