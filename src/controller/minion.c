#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "minion.h"
#include "log.h"
#include "bsp/rs232.h"
#include "model/model.h"
#include "services/socketq.h"
#include "pthread.h"

#define LIGHTMODBUS_MASTER_FULL
#define LIGHTMODBUS_DEBUG
#define LIGHTMODBUS_IMPL
#include "lightmodbus/lightmodbus.h"


#define MODBUS_RESPONSE_03_LEN(data_len)   (5 + data_len * 2)
#define MODBUS_RESPONSE_05_LEN             8
#define MODBUS_REQUEST_MESSAGE_QUEUE_SIZE  8
#define MODBUS_RESPONSE_MESSAGE_QUEUE_SIZE 8
#define MODBUS_TIMEOUT                     40
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

#define REQUEST_SOCKET_PATH  "/tmp/application_minion_request_socket"
#define RESPONSE_SOCKET_PATH "/tmp/application_minion_response_socket"


typedef enum {
    TASK_MESSAGE_TAG_SYNC,
    TASK_MESSAGE_TAG_RETRY_COMMUNICATION,
} task_message_tag_t;


struct __attribute__((packed)) task_message {
    task_message_tag_t tag;

    union {
        struct {
            uint8_t  test_on;
            uint16_t outputs;
            uint16_t pwm;
        } sync;
    } as;
};


typedef struct {
    uint16_t start;
    void    *pointer;
} master_context_t;


static void       *minion_task(void *args);
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


static socketq_t messageq  = {0};
static socketq_t responseq = {0};


void minion_init(void) {
    socketq_init(&messageq, REQUEST_SOCKET_PATH, sizeof(struct task_message));
    socketq_init(&responseq, RESPONSE_SOCKET_PATH, sizeof(minion_response_t));

    pthread_t id;
    pthread_create(&id, NULL, minion_task, NULL);
    pthread_detach(id);
}


void minion_retry_communication(void) {
    struct task_message msg = {.tag = TASK_MESSAGE_TAG_RETRY_COMMUNICATION};
    socketq_send(&messageq, (uint8_t *)&msg);
}


void minion_sync(model_t *model) {
    struct task_message msg = {
        .tag = TASK_MESSAGE_TAG_SYNC,
        .as =
            {
                .sync =
                    {
                        .test_on = model->run.minion.write.test_on,
                        .outputs = model->run.minion.write.outputs,
                        .pwm     = model->run.minion.write.pwm,
                    },
            },
    };
    socketq_send(&messageq, (uint8_t *)&msg);
}


uint8_t minion_get_response(minion_response_t *response) {
    return socketq_receive_nonblock(&responseq, (uint8_t *)response, 0);
}


static void *minion_task(void *args) {
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

    log_info("Task starting");

    for (;;) {
        if (socketq_receive(&messageq, (uint8_t *)&message)) {
            if (communication_error && 0) {
                if (message.tag == TASK_MESSAGE_TAG_RETRY_COMMUNICATION) {
                    communication_error = handle_message(&master, last_unsuccessful_message);
                    usleep((MODBUS_TIMEOUT / 2) * 1000);
                }
            } else {
                last_unsuccessful_message = message;
                communication_error       = handle_message(&master, message);
                usleep((MODBUS_TIMEOUT / 2) * 1000);
            }
        }
    }

    pthread_exit(NULL);
    return NULL;
}


uint8_t handle_message(ModbusMaster *master, struct task_message message) {
    uint8_t           error    = 0;
    minion_response_t response = {};

    switch (message.tag) {
        case TASK_MESSAGE_TAG_SYNC: {
            response.tag = MINION_RESPONSE_TAG_SYNC;

            uint16_t values[6] = {0};
            if (read_input_registers(master, values, MINION_ADDR, MODBUS_IR_FIRMWARE_VERSION_MAJOR,
                                     sizeof(values) / sizeof(values[0]))) {
                error = 1;
            } else {
                response.as.sync.firmware_version_major = (values[0] >> 11) & 0x1F;
                response.as.sync.firmware_version_minor = (values[0] >> 6) & 0x1F;
                response.as.sync.firmware_version_patch = (values[0] >> 0) & 0x3F;
                response.as.sync.inputs                 = values[1];
                response.as.sync.v0_10_adc              = values[2];
                response.as.sync.ma4_20_adc             = values[4];
            }

            if (!error) {
                uint16_t values[3] = {
                    message.as.sync.test_on,
                    message.as.sync.outputs,
                    message.as.sync.pwm,
                };

                if (write_holding_registers(master, MINION_ADDR, MODBUS_HR_TEST_MODE, values,
                                            sizeof(values) / sizeof(values[0]))) {
                    error = 1;
                }
            }

            if (error) {
                response.tag = MINION_RESPONSE_TAG_ERROR;
            }

            socketq_send(&responseq, (uint8_t *)&response);
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
    log_info("Received exception (function %d) from slave %d code %d", function, address, code);

    return MODBUS_OK;
}


static int write_holding_registers(ModbusMaster *master, uint8_t address, uint16_t starting_address, uint16_t *data,
                                   size_t num) {
    uint8_t buffer[MODBUS_MAX_PACKET_SIZE] = {0};
    int     res                            = 0;
    size_t  counter                        = 0;

    bsp_rs232_flush();

    do {
        res                 = 0;
        ModbusErrorInfo err = modbusBuildRequest16RTU(master, address, starting_address, num, data);
        assert(modbusIsOk(err));
        bsp_rs232_write((uint8_t *)modbusMasterGetRequest(master), modbusMasterGetRequestLength(master));

        int len = bsp_rs232_read(buffer, sizeof(buffer));
        err     = modbusParseResponseRTU(master, modbusMasterGetRequest(master), modbusMasterGetRequestLength(master),
                                         buffer, len);

        if (!modbusIsOk(err)) {
            log_warn("Write holding registers for %i error (%i): %i %i", address, len, err.source, err.error);
            res = 1;
            usleep(MODBUS_TIMEOUT * 1000);
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    if (res) {
        log_warn("ERROR!");
    } else {
        log_debug("Success");
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

        int len = bsp_rs232_read(buffer, sizeof(buffer));
        err     = modbusParseResponseRTU(master, modbusMasterGetRequest(master), modbusMasterGetRequestLength(master),
                                         buffer, len);

        if (!modbusIsOk(err)) {
            log_warn("Read holding registers for %i error (%i): %i %i", address, len, err.source, err.error);
            res = 1;
            usleep(MODBUS_TIMEOUT * 1000);
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

        int len = bsp_rs232_read(buffer, sizeof(buffer));
        err     = modbusParseResponseRTU(master, modbusMasterGetRequest(master), modbusMasterGetRequestLength(master),
                                         buffer, len);

        if (!modbusIsOk(err)) {
            log_warn("Read input registers for %i error (%i): %i %i", address, len, err.source, err.error);
            res = 1;
            usleep((MODBUS_TIMEOUT / 2) * 1000);
        }
    } while (res && ++counter < MODBUS_COMMUNICATION_ATTEMPTS);

    if (res) {
        log_warn("ERROR!");
    } else {
        log_debug("Success");
    }

    return res;
}
