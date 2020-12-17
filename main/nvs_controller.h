#ifndef nvs_h
#define nvs_h
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "types.h"

void le_valor_nvs(Device *device);
void grava_valor_nvs(Device *device);

#endif