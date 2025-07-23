#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include "model/model.h"
#include "app_config.h"


void command_line_parse_args(mut_model_t *model, int argc, char *argv[]) {
    int flag_version = 0;

    struct option long_options[] = {
        /* These options set a flag. */
        {"version", no_argument, &flag_version, 1},
        {0, 0, 0, 0},
    };
    int opt;

    while ((opt = getopt_long(argc, argv, "cmu:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                model->args.flag_crash = 1;
                break;

            case 'u':
                model->args.new_firmware_path = malloc(strlen(optarg) + 1);
                if (model->args.new_firmware_path) {
                    snprintf(model->args.new_firmware_path, strlen(optarg) + 1, optarg);
                    model->args.flag_firmware_update = 1;
                }
                break;

            default:
                break;
        }
    }

    if (flag_version) {
        printf("%s %s\n", SOFTWARE_VERSION, SOFTWARE_BUILD_DATE);
        exit(0);
    }
}
