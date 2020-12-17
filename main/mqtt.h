#ifndef MQTT_H
#define MQTT_H
#define TOPIC_PREFIX "fse2020/160028361"
#include "types.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "nvs_controller.h"
#include "types.h"

#define TAG "MQTT"


void mqtt_start();

void mqtt_envia_mensagem(char * topico, char * mensagem);
void mqtt_subscribe(char *topico);
void mqtt_unsubscribe(char *topico);
void mqtt_send_input(Device *device);
void mqtt_send_status(Device *device);
#endif
