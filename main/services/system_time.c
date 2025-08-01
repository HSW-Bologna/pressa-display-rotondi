#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <esp_log.h>


#define RTC_ATTEMPTS 5


static const char *TAG = __FILE_NAME__;


static int change_rtc_time(time_t seconds) {

    return 0;
}


static int change_system_time(time_t seconds) {
    int res = 0;

#ifndef BUILD_CONFIG_SIMULATOR
    struct timespec tv = {.tv_sec = seconds, .tv_nsec = 0};

    if ((res = clock_settime(CLOCK_REALTIME, &tv))) {
        ESP_LOGW(TAG, "Non sono riuscito a impostare data e ora: %s", strerror(errno));
    } else {
        char *t          = ctime((const time_t *)&seconds);
        t[strlen(t) - 1] = '\0';
        ESP_LOGW(TAG, "Aggiornamento di data e ora: %s", t);
    }
#endif

    return res;
}


int update_system_rtc_time(time_t seconds) {
    int res1 = change_system_time(seconds);
    int res2 = change_rtc_time(seconds);
    return res1 || res2;
}


int init_system_time_from_rtc(void) {
    return 0;
}


time_t mktime_autodst(struct tm *tm) {
    // struct tm autodst = *tm;
    // mktime(&autodst);
    // tm->tm_isdst = autodst.tm_isdst;
    ESP_LOGI(TAG, "Ora da impostare %i:%i:%i (%i)", tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_isdst);
    time_t res = mktime(tm);
    ESP_LOGI(TAG, "Ora impostata %i:%i:%i (%i)", tm->tm_hour, tm->tm_min, tm->tm_sec, tm->tm_isdst);
    return res;
}
