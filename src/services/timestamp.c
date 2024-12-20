#include "timestamp.h"
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>


timestamp_t timestamp_get(void) {
    unsigned long   now_ms;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    now_ms = ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL;

    return now_ms;
}
