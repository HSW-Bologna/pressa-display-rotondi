#include "model.h"
#include <string.h>

void model_init(mut_model_t *model) {
    for (int i = 0; i < NUM_PROGRAMS; i++) {
        snprintf(model->programs[i].name, sizeof(model->programs[i].name), "Program %d", i + 1);
        
        for (int j = 0; j < NUM_CHANNELS; j++) {
            snprintf(model->programs[i].channels[j].name, sizeof(model->programs[i].channels[j].name), "Channel %d", j + 1);
            
            for (int k = 0; k < NUM_TIME_UNITS; k++) {
                model->programs[i].channels[j].states[k] = false;
            }
        }
    }
}
