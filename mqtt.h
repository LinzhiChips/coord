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
#define	MQTT_TOPIC_0_SKIP	"/temp/0/skip"
#define	MQTT_TOPIC_1_SKIP	"/temp/1/skip"
#define	MQTT_TOPIC_FAN_PROFILE	"/fan/profile"
#define	MQTT_TOPIC_ALL_PWM_SET	"/fan/all/pwm-set"

#define	MQTT_TOPIC_TEMP_OVERDUE	"/temp/%s/overdue"

#define	MQTT_TOPIC_MINED_TIME	"/daemon/mined/time"
#define	MQTT_TOPIC_MINED0_TIME	"/daemon/mined0/time"
#define	MQTT_TOPIC_MINED1_TIME	"/daemon/mined1/time"
#define	MQTT_TOPIC_DAEMON_START	"/daemon/%s/start"
#define	MQTT_TOPIC_DAEMON_STOP	"/daemon/%s/stop"

#define	MQTT_TOPIC_ONOFF_MASTER	"/power/on/master"
#define	MQTT_TOPIC_ONOFF_SLOT0	"/power/on/slot0"
#define	MQTT_TOPIC_ONOFF_SLOT1	"/power/on/slot1"

#define	MQTT_TOPIC_CFG_MASTER	"/config/user/SWITCH_LAST/set"
#define	MQTT_TOPIC_CFG_SLOT0	"/config/user0/SWITCH_LAST/set"
#define	MQTT_TOPIC_CFG_SLOT1	"/config/user1/SWITCH_LAST/set"

void mqtt_setup(void);
void mqtt_loop(void);

#endif /* !COORD_MQTT_H */
