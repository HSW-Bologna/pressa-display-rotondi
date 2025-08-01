#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "minion.h"
#include <esp_log.h>
#include "bsp/rs232.h"
#include "model/model.h"
#include "services/timestamp.h"

#define LIGHTMODBUS_MASTER_FULL
#define LIGHTMODBUS_DEBUG
#define LIGHTMODBUS_IMPL
#include "lightmodbus/lightmodbus.h"


#define MODBUS_RESPONSE_04_LEN(data_len)   (5 + data_len * 2)
#define MODBUS_RESPONSE_16_LEN             8
#define MODBUS_REQUEST_MESSAGE_QUEUE_SIZE  8
#define MODBUS_RESPONSE_MESSAGE_QUEUE_SIZE 4
#define MODBUS_TIMEOUT                     30
#define MODBUS_MAX_PACKET_SIZE             256
#define MODBUS_COMMUNICATION_ATTEMPTS      3

#define MODBUS_IR_FIRMWARE_VERSION_MAJOR 0
#define MODBUS_IR_CYCLE_STATE            9

#define MODBUS_HR_TEST_MODE      0
#define MODBUS_HR_PROGRAM_NUMBER 11

#define MINION_ADDR 1

#define COMMAND_REGISTER_NONE         0
#define COMMAND_REGISTER_RESUME       1
#define COMMAND_REGISTER_PAUSE        2
#define COMMAND_REGISTER_DONE         3
#define COMMAND_REGISTER_CLEAR_ALARMS 4


typedef enum {
    TASK_MESSAGE_TAG_SYNC,
    TASK_MESSAGE_TAG_RETRY_COMMUNICATION,
} task_message_tag_t;


struct __attribute__((packed)) task_message {
    task_message_tag_t tag;

    union {
        struct {
            uint8_t                            test_on;
            machine_model_t                    machine_model;
            uint16_t                           outputs;
            uint16_t                           pwm;
            uint16_t                           headgap_offset_up;
            uint16_t                           headgap_offset_down;
            program_digital_channel_schedule_t digital_channels[PROGRAM_NUM_CHANNELS];
            program_pressure_channel_state_t   dac_channel[PROGRAM_NUM_TIME_UNITS];
            program_sensor_channel_threshold_t sensor_channel[PROGRAM_NUM_TIME_UNITS];
            uint16_t                           time_unit_decisecs;
            uint16_t                           dac_levels[PROGRAM_PRESSURE_LEVELS];
            uint16_t                           adc_levels[PROGRAM_SENSOR_LEVELS];
        } sync;
    } as;
};


typedef struct {
    uint16_t start;
    void    *pointer;
} master_context_t;


static void        minion_task(void *args);
uint8_t            handle_message(ModbusMaster *master, struct task_message message);
static ModbusError exception_callback(const ModbusMaster *master, uint8_t address, uint8_t function,
                                      ModbusExceptionCode code);
static ModbusError data_callback(const ModbusMaster *master, const ModbusDataCallbackArgs *args);
static int write_holding_registers(ModbusMaster *master, uint8_t address, uint16_t starting_address, uint16_t *data,
                                   size_t num);
static int read_holding_registers(ModbusMaster *master, uint16_t *registers, uint8_t address, uint16_t start,
                                  uint16_t count);
static int read_input_registers(ModbusMaster *master, uint16_t *registers, uint8_t address, uint16_t start,
                                uint16_t count);


static const char   *TAG = __FILE_NAME__;
static QueueHandle_t requestq;
static QueueHandle_t responseq;


void minion_init(void) {
    {
        static StaticQueue_t static_queue;
        static uint8_t       queue_buffer[sizeof(struct task_message) * 8] = {0};
        requestq = xQueueCreateStatic(sizeof(queue_buffer) / sizeof(struct task_message), sizeof(struct task_message),
                                      queue_buffer, &static_queue);
    }
    {
        static StaticQueue_t static_queue;
        static uint8_t       queue_buffer[sizeof(minion_response_t) * 8] = {0};
        responseq = xQueueCreateStatic(sizeof(queue_buffer) / sizeof(minion_response_t), sizeof(minion_response_t),
                                       queue_buffer, &static_queue);
    }

    {
        static StackType_t task_stack[512 * 8] = {0};
#ifdef BUILD_CONFIG_SIMULATED_APP
        xTaskCreate(minion_task, TAG, sizeof(task_stack), NULL, 5, NULL);
#else
        static StaticTask_t static_task;
        xTaskCreateStatic(minion_task, TAG, sizeof(task_stack), NULL, 5, task_stack, &static_task);
#endif
    }
}


