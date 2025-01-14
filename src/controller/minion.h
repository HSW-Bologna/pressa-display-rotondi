#ifndef MINION_H_INCLUDED
#define MINION_H_INCLUDED


#include <stdint.h>
#include "model/model.h"


typedef enum {
    MINION_RESPONSE_TAG_ERROR,
    MINION_RESPONSE_TAG_SYNC,
} minion_response_tag_t;


typedef struct {
    minion_response_tag_t tag;
    union {
        struct {
            uint8_t  firmware_version_major;
            uint8_t  firmware_version_minor;
            uint8_t  firmware_version_patch;
            uint16_t inputs;
            uint16_t v0_10_adc;
            uint16_t ma4_20_adc;
            uint8_t  running;
            uint16_t elapsed_time_ms;
        } sync;
    } as;
} minion_response_t;


void    minion_init(void);
void    minion_sync(model_t *model);
uint8_t minion_get_response(minion_response_t *response);
void    minion_retry_communication(void);


#endif
