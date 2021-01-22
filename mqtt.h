/*
 * mqtt.h - MQTT setup and input processing
 *
 * Copyright (C) 2021 Linzhi Ltd.
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file COPYING.txt
 */

#ifndef COORD_MQTT_H
#define	COORD_MQTT_H

#define	MQTT_TOPIC_HIGHLIGHT	"/sys/highlight"
#define	MQTT_TOPIC_CLEAR	"/sys/clear-shutdown"
#define	MQTT_TOPIC_PSHUT	"/power/shutdown"
#define	MQTT_TOPIC_I2CWARN	"/sys/i2c/warning"
#define	MQTT_TOPIC_I2CSHUT	"/sys/i2c/shutdown"
#define	MQTT_TOPIC_TSHUT	"/temp/shutdown"
#define	MQTT_TOPIC_TWARN	"/temp/warning"
#define	MQTT_TOPIC_CARD_WARN	"/card/warning"
#define	MQTT_TOPIC_POOL_STATS	"/mine/pool/statistics"
#define	MQTT_TOPIC_USER		"/sys/button/user"
#define	MQTT_TOPIC_MINED_STATE	"/mine/mined-state"
#define	MQTT_TOPIC_RECOVERY	"/sys/recovery-mode"
#define	MQTT_TOPIC_RESET	"/mine/reset"
#define	MQTT_TOPIC_BOOT_PROBLEM	"/sys/boot/problem"

#define	MQTT_TOPIC_DARK_MODE	"/sys/led/dark-mode"

#define	MQTT_TOPIC_LED_RGB	"/sys/led/rgb"
#define	MQTT_TOPIC_LED_ETHER	"/sys/led/ether"

#define	MQTT_TOPIC_0_TEMP	"/temp/0/current"
#define	MQTT_TOPIC_1_TEMP	"/temp/1/current"
#define	MQTT_TOPIC_FAN_PROFILE	"/fan/profile"
#define	MQTT_TOPIC_ALL_PWM_SET	"/fan/all/pwm-set"


void mqtt_setup(void);
void mqtt_loop(void);

#endif /* !COORD_MQTT_H */