void minion_retry_communication(void) {
    struct task_message msg = {.tag = TASK_MESSAGE_TAG_RETRY_COMMUNICATION};
    xQueueSend(requestq, (uint8_t *)&msg, pdMS_TO_TICKS(10));
}


void minion_sync(model_t *model) {
    const program_t *program = model_get_current_program(model);

    struct task_message msg = {
        .tag = TASK_MESSAGE_TAG_SYNC,
        .as =
            {
                .sync =
                    {
                        .test_on             = model->run.minion.write.test_on,
                        .outputs             = model->run.minion.write.outputs,
                        .pwm                 = model->run.minion.write.pwm,
                        .machine_model       = model->config.machine_model,
                        .headgap_offset_up   = model_position_mm_to_adc(model, model->config.headgap_offset_up),
                        .headgap_offset_down = model_position_mm_to_adc(model, model->config.headgap_offset_down),
                        .time_unit_decisecs  = program->time_unit_decisecs,
                    },
            },
    };

    for (size_t i = 0; i < PROGRAM_NUM_PROGRAMMABLE_CHANNELS; i++) {
        msg.as.sync.digital_channels[i] = program->digital_channels[i];
    }
    // Last channel is always active during the cycle
    msg.as.sync.digital_channels[PROGRAM_NUM_PROGRAMMABLE_CHANNELS] = 0xFFFFFFFF;

    memcpy(&msg.as.sync.dac_channel, &program->pressure_channel, sizeof(program->pressure_channel));
    memcpy(&msg.as.sync.sensor_channel, &program->sensor_channel, sizeof(program->sensor_channel));
    for (size_t i = 0; i < PROGRAM_PRESSURE_LEVELS; i++) {
        msg.as.sync.dac_levels[i] = (program->pressure_levels[i] * 100) / 60;
    }

    for (size_t i = 0; i < PROGRAM_SENSOR_LEVELS; i++) {
        msg.as.sync.adc_levels[i] =
            model->config.ma4_20_offset + model_position_mm_to_adc(model, program->position_levels[i]);
    }

    xQueueSend(requestq, (uint8_t *)&msg, pdMS_TO_TICKS(10));
}


uint8_t minion_get_response(minion_response_t *response) {
    return xQueueReceive(responseq, (uint8_t *)response, 0);
}


static void minion_task(void *args) {
    (void)args;
    ModbusMaster    master;
    ModbusErrorInfo err = modbusMasterInit(&master,
                                           data_callback,              // Callback for handling incoming data
                                           exception_callback,         // Exception callback (optional)
                                           modbusDefaultAllocator,     // Memory allocator used to allocate request
                                           modbusMasterDefaultFunctions,        // Set of supported functions
                                           modbusMasterDefaultFunctionCount     // Number of supported functions
    );

    // Check for errors
    assert(modbusIsOk(err) && "modbusMasterInit() failed");
    struct task_message last_unsuccessful_message = {0};
    struct task_message message                   = {0};
    uint8_t             communication_error       = 0;

    ESP_LOGI(TAG, "Task starting");

    for (;;) {
        if (xQueueReceive(requestq, (uint8_t *)&message, portMAX_DELAY)) {
            if (communication_error && 0) {
                if (message.tag == TASK_MESSAGE_TAG_RETRY_COMMUNICATION) {
                    communication_error = handle_message(&master, last_unsuccessful_message);
                    vTaskDelay(pdMS_TO_TICKS((MODBUS_TIMEOUT / 2) * 1000));
                }
            } else {
                last_unsuccessful_message = message;
                communication_error       = handle_message(&master, message);
                vTaskDelay(pdMS_TO_TICKS((MODBUS_TIMEOUT / 2) * 1000));
            }
        }
    }

    vTaskDelete(NULL);
}


