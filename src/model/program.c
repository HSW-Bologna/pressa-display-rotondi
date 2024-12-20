#include <assert.h>
#include "program.h"
#include "app_config.h"


void program_init(program_t *program) {
    assert(program);
    program->time_unit_decisecs = APP_CONFIG_MIN_TIME_UNIT_DECISECS;

    for (uint16_t i = 0; i < PROGRAM_DAC_LEVELS; i++) {
        program->dac_levels[i] = APP_CONFIG_MIN_DAC_LEVEL +
                                 ((APP_CONFIG_MAX_DAC_LEVEL - APP_CONFIG_MIN_DAC_LEVEL) / PROGRAM_DAC_LEVELS) * (i + 1);
    }
}


void program_check_parameters(program_t *program) {
#define CHECK_WITHIN(Par, Min, Max)                                                                                    \
    if ((Par) < (Min)) {                                                                                               \
        (Par) = (Min);                                                                                                 \
    } else if ((Par) > (Max)) {                                                                                        \
        (Par) = (Max);                                                                                                 \
    }
    assert(program);

    CHECK_WITHIN(program->time_unit_decisecs, APP_CONFIG_MIN_TIME_UNIT_DECISECS, APP_CONFIG_MAX_TIME_UNIT_DECISECS);
    CHECK_WITHIN(program->dac_levels[0], APP_CONFIG_MIN_DAC_LEVEL, APP_CONFIG_MAX_DAC_LEVEL);
    CHECK_WITHIN(program->dac_levels[1], APP_CONFIG_MIN_DAC_LEVEL, APP_CONFIG_MAX_DAC_LEVEL);
    CHECK_WITHIN(program->dac_levels[2], APP_CONFIG_MIN_DAC_LEVEL, APP_CONFIG_MAX_DAC_LEVEL);

    CHECK_WITHIN(program->sensor_levels[0], APP_CONFIG_MIN_SENSOR_LEVEL, APP_CONFIG_MAX_SENSOR_LEVEL);
    CHECK_WITHIN(program->sensor_levels[1], APP_CONFIG_MIN_SENSOR_LEVEL, APP_CONFIG_MAX_SENSOR_LEVEL);
    CHECK_WITHIN(program->sensor_levels[2], APP_CONFIG_MIN_SENSOR_LEVEL, APP_CONFIG_MAX_SENSOR_LEVEL);

#undef CHECK_WITHIN
}


uint8_t program_get_digital_channel_state_at(const program_t *program, uint16_t channel, uint16_t instant) {
    assert(program && channel < PROGRAM_NUM_DIGITAL_CHANNELS);
    return (program->digital_channels[channel] & (((program_digital_channel_schedule_t)1) << instant)) > 0;
}


program_dac_channel_state_t program_get_dac_channel_state_at(const program_t *program, uint16_t instant) {
    assert(program);
    return program->dac_channel[instant];
}


program_sensor_channel_threshold_t program_get_sensor_channel_state_at(const program_t *program, uint16_t instant) {
    assert(program);
    return program->sensor_channel[instant];
}


void program_increase_dac_channel_state_at(program_t *program, uint16_t instant) {
    assert(program);
    program->dac_channel[instant] = (program->dac_channel[instant] + 1) % PROGRAM_DAC_CHANNEL_STATE_NUM;
}


void program_increase_sensor_channel_threshold_at(program_t *program, uint16_t instant) {
    assert(program);
    program->sensor_channel[instant] = (program->sensor_channel[instant] + 1) % PROGRAM_SENSOR_CHANNEL_THRESHOLD_NUM;
}


void program_flip_digital_channel_state_at(program_t *program, uint16_t channel, uint16_t instant) {
    assert(program && channel < PROGRAM_NUM_DIGITAL_CHANNELS);

    if (program_get_digital_channel_state_at(program, channel, instant)) {
        program->digital_channels[channel] &= ~(((program_digital_channel_schedule_t)1) << instant);
    } else {
        program->digital_channels[channel] |= ((program_digital_channel_schedule_t)1) << instant;
    }
}
