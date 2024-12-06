#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED


#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "program.h"


#define NUM_PROGRAMS 20
#define NUM_INPUTS   12
#define NUM_OUTPUTS  16


typedef enum {
    WIFI_CONNECTED,
    WIFI_CONNECTING,
    WIFI_SCANNING,
    WIFI_INACTIVE,
} wifi_status_t;


typedef struct {
    int  signal;
    char ssid[33];
} wifi_network_t;


// collection of all models
typedef struct {
    struct {
        name_t    channel_names[PROGRAM_NUM_CHANNELS];
        program_t programs[NUM_PROGRAMS];
    } config;

    struct {
        char            wifi_ipaddr[16];
        char            eth_ipaddr[16];
        char            ssid[33];
        wifi_status_t   net_status;
        size_t          num_networks;
        wifi_network_t *networks;
        int             connected;

        // size_t  num_drive_machines;
        // name_t *drive_machines;
        uint8_t drive_mounted;
        uint8_t firmware_update_ready;
    } system;

    struct {
        struct {
            struct {
                uint16_t firmware_version_major;
                uint16_t firmware_version_minor;
                uint16_t firmware_version_patch;
                uint16_t inputs;
                uint16_t ma4_20_adc;
                uint16_t v0_10_adc;
            } read;

            struct {
                uint8_t  test_on;
                uint16_t outputs;
                uint16_t pwm;
            } write;
        } minion;
    } run;
} mut_model_t;

typedef const mut_model_t model_t;

void model_init(mut_model_t *model);

size_t           model_get_num_programs(model_t *model);
const char      *model_get_program_name(model_t *model, size_t num);
const program_t *model_get_program(model_t *model, size_t num);
program_t       *model_get_program_mut(mut_model_t *model, size_t num);
void             model_clear_test_outputs(mut_model_t *model);
void             model_set_test_output(mut_model_t *model, uint16_t output_index);

#endif
