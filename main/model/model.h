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
    OTA_STATE_NONE = 0,
    OTA_STATE_IN_PROGRESS,
    OTA_STATE_DONE,
} ota_state_t;

typedef enum {
    MACHINE_MODEL_TRADITIONAL = 0,
    MACHINE_MODEL_DOUBLE_FRONT,
} machine_model_t;

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

typedef struct {
    name_t    channel_names[PROGRAM_NUM_CHANNELS];
    program_t programs[NUM_PROGRAMS];
    uint16_t  ma4_20_offset;
    uint16_t  headgap_offset_up;
    uint16_t  headgap_offset_down;
    uint16_t  machine_model;
    uint16_t  position_sensor_scale_mm;

    program_digital_channel_schedule_t digital_channels[PROGRAM_NUM_CHANNELS];
} configuration_t;

// collection of all models
typedef struct {
    configuration_t config;

    struct {
        char            wifi_ipaddr[16];
        char            eth_ipaddr[16];
        char            ssid[33];
        wifi_status_t   net_status;
        size_t          num_networks;
        wifi_network_t *networks;
        int             connected;
    } network;

    struct {
        struct {
            uint8_t communication_error;
            uint8_t communication_enabled;

            struct {
                uint16_t firmware_version_major;
                uint16_t firmware_version_minor;
                uint16_t firmware_version_patch;
                uint16_t inputs;
                uint16_t ma4_adc;
                uint16_t ma20_adc;
                uint16_t ma4_20_adc;
                uint16_t v0_10_adc;
                uint8_t  running;
                uint16_t elapsed_milliseconds;
            } read;

            struct {
                uint8_t  test_on;
                uint16_t outputs;
                uint16_t pwm;
            } write;
        } minion;

        int16_t current_program_index;
        uint8_t drive_mounted;
        uint8_t firmware_update_ready;

        size_t num_importable_configurations;
        char **importable_configurations;

        uint8_t network_connected;
    } run;

    struct {
        int   flag_crash;
        int   flag_firmware_update;
        char *new_firmware_path;
    } args;
} mut_model_t;

typedef const mut_model_t model_t;

void model_init(mut_model_t *model);

size_t           model_get_num_programs(model_t *model);
const char      *model_get_program_name(model_t *model, size_t num);
const program_t *model_get_program(model_t *model, size_t num);
program_t       *model_get_program_mut(mut_model_t *model, size_t num);
void             model_clear_test_outputs(mut_model_t *model);
void             model_set_test_output(mut_model_t *model, uint16_t output_index);
void             model_check_parameters(mut_model_t *model);
uint8_t          model_is_program_ready(model_t *model);
const program_t *model_get_current_program(model_t *model);
void             model_clear_current_program(mut_model_t *model);
void             model_set_current_program(mut_model_t *model, uint16_t current_program_index);
uint8_t          model_is_communication_ok(model_t *model);
void             model_copy_program(mut_model_t *model, uint16_t source_index, uint16_t destination_index);
uint16_t         model_get_uncalibrated_position_mm(model_t *model);
uint16_t         model_get_calibrated_position_mm(model_t *model);
uint16_t         model_position_mm_to_adc(model_t *model, uint16_t mm);
uint16_t         model_get_current_position_target(model_t *model);
void             model_reset_program(mut_model_t *model, uint16_t program_index);

#endif
