#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "program.h"
#include "app_config.h"


void program_init(program_t *program) {
    assert(program);
    program->time_unit_decisecs = APP_CONFIG_MIN_TIME_UNIT_DECISECS;
    memset(program->digital_channels, 0, sizeof(program->digital_channels));
    memset(program->sensor_channel, 0, sizeof(program->sensor_channel));
    memset(program->pressure_channel, 0, sizeof(program->pressure_channel));

    for (uint16_t i = 0; i < PROGRAM_PRESSURE_LEVELS; i++) {
        program->pressure_levels[i] =
            APP_CONFIG_MIN_PRESSURE_LEVEL +
            ((APP_CONFIG_MAX_PRESSURE_LEVEL - APP_CONFIG_MIN_PRESSURE_LEVEL) / PROGRAM_PRESSURE_LEVELS) * (i + 1);

        program->position_levels[i] =
            APP_CONFIG_MIN_POSITION_SENSOR_SCALE_MM +
            (((APP_CONFIG_MAX_POSITION_SENSOR_SCALE_MM - APP_CONFIG_MIN_POSITION_SENSOR_SCALE_MM)) /
             PROGRAM_SENSOR_LEVELS) *
                (i + 1);
    }
}


void program_check_parameters(program_t *program, uint16_t max_position_level) {
#define CHECK_WITHIN(Par, Min, Max)                                                                                    \
    if ((Par) < (Min)) {                                                                                               \
        (Par) = (Min);                                                                                                 \
    } else if ((Par) > (Max)) {                                                                                        \
        (Par) = (Max);                                                                                                 \
    }
    assert(program);

    CHECK_WITHIN(program->time_unit_decisecs, APP_CONFIG_MIN_TIME_UNIT_DECISECS, APP_CONFIG_MAX_TIME_UNIT_DECISECS);
    CHECK_WITHIN(program->pressure_levels[0], APP_CONFIG_MIN_PRESSURE_LEVEL, APP_CONFIG_MAX_PRESSURE_LEVEL);
    CHECK_WITHIN(program->pressure_levels[1], APP_CONFIG_MIN_PRESSURE_LEVEL, APP_CONFIG_MAX_PRESSURE_LEVEL);
    CHECK_WITHIN(program->pressure_levels[2], APP_CONFIG_MIN_PRESSURE_LEVEL, APP_CONFIG_MAX_PRESSURE_LEVEL);

    CHECK_WITHIN(program->position_levels[2], 0, max_position_level);
    CHECK_WITHIN(program->position_levels[0], 0, max_position_level);
    CHECK_WITHIN(program->position_levels[1], 0, max_position_level);

    for (size_t i = 0; i < PROGRAM_NUM_TIME_UNITS; i++) {
        // The pressure channel also flips the corresponding digital output
        if (program->pressure_channel[i] == 0) {
            program->digital_channels[PROGRAM_PRESSURE_CHANNEL_INDEX] &=
                ~(((program_digital_channel_schedule_t)1) << i);
        } else {
            program->digital_channels[PROGRAM_PRESSURE_CHANNEL_INDEX] |= ((program_digital_channel_schedule_t)1) << i;
        }
    }

#undef CHECK_WITHIN
}


uint8_t program_get_digital_channel_state_at(const program_t *program, uint16_t channel, uint16_t instant) {
    assert(program && channel < PROGRAM_NUM_PROGRAMMABLE_CHANNELS);
    return (program->digital_channels[channel] & (((program_digital_channel_schedule_t)1) << instant)) > 0;
}


program_pressure_channel_state_t program_get_pressure_channel_state_at(const program_t *program, uint16_t instant) {
    assert(program);
    return program->pressure_channel[instant];
}


program_sensor_channel_threshold_t program_get_sensor_channel_state_at(const program_t *program, uint16_t instant) {
    assert(program);
    return program->sensor_channel[instant];
}


void program_increase_pressure_channel_state_at(program_t *program, uint16_t instant) {
    assert(program);
    program->pressure_channel[instant] = (program->pressure_channel[instant] + 1) % PROGRAM_PRESSURE_CHANNEL_STATE_NUM;
    // The pressure channel also flips the corresponding digital output
    if (program->pressure_channel[instant] == 0) {
        program->digital_channels[PROGRAM_PRESSURE_CHANNEL_INDEX] &=
            ~(((program_digital_channel_schedule_t)1) << instant);
    } else {
        program->digital_channels[PROGRAM_PRESSURE_CHANNEL_INDEX] |= ((program_digital_channel_schedule_t)1) << instant;
    }
}


void program_increase_sensor_channel_threshold_at(program_t *program, uint16_t instant) {
    assert(program);
    program->sensor_channel[instant] = (program->sensor_channel[instant] + 1) % PROGRAM_SENSOR_CHANNEL_THRESHOLD_NUM;
}


void program_flip_digital_channel_state_at(program_t *program, uint16_t channel, uint16_t instant) {
    assert(program && channel < PROGRAM_NUM_PROGRAMMABLE_CHANNELS);

    if (program_get_digital_channel_state_at(program, channel, instant)) {
        program->digital_channels[channel] &= ~(((program_digital_channel_schedule_t)1) << instant);
    } else {
        program->digital_channels[channel] |= ((program_digital_channel_schedule_t)1) << instant;
    }
}


uint32_t program_get_duration_milliseconds(const program_t *program) {
    assert(program);

    return PROGRAM_NUM_TIME_UNITS * program->time_unit_decisecs * 100;
}
