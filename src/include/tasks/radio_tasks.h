#ifndef RADIO_TASKS_H
#define RADIO_TASKS_H

void initRadio();

void initRadioTask(void *pvParameters);

void telemetryRadio(void *pvParameters);

void commandRadio(void *pvParameters);

#endif