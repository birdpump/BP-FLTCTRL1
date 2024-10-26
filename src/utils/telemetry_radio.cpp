#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <cstdio>

#include <pico/stdlib.h>

#include <RadioLib.h>

#include "hal/RPiPico/PicoHal.h"

#include "utils/telemetry_radio.h"

#define SPI_PORT spi1
#define SPI_MISO 12
#define SPI_MOSI 11
#define SPI_SCK 10

#define RFM_NSS 3
#define RFM_RST 15
#define RFM_DIO1 20
#define RFM_DIO2 2


PicoHal* hal = new PicoHal(SPI_PORT, SPI_MISO, SPI_MOSI, SPI_SCK);

SX1262 radio = new Module(hal, RFM_NSS, RFM_DIO1, RFM_RST, RFM_DIO2);

void testRadio(){
    printf("[SX1262] Initializing ... ");
    int state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        printf("failed, code %d\n", state);
        return;
    }
    printf("success!\n");

    printf("[SX1276] Transmitting packet ... ");
    state = radio.transmit("Hello World!");
    if(state == RADIOLIB_ERR_NONE) {
        printf("success!\n");

        hal->delay(1000);

        printf("all done!\n");
    } else {
        printf("failed, code %d\n", state);
        return;
    }
}