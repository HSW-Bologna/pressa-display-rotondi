#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include "model.h"
#include <string.h>


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
}


void model_check_parameters(mut_model_t *model) {
    assert(model);

    for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
        program_check_parameters(&model->config.programs[i]);
    }
}


size_t model_get_num_programs(model_t *model) {
    assert(model != NULL);
    return 0;
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
