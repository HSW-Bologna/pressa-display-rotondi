#ifndef APP_CONF_H_INCLUDED
#define APP_CONF_H_INCLUDED


#define SOFTWARE_VERSION    "0.0.1"
#define SOFTWARE_BUILD_DATE __DATE__


#ifdef BUILD_CONFIG_SIMULATOR
#define APP_CONFIG_DATA_PATH        "./data"
#define APP_CONFIG_IFWIFI           "wlp112s0"
#define APP_CONFIG_IFETH            "enp109s0"
#define APP_CONFIG_HTTP_SERVER_PORT 8080
#define APP_CONFIG_LOG_LEVEL        LOG_DEBUG
#else
#define APP_CONFIG_DATA_PATH        "/mnt/data"
#define APP_CONFIG_IFWIFI           "wlan0"
#define APP_CONFIG_IFETH            "eth0"
#define APP_CONFIG_HTTP_SERVER_PORT 80
#define APP_CONFIG_LOG_LEVEL        LOG_INFO
#endif

#define APP_CONFIG_DATA_VERSION 1

#define APP_CONFIG_CONFIGURATION_PATH APP_CONFIG_DATA_PATH "/configuration.bin"
#define APP_CONFIG_DRIVE_MOUNT_PATH   "/tmp/mnt"
#define APP_CONFIG_LOGFILE            "/tmp/pressa_log.txt"
#define MAX_LOGFILE_SIZE              4000000UL

#define APP_CONFIG_MIN_TIME_UNIT_DECISECS 5
#define APP_CONFIG_MAX_TIME_UNIT_DECISECS 20
#define APP_CONFIG_MIN_DAC_LEVEL          0
#define APP_CONFIG_MAX_DAC_LEVEL          100
#define APP_CONFIG_MIN_SENSOR_LEVEL       0
#define APP_CONFIG_MAX_SENSOR_LEVEL       100


#endif
