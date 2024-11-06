#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"

#include <pico/stdlib.h>

#include <RadioLib.h>

#include "hal/RPiPico/PicoHal.h"

#include "tasks/telemetry_radio.h"

#include "pb_encode.h"
#include "pb_decode.h"
#include "telemetry.pb.h"  // Generated from protobuf

#include <semphr.h>

using namespace std;

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

#define START_BYTE 0xAA
#define TELEMETRY_TYPE 0x01
#define COMMAND_TYPE 0x02
#define UART_ID uart0
#define BAUD_RATE 115200
#define TX_PIN PICO_DEFAULT_UART_TX_PIN
#define RX_PIN PICO_DEFAULT_UART_RX_PIN

SemaphoreHandle_t xPacketSemaphore;

uint8_t calculateChecksum(const uint8_t *buffer, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^= buffer[i];
    }
    return checksum;
}

void telemetryRadio(void *pvParameters) {
    printf("[SX1262] Initializing ... ");
    int state = radio.begin(902.5, 125.0, 8, 5, 0x36, 22, 14);
    if (state != RADIOLIB_ERR_NONE) {
        printf("failed, code %d\n", state);
        return;
    }
    printf("success!\n");

    const TickType_t ticksPerSecond = configTICK_RATE_HZ;
    TickType_t startTicks = xTaskGetTickCount();

    const char *elapsedTimeStr = nullptr;

    for (;;) {
        TickType_t currentTicks = xTaskGetTickCount();

        TickType_t elapsedTicks = currentTicks - startTicks;
        double elapsedTimeInSeconds = static_cast<double>(elapsedTicks) / ticksPerSecond;

        static char buffer[50];
        snprintf(buffer, sizeof(buffer), "Time running: %.2f seconds", elapsedTimeInSeconds);

        elapsedTimeStr = buffer;

        printf("[SX1276] Transmitting packet ... ");
        state = radio.transmit(elapsedTimeStr);
        if (state == RADIOLIB_ERR_NONE) {
            printf("success!\n");
            hal->delay(500);
        } else {
            printf("failed, code %d\n", state);
            return;
        }
    }
}

void setFlag(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Give the semaphore from ISR to unblock the task
    xSemaphoreGiveFromISR(xPacketSemaphore, &xHigherPriorityTaskWoken);

    // Perform a context switch if needed
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void commandRadio(void *pvParameters) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    printf("[SX1262] Initializing ... \n");
    int state = radio.begin(902.5, 125.0, 8, 5, 0x36, 22, 14);
    if (state != RADIOLIB_ERR_NONE) {
        printf("Initialization failed, code %d\n", state);
        return;
    }
    printf("Initialization success!\n");

    printf("[SX1262] Starting to listen ... ");
    state = radio.startReceive();
    if (state != RADIOLIB_ERR_NONE) {
        printf("Listening failed, code %d\n", state);
        return;
    }
    printf("Listening success!\n");

    radio.setPacketReceivedAction(setFlag);

    xPacketSemaphore = xSemaphoreCreateBinary(); //todo this should be moved to the setup function and passed here by the linker

    printf("Semaphore IRQ called!\n");

    for (;;) {

        if (xSemaphoreTake(xPacketSemaphore, portMAX_DELAY) == pdTRUE) {
            const size_t len = 80;
            uint8_t data[len];

            // Attempt to read data from the LoRa radio
            int16_t state = radio.readData(data, len);

            if (state == RADIOLIB_ERR_NONE) {
                printf("[SX1262] Received data!\n");

                // Validate the start byte
                if (data[0] != START_BYTE) {
                    printf("Invalid start byte\n");
                    continue;
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
                    continue;
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
                        if(command.test){
                            gpio_put(PICO_DEFAULT_LED_PIN, 1);
                        }else{
                            gpio_put(PICO_DEFAULT_LED_PIN, 0);
                        }
                    } else {
                        printf("Failed to decode Protobuf message\n");
                    }
                }

                // Print LoRa metadata
                printf("[SX1262] RSSI:\t\t%.2f dBm\n", radio.getRSSI());
                printf("[SX1262] SNR:\t\t%.2f dB\n", radio.getSNR());
                printf("[SX1262] FreqErr:\t%.2f Hz\n", radio.getFrequencyError());

            } else {
                printf("Failed to read data, code %d\n", state);
            }

            // Delay between reads
//            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}
