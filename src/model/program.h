#ifndef MODEL_PROGRAM_H_INCLUDED
#define MODEL_PROGRAM_H_INCLUDED


#include <stdint.h>


#define PROGRAM_NAME_LENGTH            20
#define PROGRAM_NAME_SIZE              (PROGRAM_NAME_LENGTH + 1)
#define PROGRAM_NUM_DIGITAL_CHANNELS   14
#define PROGRAM_NUM_CHANNELS           (PROGRAM_NUM_DIGITAL_CHANNELS + 2)
#define PROGRAM_NUM_TIME_UNITS         25


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
        program_digital_channel_schedule_t;

typedef enum {
    PROGRAM_DAC_CHANNEL_STATE_OFF = 0,
    PROGRAM_DAC_CHANNEL_STATE_LOW,
    PROGRAM_DAC_CHANNEL_STATE_MEDIUM,
    PROGRAM_DAC_CHANNEL_STATE_HIGH,
#define PROGRAM_DAC_CHANNEL_STATE_NUM 4
} program_dac_channel_state_t;

#define PROGRAM_DAC_LEVELS (PROGRAM_DAC_CHANNEL_STATE_NUM - 1)


typedef enum {
    PROGRAM_SENSOR_CHANNEL_THRESHOLD_NONE = 0,
    PROGRAM_SENSOR_CHANNEL_THRESHOLD_CLOSE,
    PROGRAM_SENSOR_CHANNEL_THRESHOLD_MEDIUM,
    PROGRAM_SENSOR_CHANNEL_THRESHOLD_FAR,
#define PROGRAM_SENSOR_CHANNEL_THRESHOLD_NUM 4
} program_sensor_channel_threshold_t;

#define PROGRAM_SENSOR_LEVELS (PROGRAM_SENSOR_CHANNEL_THRESHOLD_NUM - 1)


typedef struct {
    name_t                             name;
    program_digital_channel_schedule_t digital_channels[PROGRAM_NUM_DIGITAL_CHANNELS];
    program_dac_channel_state_t        dac_channel[PROGRAM_NUM_TIME_UNITS];
    program_sensor_channel_threshold_t sensor_channel[PROGRAM_NUM_TIME_UNITS];
    uint16_t                           time_unit_decisecs;
    program_dac_channel_state_t        dac_levels[PROGRAM_DAC_LEVELS];
    program_sensor_channel_threshold_t sensor_levels[PROGRAM_SENSOR_LEVELS];
} program_t;


uint8_t program_get_digital_channel_state_at(const program_t *program, uint16_t channel, uint16_t instant);
void    program_flip_digital_channel_state_at(program_t *program, uint16_t channel, uint16_t instant);
program_sensor_channel_threshold_t program_get_sensor_channel_state_at(const program_t *program, uint16_t instant);
program_dac_channel_state_t        program_get_dac_channel_state_at(const program_t *program, uint16_t instant);
void                               program_increase_dac_channel_state_at(program_t *program, uint16_t instant);
void                               program_increase_sensor_channel_threshold_at(program_t *program, uint16_t instant);
void                               program_init(program_t *program);
void                               program_check_parameters(program_t *program);


#endif
