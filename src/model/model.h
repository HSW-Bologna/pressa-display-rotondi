#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED

#include <stdbool.h>

#define NUM_PROGRAMS 10
#define NUM_CHANNELS 15
#define NUM_TIME_UNITS 25
#define MAX_PROGRAM_NAME_LENGTH 20
#define MAX_CHANNEL_NAME_LENGTH 20

typedef struct {
    char name[MAX_CHANNEL_NAME_LENGTH];
    bool states[NUM_TIME_UNITS];
} channel_t;

typedef struct {
    char name[MAX_PROGRAM_NAME_LENGTH];
    channel_t channels[NUM_CHANNELS];
} program_t;

typedef struct {
    program_t programs[NUM_PROGRAMS];
} model_t;

typedef model_t mut_model_t;

void model_init(mut_model_t *model);

#endif
