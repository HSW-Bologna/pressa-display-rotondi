#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include "model.h"
#include <string.h>
#include "config/app_config.h"


static uint16_t adc_to_mm(model_t *model, uint16_t adc);


void model_init(mut_model_t *model) {
    assert(model != NULL);

    memset(model, 0, sizeof(mut_model_t));

    model->config.position_sensor_scale_mm = 200;

    for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
        snprintf(model->config.programs[i].name, sizeof(model->config.programs[i].name), "Program %i", i + 1);
        program_init(&model->config.programs[i]);
    }
    for (uint16_t i = 0; i < PROGRAM_NUM_CHANNELS; i++) {
        snprintf(model->config.channel_names[i], sizeof(model->config.channel_names[i]), "CH %i", i + 1);
    }

    model->run.current_program_index         = -1;
    model->run.minion.communication_enabled  = 1;
    model->run.minion.communication_error    = 0;
    model->run.num_importable_configurations = 0;
    model->run.importable_configurations     = NULL;
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
    CHECK_WITHIN(model->config.machine_model, MACHINE_MODEL_TRADITIONAL, MACHINE_MODEL_DOUBLE_FRONT);
    CHECK_WITHIN(model->config.position_sensor_scale_mm, APP_CONFIG_MIN_POSITION_SENSOR_SCALE_MM,
                 APP_CONFIG_MAX_POSITION_SENSOR_SCALE_MM);

    for (uint16_t i = 0; i < NUM_PROGRAMS; i++) {
        program_check_parameters(&model->config.programs[i], model->config.position_sensor_scale_mm);
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


void model_reset_program(mut_model_t *model, uint16_t program_index) {
    assert(model != NULL);
    program_init(&model->config.programs[program_index]);
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


uint16_t model_get_calibrated_position_mm(model_t *model) {
    assert(model != NULL);

    int16_t calibrated_ma4_20 = model->run.minion.read.ma4_20_adc - model->config.ma4_20_offset;
    if (calibrated_ma4_20 < 0) {
        calibrated_ma4_20 = 0;
    }

    return adc_to_mm(model, calibrated_ma4_20);
}


uint16_t model_get_uncalibrated_position_mm(model_t *model) {
    assert(model != NULL);

    return adc_to_mm(model, model->run.minion.read.ma4_20_adc);
}


uint16_t model_position_mm_to_adc(model_t *model, uint16_t mm) {
    assert(model != NULL);

    int16_t total_range_adc = model->run.minion.read.ma20_adc - model->run.minion.read.ma4_adc;
    if (total_range_adc <= 0 || model->config.position_sensor_scale_mm == 0) {
        return 0;
    }

    if (mm > model->config.position_sensor_scale_mm) {
        // Maximum value
        return model->run.minion.read.ma20_adc;
    } else {
        return (mm * total_range_adc) / (model->config.position_sensor_scale_mm);
    }
}


uint16_t model_get_current_position_target(model_t *model) {
    assert(model != NULL);

    const program_t *program         = model_get_current_program(model);
    uint32_t         elapsed_time_ms = model->run.minion.read.elapsed_milliseconds;

    if (program->time_unit_decisecs > 0) {
        uint16_t position_time_index = elapsed_time_ms / program->time_unit_decisecs;
        if (position_time_index < PROGRAM_NUM_TIME_UNITS) {
            uint16_t position_index = program->sensor_channel[position_time_index];
            if (position_index > 0 && position_index < 4) {
                return program->position_levels[position_index - 1];
            }
        }
    }

    return 0;
}


static uint16_t adc_to_mm(model_t *model, uint16_t adc) {
    assert(model != NULL);

    int16_t total_range_adc = model->run.minion.read.ma20_adc - model->run.minion.read.ma4_adc;
    if (total_range_adc <= 0) {
        return 0;
    }

    if (adc > total_range_adc) {
        // Maximum value
        return APP_CONFIG_MAX_POSITION_SENSOR_SCALE_MM;
    } else {
        return (adc * model->config.position_sensor_scale_mm) / total_range_adc;
    }
}
