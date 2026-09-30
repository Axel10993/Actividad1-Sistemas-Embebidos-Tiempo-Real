#include "buttons.hpp"
#include "app_config.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

namespace practica1
{
    namespace
    {
        const char *TAG = "BUTTON";
    }

    void buttonInit(gpio_num_t pin)
    {
        gpio_reset_pin(pin);
        gpio_set_direction(pin, GPIO_MODE_INPUT);
        gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    }

    bool buttonIsPressed(gpio_num_t pin)
    {
        return gpio_get_level(pin) == 0;
    }

    void buttonTask(void *pvParameters)
    {
        auto *params = static_cast<ButtonTaskParams *>(pvParameters);
        bool previous = false;

        if ((params == nullptr) || (params->event == nullptr))
        {
            ESP_LOGE(TAG, "%s: parametros invalidos, se elimina la tarea", pcTaskGetName(nullptr));
            vTaskDelete(nullptr);
            return;
        }

        buttonInit(params->pin);

        /* Con pull-up, en reposo debe leer 1. Si lee 0 hay un corto a GND o el botón está mal puesto. */
        const int level = gpio_get_level(params->pin);
        if (level == 1)
        {
            ESP_LOGI(TAG, "%s lista en GPIO%d (reposo=1 OK)",
                     pcTaskGetName(nullptr), static_cast<int>(params->pin));
        }
        else
        {
            ESP_LOGW(TAG, "%s en GPIO%d lee 0 en reposo: revisa el cableado",
                     pcTaskGetName(nullptr), static_cast<int>(params->pin));
        }

        for (;;)
        {
            const bool current = buttonIsPressed(params->pin);

            /* Flanco de pulsación (muestreo cada 20 ms = antirrebote simple). */
            if (current && !previous)
            {
                params->event->pending = true;
                ESP_LOGI(TAG, "%s presionado (GPIO%d)",
                         pcTaskGetName(nullptr), static_cast<int>(params->pin));
            }

            previous = current;
            vTaskDelay(pdMS_TO_TICKS(config::kButtonPeriodMs));
        }
    }
}
