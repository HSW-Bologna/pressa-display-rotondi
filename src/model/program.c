#include <assert.h>
#include "program.h"


uint8_t program_get_digital_channel_state_at(const program_t *program, uint16_t channel, uint16_t instant) {
    assert(program && channel < PROGRAM_NUM_DIGITAL_CHANNELS);
    return (program->digital_channels[channel] & (((digital_channel_schedule_t)1) << instant)) > 0;
}


void program_flip_digital_channel_state_at(program_t *program, uint16_t channel, uint16_t instant) {
    assert(program && channel < PROGRAM_NUM_DIGITAL_CHANNELS);

    if (program_get_digital_channel_state_at(program, channel, instant)) {
        program->digital_channels[channel] &= ~(((digital_channel_schedule_t)1) << instant);
    } else {
        program->digital_channels[channel] |= ((digital_channel_schedule_t)1) << instant;
    }
}
