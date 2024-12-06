#ifndef MODEL_PROGRAM_H_INCLUDED
#define MODEL_PROGRAM_H_INCLUDED


#include <stdint.h>


#define PROGRAM_NAME_LENGTH          20
#define PROGRAM_NAME_SIZE            (PROGRAM_NAME_LENGTH + 1)
#define PROGRAM_NUM_DIGITAL_CHANNELS 14
#define PROGRAM_NUM_CHANNELS         (PROGRAM_NUM_DIGITAL_CHANNELS + 2)
#define PROGRAM_NUM_TIME_UNITS       25


typedef char name_t[PROGRAM_NAME_SIZE];

typedef
#if PROGRAM_NUM_TIME_UNITS <= 8
    uint8_t
#elif PROGRAM_NUM_TIME_UNITS <= 16
    uint16_t
#elif PROGRAM_NUM_TIME_UNITS <= 32
    uint32_t
#elif PROGRAM_NUM_TIME_UNITS <= 64
    uint64_t
#else
#error "Too many time units!"
#endif
        digital_channel_schedule_t;

typedef enum {
    DAC_CHANNEL_STATE_OFF = 0,
    DAC_CHANNEL_STATE_LOW,
    DAC_CHANNEL_STATE_MEDIUM,
    DAC_CHANNEL_STATE_HIGH,
} dac_channel_state_t;

typedef enum {
    SENSOR_CHANNEL_THRESHOLD_NONE = 0,
    SENSOR_CHANNEL_THRESHOLD_CLOSE,
    SENSOR_CHANNEL_THRESHOLD_MEDIUM,
    SENSOR_CHANNEL_THRESHOLD_FAR,
} sensor_channel_threshold_t;

typedef struct {
    name_t                     name;
    digital_channel_schedule_t digital_channels[PROGRAM_NUM_DIGITAL_CHANNELS];
    dac_channel_state_t        dac_channel[PROGRAM_NUM_TIME_UNITS];
    sensor_channel_threshold_t sensor_channel[PROGRAM_NUM_TIME_UNITS];
} program_t;


uint8_t program_get_digital_channel_state_at(const program_t *program, uint16_t channel, uint16_t instant);
void    program_flip_digital_channel_state_at(program_t *program, uint16_t channel, uint16_t instant);


#endif
