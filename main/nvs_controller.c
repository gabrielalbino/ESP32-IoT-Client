#include "nvs_controller.h"

#define STORAGE_NAMESPACE "storage"

void le_valor_nvs(Device *device)
{
  // Inicia o acesso à partição padrão nvs
  ESP_ERROR_CHECK(nvs_flash_init());

  nvs_handle particao_padrao_handle;
  
  // Abre o acesso à partição nvs
  esp_err_t res_nvs = nvs_open("storage", NVS_READONLY, &particao_padrao_handle);
  ESP_LOGI("NVS", "Abrindo particao (%s)", esp_err_to_name(res_nvs));


  if(res_nvs == ESP_ERR_NVS_NOT_FOUND)
  {
    device->name[0] = '\0';
    return;
  }
  else
  {
    size_t size = sizeof(Device);
    esp_err_t res = nvs_get_blob(particao_padrao_handle, "device", device, &size);
    ESP_LOGI("NVS", "lendo blob[%d] (%s)", size, esp_err_to_name(res));

    switch (res)
    {
    case ESP_OK:
      break;
    case ESP_ERR_NOT_FOUND:
      device->name[0] = '\0';
      break;
    default:
      break;
      }

      nvs_close(particao_padrao_handle);
  }
}

void grava_valor_nvs(Device* device)
{
  ESP_ERROR_CHECK(nvs_flash_init());

  nvs_handle particao_padrao_handle;

  esp_err_t res_nvs = nvs_open("storage", NVS_READWRITE, &particao_padrao_handle);
  
  if(res_nvs == ESP_ERR_NVS_NOT_FOUND)
  {
      ESP_LOGE("NVS", "Namespace: storage, não encontrado");
  }
  size_t size = sizeof(Device);
  esp_err_t res = nvs_set_blob(particao_padrao_handle, "device", device, size);
  ESP_LOGI("NVS", "Salvando blob[%d] (%s)", size, esp_err_to_name(res));
  if(res != ESP_OK)
  {
      ESP_LOGE("NVS", "Não foi possível escrever no NVS (%s)", esp_err_to_name(res));
  }
  nvs_commit(particao_padrao_handle);
  nvs_close(particao_padrao_handle);
}
