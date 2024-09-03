#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include "model.h"
#include <string.h>
#include "utils/system_time.h"


void model_init(mut_model_t *model) {
    assert(model != NULL);
    
    // TODO implement this
}


size_t model_get_num_programs(model_t *pmodel) {
    assert(pmodel != NULL);
    return pmodel->configuration.num_programs;
}


const char *model_get_program_name(model_t *pmodel, size_t num) {
    assert(pmodel != NULL);
    if (num < model_get_num_programs(pmodel)) {
        return model_get_program(pmodel, num)->nomi;
    } else {
        return "MISSING";
    }
}


program_t *model_get_program(model_t *pmodel, size_t num) {
    assert(pmodel != NULL);

    if (num >= model_get_num_programs(pmodel)) {
        return NULL;
    } else {
        return &pmodel->configuration.programs[num];
    }
}

