#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED

#include <stdbool.h>

#define NUM_PROGRAMS 10
#define NUM_CHANNELS 15
#define NUM_TIME_UNITS 25
#define STRING_NAME_SIZE 20

typedef enum {
    WIFI_CONNECTED,
    WIFI_CONNECTING,
    WIFI_SCANNING,
    WIFI_INACTIVE,
} wifi_status_t;


typedef char name_t[STRING_NAME_SIZE];

typedef struct {
    int  signal;
    char ssid[STRING_NAME_SIZE];
} wifi_network_t;


typedef struct {
    name_t ch_name;
    bool states[NUM_TIME_UNITS];
} channel_t;

typedef struct {
    name_t prog_name;
    channel_t channels[NUM_CHANNELS];
} program_t;


// collection of all models
typedef struct {
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

} model_t;

typedef model_t mut_model_t;

void model_init(mut_model_t *model);

size_t model_get_num_programs(model_t *pmodel);
const char *model_get_program_name(model_t *pmodel, size_t num);
program_t *model_get_program(model_t *pmodel, size_t num);

#endif
