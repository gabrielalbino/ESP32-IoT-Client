
#include "mqtt.h"
extern xSemaphoreHandle conexaoMQTTSemaphore, conexaoServidorSemaphore;
extern Device device;

esp_mqtt_client_handle_t client;

int _strpos(char *hay, char *needle, int offset)
{
   char haystack[strlen(hay)];
   strncpy(haystack, hay+offset, strlen(hay)-offset);
   char *p = strstr(haystack, needle);
   if (p)
      return p - haystack+offset;
   return -1;
}

void handleEventData(esp_mqtt_event_handle_t event){
    cJSON *json = cJSON_Parse(event->data);
    char *tipo = cJSON_GetObjectItemCaseSensitive(json, "tipo")->valuestring;
    if(strcmp(tipo, "cadastro-realizado") == 0){
        char *name = cJSON_GetObjectItemCaseSensitive(json, "nome")->valuestring;
        char *entrada = cJSON_GetObjectItemCaseSensitive(json, "entrada")->valuestring;
        char *saida = cJSON_GetObjectItemCaseSensitive(json, "saida")->valuestring;
        strcpy(device.name, name);
        strcpy(device.input.name, entrada);
        strcpy(device.output.name, saida);
        xSemaphoreGive(conexaoServidorSemaphore);
    }
    else if(strcmp(tipo, "set-output") == 0){
        int status = cJSON_GetObjectItemCaseSensitive(json, "status")->valueint;
        device.output.status = status;
    }
    grava_valor_nvs(&device);
    cJSON_Delete(json);
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            xSemaphoreGive(conexaoMQTTSemaphore);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "data = %.*s", event->data_len, event->data);
            handleEventData(event);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

void mqtt_start()
{
    esp_mqtt_client_config_t mqtt_config = {
        .uri = "mqtt://test.mosquitto.org",
    };
    client = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void mqtt_envia_mensagem(char * topico, char * mensagem)
{
    int message_id = esp_mqtt_client_publish(client, topico, mensagem, _strpos(mensagem, "}", 0) + 1, 1, 0);
    ESP_LOGI(TAG, "Mesnagem enviada para %s, ID: %d", topico, message_id);
}

void mqtt_subscribe(char* topico){
    ESP_LOGI(TAG, "Se inscrevendo em %s", topico);
    esp_mqtt_client_subscribe(client, topico, 0);
}


void mqtt_unsubscribe(char* topico){
    esp_mqtt_client_unsubscribe(client, topico);
}


void mqtt_send_status(Device *device)
{
    char topico[100];
    sprintf(topico, "%s/%s/temperatura", TOPIC_PREFIX, device->name);
    cJSON *json = cJSON_CreateObject();
    cJSON *value = cJSON_CreateNumber((double)device->temperature);
    cJSON_AddItemToObject(json, "temperatura", value);
    char *szJson = cJSON_Print(json);
    mqtt_envia_mensagem(topico, szJson);
    cJSON_Delete(json);
    free(szJson);
    sprintf(topico, "%s/%s/umidade", TOPIC_PREFIX, device->name);
    json = cJSON_CreateObject();
    value = cJSON_CreateNumber((double)device->humidity);
    cJSON_AddItemToObject(json, "umidade", value);
    szJson = cJSON_Print(json);
    mqtt_envia_mensagem(topico, szJson);
    cJSON_Delete(json);
    free(szJson);
    grava_valor_nvs(device);
}

void mqtt_send_input(Device *device)
{
    char topico[100];
    sprintf(topico, "%s/%s/estado", TOPIC_PREFIX, device->name);
    ESP_LOGI("mqtt_send_input", "topico: %s", topico);
    cJSON *json = cJSON_CreateObject();
    cJSON *tipo = cJSON_CreateString("set-input");
    cJSON *value = cJSON_CreateNumber((double)device->input.status);
    cJSON_AddItemToObject(json, "input", value);
    cJSON_AddItemToObject(json, "tipo", tipo);
    char *szJson = cJSON_Print(json);
    mqtt_envia_mensagem(topico, szJson);
    cJSON_Delete(json);
    free(szJson);
    grava_valor_nvs(device);
}