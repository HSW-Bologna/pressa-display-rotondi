#ifndef APP_CONF_H_INCLUDED
#define APP_CONF_H_INCLUDED


#define SOFTWARE_VERSION    "0.0.1"
#define SOFTWARE_BUILD_DATE __DATE__


#ifdef TARGET_DEBUG
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

#define APP_CONFIG_DRIVE_MOUNT_PATH  "/tmp/mnt"
#define APP_CONFIG_DATA_VERSION_FILE APP_CONFIG_DATA_PATH "version.txt"
#define APP_CONFIG_PARMAC_FILE       APP_CONFIG_DATA_PATH "/parmac.bin"
#define APP_CONFIG_LOGFILE           "/tmp/pressa_log.txt"
#define MAX_LOGFILE_SIZE             4000000UL


#endif
