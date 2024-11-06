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

    const char* elapsedTimeStr = nullptr;

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