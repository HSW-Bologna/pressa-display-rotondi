#ifndef TIMESTAMP_H_INCLUDED
#define TIMESTAMP_H_INCLUDED


#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"


typedef unsigned long timestamp_t;

#define TIMESTAMP_AFTER(a, b)          ((long)((b) - (a)) < 0)
#define TIMESTAMP_AFTER_OR_EQUAL(a, b) (((long)((b) - (a)) <= 0))


static inline timestamp_t __attribute__((always_inline)) timestamp_get() {
    return (xTaskGetTickCount() * portTICK_PERIOD_MS);
}


static inline __attribute__((always_inline)) uint8_t timestamp_is_expired_at(timestamp_t start, timestamp_t at,
                                                                             timestamp_t period) {
    return TIMESTAMP_AFTER_OR_EQUAL(at, start + period);
}


static inline __attribute__((always_inline)) timestamp_t timestamp_interval(timestamp_t a, timestamp_t b) {
    if (TIMESTAMP_AFTER(a, b))
        return -((timestamp_t)b - (timestamp_t)a);
    else
        return (timestamp_t)b - (timestamp_t)a;
}


static inline __attribute__((always_inline)) timestamp_t timestamp_since(timestamp_t ts) {
    return timestamp_interval(timestamp_get(), ts);
}


static inline __attribute__((always_inline)) uint8_t timestamp_is_expired(timestamp_t start, timestamp_t period) {
    return timestamp_is_expired_at(start, timestamp_get(), period);
}

#endif