uint8_t handle_message(ModbusMaster *master, struct task_message message) {
    uint8_t           error    = 0;
    minion_response_t response = {};

    switch (message.tag) {
        case TASK_MESSAGE_TAG_SYNC: {
            response.tag = MINION_RESPONSE_TAG_SYNC;

            uint16_t values[10] = {0};
            if (read_input_registers(master, values, MINION_ADDR, MODBUS_IR_FIRMWARE_VERSION_MAJOR,
                                     sizeof(values) / sizeof(values[0]))) {
                error = 1;
            } else {
                response.as.sync.firmware_version_major = (values[0] >> 11) & 0x1F;
                response.as.sync.firmware_version_minor = (values[0] >> 6) & 0x1F;
                response.as.sync.firmware_version_patch = (values[0] >> 0) & 0x3F;
                response.as.sync.inputs                 = values[1];
                response.as.sync.v0_10_adc              = values[2];
                response.as.sync.ma4_adc                = values[4];
                response.as.sync.ma20_adc               = values[5];
                response.as.sync.ma4_20_adc             = values[6];
                response.as.sync.running                = values[8];
                response.as.sync.elapsed_time_ms        = values[9];
            }

            if (!error) {
                uint16_t values[57] = {
                    message.as.sync.test_on,
                    message.as.sync.outputs,
                    message.as.sync.pwm,
                    message.as.sync.machine_model,
                    message.as.sync.headgap_offset_up,
                    message.as.sync.headgap_offset_down,
                    message.as.sync.time_unit_decisecs,
                    message.as.sync.dac_levels[0],
                    message.as.sync.dac_levels[1],
                    message.as.sync.dac_levels[2],
                    message.as.sync.adc_levels[0],
                    message.as.sync.adc_levels[1],
                    message.as.sync.adc_levels[2],
                    (message.as.sync.digital_channels[0] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[0] & 0xFFFF,
                    (message.as.sync.digital_channels[1] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[1] & 0xFFFF,
                    (message.as.sync.digital_channels[2] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[2] & 0xFFFF,
                    (message.as.sync.digital_channels[3] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[3] & 0xFFFF,
                    (message.as.sync.digital_channels[4] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[4] & 0xFFFF,
                    (message.as.sync.digital_channels[5] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[5] & 0xFFFF,
                    (message.as.sync.digital_channels[6] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[6] & 0xFFFF,
                    (message.as.sync.digital_channels[7] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[7] & 0xFFFF,
                    (message.as.sync.digital_channels[8] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[8] & 0xFFFF,
                    (message.as.sync.digital_channels[9] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[9] & 0xFFFF,
                    (message.as.sync.digital_channels[10] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[10] & 0xFFFF,
                    (message.as.sync.digital_channels[11] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[11] & 0xFFFF,
                    (message.as.sync.digital_channels[12] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[12] & 0xFFFF,
                    (message.as.sync.digital_channels[13] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[13] & 0xFFFF,
                    (message.as.sync.digital_channels[14] >> 16) & 0xFFFF,
                    message.as.sync.digital_channels[14] & 0xFFFF,
                    ((message.as.sync.dac_channel[0] & 0xF) << 12) | ((message.as.sync.dac_channel[1] & 0xF) << 8) |
                        ((message.as.sync.dac_channel[2] & 0xF) << 4) | (message.as.sync.dac_channel[3] & 0xF),
                    ((message.as.sync.dac_channel[4] & 0xF) << 12) | ((message.as.sync.dac_channel[5] & 0xF) << 8) |
                        ((message.as.sync.dac_channel[6] & 0xF) << 4) | (message.as.sync.dac_channel[7] & 0xF),
                    ((message.as.sync.dac_channel[8] & 0xF) << 12) | ((message.as.sync.dac_channel[9] & 0xF) << 8) |
                        ((message.as.sync.dac_channel[10] & 0xF) << 4) | (message.as.sync.dac_channel[11] & 0xF),
                    ((message.as.sync.dac_channel[12] & 0xF) << 12) | ((message.as.sync.dac_channel[13] & 0xF) << 8) |
                        ((message.as.sync.dac_channel[14] & 0xF) << 4) | (message.as.sync.dac_channel[15] & 0xF),
                    ((message.as.sync.dac_channel[16] & 0xF) << 12) | ((message.as.sync.dac_channel[17] & 0xF) << 8) |
                        ((message.as.sync.dac_channel[18] & 0xF) << 4) | (message.as.sync.dac_channel[19] & 0xF),
                    ((message.as.sync.dac_channel[20] & 0xF) << 12) | ((message.as.sync.dac_channel[21] & 0xF) << 8) |
                        ((message.as.sync.dac_channel[22] & 0xF) << 4) | (message.as.sync.dac_channel[23] & 0xF),
                    message.as.sync.dac_channel[24],
                    ((message.as.sync.sensor_channel[0] & 0xF) << 12) |
                        ((message.as.sync.sensor_channel[1] & 0xF) << 8) |
                        ((message.as.sync.sensor_channel[2] & 0xF) << 4) | (message.as.sync.sensor_channel[3] & 0xF),
                    ((message.as.sync.sensor_channel[4] & 0xF) << 12) |
                        ((message.as.sync.sensor_channel[5] & 0xF) << 8) |
                        ((message.as.sync.sensor_channel[6] & 0xF) << 4) | (message.as.sync.sensor_channel[7] & 0xF),
                    ((message.as.sync.sensor_channel[8] & 0xF) << 12) |
                        ((message.as.sync.sensor_channel[9] & 0xF) << 8) |
                        ((message.as.sync.sensor_channel[10] & 0xF) << 4) | (message.as.sync.sensor_channel[11] & 0xF),
                    ((message.as.sync.sensor_channel[12] & 0xF) << 12) |
                        ((message.as.sync.sensor_channel[13] & 0xF) << 8) |
                        ((message.as.sync.sensor_channel[14] & 0xF) << 4) | (message.as.sync.sensor_channel[15] & 0xF),
                    ((message.as.sync.sensor_channel[16] & 0xF) << 12) |
                        ((message.as.sync.sensor_channel[17] & 0xF) << 8) |
                        ((message.as.sync.sensor_channel[18] & 0xF) << 4) | (message.as.sync.sensor_channel[19] & 0xF),
                    ((message.as.sync.sensor_channel[20] & 0xF) << 12) |
                        ((message.as.sync.sensor_channel[21] & 0xF) << 8) |
                        ((message.as.sync.sensor_channel[22] & 0xF) << 4) | (message.as.sync.sensor_channel[23] & 0xF),
                    message.as.sync.sensor_channel[24],
                };

                if (write_holding_registers(master, MINION_ADDR, MODBUS_HR_TEST_MODE, values,
                                            sizeof(values) / sizeof(values[0]))) {
                    error = 1;
                }
            }

            if (error) {
                response.tag = MINION_RESPONSE_TAG_ERROR;
            }

            xQueueSend(responseq, (uint8_t *)&response, portMAX_DELAY);
            break;
        }

        case TASK_MESSAGE_TAG_RETRY_COMMUNICATION:
            break;
    }

    return error;
}


static ModbusError data_callback(const ModbusMaster *master, const ModbusDataCallbackArgs *args) {
    master_context_t *ctx = modbusMasterGetUserPointer(master);

    if (ctx != NULL) {
        switch (args->type) {
            case MODBUS_HOLDING_REGISTER: {
                uint16_t *buffer                 = ctx->pointer;
                buffer[args->index - ctx->start] = args->value;
                break;
            }

            case MODBUS_DISCRETE_INPUT: {
                uint8_t *buffer                  = ctx->pointer;
                buffer[args->index - ctx->start] = args->value;
                break;
            }

            case MODBUS_INPUT_REGISTER: {
                uint16_t *buffer                 = ctx->pointer;
                buffer[args->index - ctx->start] = args->value;
                break;
            }

            case MODBUS_COIL: {
                uint8_t *buffer                  = ctx->pointer;
                buffer[args->index - ctx->start] = args->value;
                break;
            }
        }
    }

    return MODBUS_OK;
}


static ModbusError exception_callback(const ModbusMaster *master, uint8_t address, uint8_t function,
                                      ModbusExceptionCode code) {
    (void)master;
    ESP_LOGI(TAG, "Received exception (function %d) from slave %d code %d", function, address, code);

    return MODBUS_OK;
}


static int write_holding_registers(ModbusMaster *master, uint8_t address, uint16_t starting_address, uint16_t *data,
                                   size_t num) {
    uint8_t buffer[MODBUS_RESPONSE_16_LEN] = {0};
    int     res                            = 0;
    size_t  counter                        = 0;

    bsp_rs232_flush();

    do {
        res                 = 0;
        ModbusErrorInfo err = modbusBuildRequest16RTU(master, address, starting_address, num, data);
        assert(modbusIsOk(err));
        bsp_rs232_write((uint8_t *)modbusMasterGetRequest(master), modbusMasterGetRequestLength(master));

        int len = bsp_rs232_read(buffer, sizeof(buffer), 50);
        err     = modbusParseResponseRTU(master, modbusMasterGetRequest(master), modbusMasterGetRequestLength(master),
                                         buffer, len);

        if (!modbusIsOk(err)) {
            ESP_LOGW(TAG, "Write holding registers for %i error (%i): %i %i", address, len, err.source, err.error);
            res = 1;
            vTaskDelay(pdMS_TO_TICKS(MODBUS_TIMEOUT * 1000));
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    if (res) {
        ESP_LOGW(TAG, "ERROR!");
    } else {
        ESP_LOGD(TAG, "Success");
    }

    return res;
}


static int read_holding_registers(ModbusMaster *master, uint16_t *registers, uint8_t address, uint16_t start,
                                  uint16_t count) {
    ModbusErrorInfo err;
    int             res                            = 0;
    size_t          counter                        = 0;
    uint8_t         buffer[MODBUS_MAX_PACKET_SIZE] = {0};

    bsp_rs232_flush();

    master_context_t ctx = {.pointer = registers, .start = start};
    if (registers == NULL) {
        modbusMasterSetUserPointer(master, NULL);
    } else {
        modbusMasterSetUserPointer(master, &ctx);
    }

    do {
        res = 0;
        err = modbusBuildRequest03RTU(master, address, start, count);
        assert(modbusIsOk(err));

        bsp_rs232_write((uint8_t *)modbusMasterGetRequest(master), modbusMasterGetRequestLength(master));

        int len = bsp_rs232_read(buffer, sizeof(buffer), 50);
        err     = modbusParseResponseRTU(master, modbusMasterGetRequest(master), modbusMasterGetRequestLength(master),
                                         buffer, len);

        if (!modbusIsOk(err)) {
            ESP_LOGW(TAG, "Read holding registers for %i error (%i): %i %i", address, len, err.source, err.error);
            res = 1;
            vTaskDelay(pdMS_TO_TICKS(MODBUS_TIMEOUT * 1000));
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    return res;
}


static int read_input_registers(ModbusMaster *master, uint16_t *registers, uint8_t address, uint16_t start,
                                uint16_t count) {
    ModbusErrorInfo err;
    int             res                            = 0;
    size_t          counter                        = 0;
    uint8_t         buffer[MODBUS_MAX_PACKET_SIZE] = {0};

    master_context_t ctx = {.pointer = registers, .start = start};
    if (registers == NULL) {
        modbusMasterSetUserPointer(master, NULL);
    } else {
        modbusMasterSetUserPointer(master, &ctx);
    }

    bsp_rs232_flush();

    do {
        res = 0;
        err = modbusBuildRequest04RTU(master, address, start, count);
        assert(modbusIsOk(err));

        bsp_rs232_write((uint8_t *)modbusMasterGetRequest(master), modbusMasterGetRequestLength(master));

        int len = bsp_rs232_read(buffer, MODBUS_RESPONSE_04_LEN(count), 50);
        err     = modbusParseResponseRTU(master, modbusMasterGetRequest(master), modbusMasterGetRequestLength(master),
                                         buffer, len);

        if (!modbusIsOk(err)) {
            ESP_LOGW(TAG, "Read input registers for %i error (%i): %i %i", address, len, err.source, err.error);
            res = 1;
            vTaskDelay(pdMS_TO_TICKS((MODBUS_TIMEOUT / 2) * 1000));
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    if (res) {
        ESP_LOGW(TAG, "ERROR!");
    } else {
        ESP_LOGD(TAG, "Success");
    }

    return res;
}
