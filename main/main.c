#include <stdio.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "wifi.h"
#include <pthread.h>
#include "nvs.h"
#include "dht11.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "nvs_controller.h"
#include "mqtt.h"

#define LED 2

Device device;

xSemaphoreHandle conexaoWifiSemaphore, conexaoMQTTSemaphore, conexaoServidorSemaphore;

int ledLevel;


void toggleLed(){
  ledLevel = device.output.status;
  gpio_set_level(LED, ledLevel);
}

#define BOTAO_1 0

xQueueHandle filaDeInterrupcao;

static void IRAM_ATTR gpio_isr_handler(void *args)
{
  int pino = (int)args;
  xQueueSendFromISR(filaDeInterrupcao, &pino, NULL);
}


void PiscaLed(void* params){
  gpio_pad_select_gpio(LED);   
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  while (true)
  {
    if(device.output.status != ledLevel){
      toggleLed();
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}


void trataInterrupcaoBotao(void *params)
{
  int pino;

  while(true)
  {
    if(xQueueReceive(filaDeInterrupcao, &pino, portMAX_DELAY))
    {
      // De-bouncing
      int estado = gpio_get_level(pino);
      ESP_LOGI("BTN", "estado: %d", estado);
      if(device.input.status != estado)
      {
        gpio_isr_handler_remove(pino);
        while(gpio_get_level(pino) == estado)
        {
          vTaskDelay(50 / portTICK_PERIOD_MS);
        }
        device.input.status = estado;
        if(strlen(device.name)) mqtt_send_input(&device);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        gpio_isr_handler_add(pino, gpio_isr_handler, (void *) pino);
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void trataBotao(void *params)
{
  int pino = BOTAO_1;

  while(true)
  {
    // De-bouncing
    int estado = gpio_get_level(pino);
    if(device.input.status != estado)
    {
      while(gpio_get_level(pino) == estado)
      {
        vTaskDelay(50 / portTICK_PERIOD_MS);
      }
      device.input.status = estado;
      if(strlen(device.name)) mqtt_send_input(&device);
      vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}


void wifiStarted(void * params)
{
  while(true)
  {
    if (xSemaphoreTake(conexaoWifiSemaphore, portMAX_DELAY))
    {
      mqtt_start();
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }
}

void printDeviceInfo(){
  printf("Comodo: %s (%s)\n", strlen(device.name) != 0 ? device.name : "Indefinido", device.id);
  printf("Temperatura: %.2f\n", device.temperature);
  printf("Humidade: %.2f\n", device.humidity);
  printf("%s: %s\n", strlen(device.input.name) != 0 ? device.input.name : "Indefinido", device.input.status ? "On" : "Off");
  printf("%s: %s\n", strlen(device.output.name) != 0 ? device.output.name : "Indefinido", device.output.status ? "On" : "Off");
}

void realizaCadastro(char* topico){
    cJSON *json = cJSON_CreateObject();
    cJSON *tipo = cJSON_CreateString("solicitacao-cadastro");
    cJSON_AddItemToObject(json, "tipo", tipo);
    char *szJson = cJSON_Print(json);
    mqtt_envia_mensagem(topico, szJson);
    mqtt_subscribe(topico);
    cJSON_Delete(json);
    free(szJson);
    if (xSemaphoreTake(conexaoServidorSemaphore, portMAX_DELAY))
    {
      grava_valor_nvs(&device);
    }
}

void trataComunicacaoComServidor(void * params)
{
  char topico[100];
  sprintf(topico, "%s/dispositivos/%s", TOPIC_PREFIX, device.id);
  if(xSemaphoreTake(conexaoMQTTSemaphore, portMAX_DELAY))
  {
    le_valor_nvs(&device);
    printDeviceInfo();
    if (strlen(device.name) == 0)
    {
      realizaCadastro(topico);

    }
    else{
      mqtt_subscribe(topico);
      sprintf(topico, "%s/%s/estado", TOPIC_PREFIX, device.name);
      mqtt_subscribe(topico);
    }
    while (true)
    {
      struct dht11_reading reading = DHT11_read();
      device.temperature = reading.temperature;
      device.humidity = reading.humidity;
      mqtt_send_status(&device);
      printDeviceInfo();
      vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
  }
}

void setupDevice(){
  ledLevel = 0;
  device.input.status = 0;
  device.output.status = 0;
  uint8_t base_mac_addr[6] = {0};
  esp_efuse_mac_get_default(base_mac_addr);
  sprintf(device.id, "%x:%x:%x:%x:%x:%x",
          base_mac_addr[0], base_mac_addr[1], base_mac_addr[2],
          base_mac_addr[3], base_mac_addr[4], base_mac_addr[5]);
  // Inicializa o NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  DHT11_init(4);
}


void usarInterrupcao(){
  gpio_pulldown_en(BOTAO_1);
  gpio_pullup_en(BOTAO_1);

  // Configura pino para interrupção
  gpio_set_intr_type(BOTAO_1, GPIO_INTR_POSEDGE);

  filaDeInterrupcao = xQueueCreate(10, sizeof(int));
  xTaskCreate(trataInterrupcaoBotao, "TrataBotao", 2048, NULL, 1, NULL);

  gpio_install_isr_service(0);
  gpio_isr_handler_add(BOTAO_1, gpio_isr_handler, (void *) BOTAO_1);
}

void app_main(void)
{
  setupDevice();
  conexaoWifiSemaphore = xSemaphoreCreateBinary();
  conexaoMQTTSemaphore = xSemaphoreCreateBinary();
  conexaoServidorSemaphore = xSemaphoreCreateBinary();
  pthread_t threadWifi;
  pthread_create(&threadWifi, NULL, (void*)&wifi_start, NULL);
  xTaskCreate(&PiscaLed, "Pisca LED", 512, NULL, 1, NULL);
  xTaskCreate(&wifiStarted,  "Processa Request", 40960, NULL, 3, NULL);
  xTaskCreate(&trataComunicacaoComServidor, "Comunicação com Broker", 4096, NULL, 1, NULL);
  gpio_pad_select_gpio(BOTAO_1);
  // Configura o pino do Botão como Entrada
  gpio_set_direction(BOTAO_1, GPIO_MODE_INPUT);
  // Configura o resistor de Pulldown para o botão (por padrão a entrada estará em Zero)

  
  usarInterrupcao(); //comentar caso for testar o loop.
  //xTaskCreate(&trataBotao, "tratar botão sem interrupcao", 4096, NULL, 1, NULL);
}
