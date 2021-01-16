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


void mqtt_setup(void);
void mqtt_loop(void);

#endif /* !COORD_MQTT_H */
