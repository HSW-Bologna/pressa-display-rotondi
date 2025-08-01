#ifndef MODEL_PROGRAM_H_INCLUDED
#define MODEL_PROGRAM_H_INCLUDED


#include <stdint.h>


#define PROGRAM_NAME_LENGTH               20
#define PROGRAM_NAME_SIZE                 (PROGRAM_NAME_LENGTH + 1)
#define PROGRAM_NUM_CHANNELS              16
#define PROGRAM_NUM_PROGRAMMABLE_CHANNELS 15
#define PROGRAM_NUM_TIME_UNITS            25
#define PROGRAM_PRESSURE_CHANNEL_INDEX    7
#define PROGRAM_SENSOR_CHANNEL_INDEX      11


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
    PROGRAM_PRESSURE_CHANNEL_STATE_OFF = 0,
    PROGRAM_PRESSURE_CHANNEL_STATE_LOW,
    PROGRAM_PRESSURE_CHANNEL_STATE_MEDIUM,
    PROGRAM_PRESSURE_CHANNEL_STATE_HIGH,
#define PROGRAM_PRESSURE_CHANNEL_STATE_NUM 4
} program_pressure_channel_state_t;

#define PROGRAM_PRESSURE_LEVELS (PROGRAM_PRESSURE_CHANNEL_STATE_NUM - 1)


typedef enum {
    PROGRAM_SENSOR_CHANNEL_THRESHOLD_NONE = 0,
    PROGRAM_SENSOR_CHANNEL_THRESHOLD_CLOSE,
    PROGRAM_SENSOR_CHANNEL_THRESHOLD_MEDIUM,
    PROGRAM_SENSOR_CHANNEL_THRESHOLD_FAR,
#define PROGRAM_SENSOR_CHANNEL_THRESHOLD_NUM 4
} program_sensor_channel_threshold_t;

#define PROGRAM_SENSOR_LEVELS (PROGRAM_SENSOR_CHANNEL_THRESHOLD_NUM - 1)


typedef struct {
    name_t name;
    // Pressure and position, at indexes 7 and 11, are special; their digital channel is always empty and they are
    // managed with the two following fields
    program_digital_channel_schedule_t digital_channels[PROGRAM_NUM_PROGRAMMABLE_CHANNELS];
    program_pressure_channel_state_t   pressure_channel[PROGRAM_NUM_TIME_UNITS];
    program_sensor_channel_threshold_t sensor_channel[PROGRAM_NUM_TIME_UNITS];
    uint16_t                           time_unit_decisecs;
    uint16_t                           pressure_levels[PROGRAM_PRESSURE_LEVELS];
    uint16_t                           position_levels[PROGRAM_SENSOR_LEVELS];
} program_t;


uint8_t program_get_digital_channel_state_at(const program_t *program, uint16_t channel, uint16_t instant);
void    program_flip_digital_channel_state_at(program_t *program, uint16_t channel, uint16_t instant);
program_sensor_channel_threshold_t program_get_sensor_channel_state_at(const program_t *program, uint16_t instant);
program_pressure_channel_state_t   program_get_pressure_channel_state_at(const program_t *program, uint16_t instant);
void                               program_increase_pressure_channel_state_at(program_t *program, uint16_t instant);
void                               program_increase_sensor_channel_threshold_at(program_t *program, uint16_t instant);
void                               program_init(program_t *program);
void                               program_check_parameters(program_t *program, uint16_t max_position_level);
uint32_t                           program_get_duration_milliseconds(const program_t *program);


#endif
