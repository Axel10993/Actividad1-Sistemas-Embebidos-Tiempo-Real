#include "app_tasks.hpp"
#include "system_state.hpp"
#include "esp_log.h"

namespace
{
    const char *TAG = "MAIN";
}

extern "C" void app_main(void)
{
    static practica1::AppContext context;

    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, " Practica 1 - Doble contador BCD (C++)");
    ESP_LOGI(TAG, "==============================================");

    ESP_LOGI(TAG, "[ETAPA 1/3] Inicializando contexto...");
    practica1::systemStateInit(context);

    ESP_LOGI(TAG, "[ETAPA 2/3] Creando tareas...");
    practica1::createApplicationTasks(context);

    ESP_LOGI(TAG, "[ETAPA 3/3] app_main terminado, el scheduler toma el control");
}
