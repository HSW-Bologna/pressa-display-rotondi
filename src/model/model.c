#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include "model.h"
#include <string.h>
#include "config/app_config.h"


void model_init(mut_model_t *model) {
    assert(model != NULL);

    memset(model, 0, sizeof(mut_model_t));

    for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
        snprintf(model->config.programs[i].name, sizeof(model->config.programs[i].name), "Program %i", i + 1);
        program_init(&model->config.programs[i]);
    }
    for (uint16_t i = 0; i < PROGRAM_NUM_CHANNELS; i++) {
        snprintf(model->config.channel_names[i], sizeof(model->config.channel_names[i]), "CH %i", i + 1);
    }

    model->run.current_program_index        = -1;
    model->run.minion.communication_enabled = 1;
    model->run.minion.communication_error   = 0;
    model->run.num_importable_configurations     = 0;
    model->run.importable_configurations         = NULL;
}


void model_check_parameters(mut_model_t *model) {
#define CHECK_WITHIN(Par, Min, Max)                                                                                    \
    if ((Par) < (Min)) {                                                                                               \
        (Par) = (Min);                                                                                                 \
    } else if ((Par) > (Max)) {                                                                                        \
        (Par) = (Max);                                                                                                 \
    }
    assert(model);

    CHECK_WITHIN(model->config.headgap_offset_up, APP_CONFIG_MIN_HEADGAP_OFFSET, APP_CONFIG_MAX_HEADGAP_OFFSET);
    CHECK_WITHIN(model->config.headgap_offset_down, APP_CONFIG_MIN_HEADGAP_OFFSET, APP_CONFIG_MAX_HEADGAP_OFFSET);

    for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
        program_check_parameters(&model->config.programs[i]);
    }
#undef CHECK_WITHIN
}


size_t model_get_num_programs(model_t *model) {
    assert(model != NULL);
    return 0;
}


void model_copy_program(mut_model_t *model, uint16_t source_index, uint16_t destination_index) {
    assert(model != NULL);
    model->config.programs[destination_index] = model->config.programs[source_index];
}


const char *model_get_program_name(model_t *model, size_t num) {
    assert(model != NULL);
    (void)num;
    return "MISSING";
}


program_t *model_get_program_mut(mut_model_t *model, size_t num) {
    assert(model != NULL && num < NUM_PROGRAMS);
    return &model->config.programs[num];
}


const program_t *model_get_program(model_t *model, size_t num) {
    assert(model != NULL && num < NUM_PROGRAMS);
    return &model->config.programs[num];
}


void model_clear_test_outputs(mut_model_t *model) {
    assert(model != NULL);

    model->run.minion.write.outputs = 0;
}


void model_set_test_output(mut_model_t *model, uint16_t output_index) {
    assert(model != NULL);

    model->run.minion.write.outputs = 1 << output_index;
}


void model_clear_current_program(mut_model_t *model) {
    assert(model != NULL);
    model->run.current_program_index = -1;
}


void model_set_current_program(mut_model_t *model, uint16_t current_program_index) {
    assert(model != NULL);
    model->run.current_program_index = current_program_index;
}


uint8_t model_is_program_ready(model_t *model) {
    assert(model != NULL);

    return model->run.current_program_index >= 0;
}


const program_t *model_get_current_program(model_t *model) {
    assert(model != NULL);

    if (model_is_program_ready(model)) {
        return &model->config.programs[model->run.current_program_index];
    } else {
        static program_t program = {0};
        return &program;
    }
}


uint8_t model_is_communication_ok(model_t *model) {
    assert(model != NULL);

    return model->run.minion.communication_enabled && !model->run.minion.communication_error;
}
