#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <cstdio>

#include <pico/stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include <semphr.h>

#include <RadioLib.h>
#include "hal/RPiPico/PicoHal.h"

#include "pb_encode.h"
#include "pb_decode.h"
#include "telemetry.pb.h"

#include "tasks/radio_tasks.h"

using namespace std;


#define START_BYTE 0xAA
#define TELEMETRY_TYPE 0x01
#define COMMAND_TYPE 0x02

#define SPI_PORT spi1
#define SPI_MISO 12
#define SPI_MOSI 11
#define SPI_SCK 10

#define RFM_NSS 3
#define RFM_RST 15
#define RFM_DIO1 20
#define RFM_DIO2 2

PicoHal *hal = new PicoHal(SPI_PORT, SPI_MISO, SPI_MOSI, SPI_SCK);
SX1262 radio = new Module(hal, RFM_NSS, RFM_DIO1, RFM_RST, RFM_DIO2);

SemaphoreHandle_t xinitSemaphore;
SemaphoreHandle_t xPacketSemaphore;
SemaphoreHandle_t xRadioMutex;

volatile uint8_t loraMode;

void setFlag() {
    if (loraMode == 0) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xPacketSemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

uint8_t calculateChecksum(const uint8_t *buffer, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^= buffer[i];
    }
    return checksum;
}

void initRadio() {
    xRadioMutex = xSemaphoreCreateMutex();
    xPacketSemaphore = xSemaphoreCreateBinary();
    xinitSemaphore = xSemaphoreCreateBinary();
}

void initRadioTask(void *pvParameters) {
    printf("[Radio] Initializing Radio...\n");
    int state = radio.begin(902.5, 125.0, 8, 5, 0x36, 22, 14);
    printf("[Radio] Radio Initialized...\n");
    if (state != RADIOLIB_ERR_NONE) {
        printf("[Radio] Initialization Failed, code %d\n", state);
        return;
    }
    printf("[Radio] Initialization Successful\n");

    radio.setPacketReceivedAction(setFlag);

    printf("[Radio] Mutex Config Successful\n");

    printf("[Radio] Starting tasks\n");

    xTaskCreate(commandRadio, "commandRadio", 8192, NULL, 2, NULL);
    xTaskCreate(telemetryRadio, "telemetryRadio", 8192, NULL, 1, NULL);

    printf("[Radio] Tasks started\n");

    vTaskDelete(NULL);
}

void telemetryRadio(void *pvParameters) {
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(500);
    xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        if (xSemaphoreTake(xRadioMutex, portMAX_DELAY) == pdTRUE) {
            loraMode = 1;

            Telemetry telemetry = Telemetry_init_zero;
            telemetry.temperature = 23.5;
            telemetry.humidity = 50.0;

            uint8_t buffer[64];
            pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
            pb_encode(&stream, Telemetry_fields, &telemetry);
            size_t message_size = stream.bytes_written;

            uint8_t frame[3 + message_size + 1]; // start byte + length + type + payload + checksum
            frame[0] = START_BYTE;
            frame[1] = message_size;
            frame[2] = TELEMETRY_TYPE;
            memcpy(&frame[3], buffer, message_size);
            frame[3 + message_size] = calculateChecksum(buffer, message_size);

            printf("[Radio] Transmitting packet ... ");
            int state = radio.transmit(frame, sizeof(frame));
            if (state == RADIOLIB_ERR_NONE) {
                printf("success!\n");
                gpio_put(PICO_DEFAULT_LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(15));
                gpio_put(PICO_DEFAULT_LED_PIN, 0);

                loraMode = 0;
            } else {
                printf("failed, code %d\n", state);
            }
            xSemaphoreGive(xRadioMutex);

            vTaskDelayUntil(&xLastWakeTime, xFrequency);
        }
    }
}


void commandRadio(void *pvParameters) {
    printf("[Radio] Starting listener...\n");
    int state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        printf("[Radio] Listening Failed, code %d\n", state);
        return;
    }

    for (;;) {
        if (xSemaphoreTake(xPacketSemaphore, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(xRadioMutex, portMAX_DELAY) == pdTRUE) {
                gpio_put(PICO_DEFAULT_LED_PIN, 1);
                vTaskDelay(pdMS_TO_TICKS(15));
                gpio_put(PICO_DEFAULT_LED_PIN, 0);

                const size_t len = 80;
                uint8_t data[len];

                // Attempt to read data from the LoRa radio
                int16_t state = radio.readData(data, len);

                if (state == RADIOLIB_ERR_NONE) {
                    printf("[SX1262] Received data!\n");

                    // Validate the start byte
                    if (data[0] != START_BYTE) {
                        printf("Invalid start byte\n");
                    }

                    // Extract length, type, and checksum from the received frame
                    uint8_t length = data[1];
                    uint8_t type = data[2];
                    uint8_t payload[length];
                    memcpy(payload, &data[3], length);
                    uint8_t received_checksum = data[3 + length];

                    // Verify checksum
                    if (received_checksum != calculateChecksum(payload, length)) {
                        printf("Checksum validation failed\n");
                    }

                    // Decode payload if the message type matches
                    if (type == COMMAND_TYPE) {
                        Command command = Command_init_zero;
                        pb_istream_t stream = pb_istream_from_buffer(payload, length);

                        // Decode the command message using Nanopb
                        if (pb_decode(&stream, Command_fields, &command)) {
                            printf("Command received:\n");
                            printf("LED On: %s\n", command.test ? "True" : "False");
                            // Add additional actions based on command data as needed
                            //                            if (command.test) {
                            //                                gpio_put(PICO_DEFAULT_LED_PIN, 1);
                            //                            } else {
                            //                                gpio_put(PICO_DEFAULT_LED_PIN, 0);
                            //                            }
                        } else {
                            printf("Failed to decode Protobuf message\n");
                        }
                    }

                    // Print LoRa metadata
                    printf("[Radio] RSSI:\t\t%.2f dBm\n", radio.getRSSI());
                    printf("[Radio] SNR:\t\t%.2f dB\n", radio.getSNR());
                    printf("[Radio] FreqErr:\t%.2f Hz\n", radio.getFrequencyError());
                } else {
                    printf("Failed to read data, code %d\n", state);
                }

                xSemaphoreGive(xRadioMutex);
            }
        }
    }
}
