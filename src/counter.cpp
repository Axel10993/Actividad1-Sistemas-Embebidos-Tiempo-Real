#include "counter.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

namespace practica1
{
    namespace
    {
        const char *TAG = "COUNTER";
    }

    BcdCounter::BcdCounter(uint8_t initialValue)
        : value_(static_cast<uint8_t>(initialValue % 10U))
    {
    }

    void BcdCounter::set(uint8_t newValue)
    {
        value_ = static_cast<uint8_t>(newValue % 10U);
    }

    void BcdCounter::step(CountDirection direction)
    {
        if (direction == CountDirection::Up)
        {
            value_ = static_cast<uint8_t>((value_ + 1U) % 10U);
        }
        else
        {
            value_ = (value_ == 0U) ? 9U : static_cast<uint8_t>(value_ - 1U);
        }
    }

    uint8_t BcdCounter::value() const
    {
        return value_;
    }

    void counterTask(void *pvParameters)
    {
        auto *config = static_cast<CounterConfig *>(pvParameters);

        if (config == nullptr)
        {
            ESP_LOGE(TAG, "%s: pvParameters es NULL, se elimina la tarea", pcTaskGetName(nullptr));
            vTaskDelete(nullptr);
            return;
        }

        /* Misma función, distinto pvParameters: el nombre y la dirección de config lo demuestran. */
        ESP_LOGI(TAG, "%s iniciada: config=%p display=%u valor=%u",
                 pcTaskGetName(nullptr),
                 static_cast<void *>(config),
                 static_cast<unsigned>(config->displayId),
                 static_cast<unsigned>(config->value));

        BcdCounter counter(config->value);
        bool firstStep = true;

        for (;;)
        {
            /* Primero espera: así se ven 0 y 9 y el manager suspende antes del primer paso. */
            vTaskDelay(pdMS_TO_TICKS(config->periodMs));

            /* Toma el valor publicado: el TaskManager lo ajusta al cambiar a SAME. */
            counter.set(config->value);
            counter.step(config->direction);
            config->value = counter.value();

            if (firstStep)
            {
                firstStep = false;
                ESP_LOGI(TAG, "%s contando: primer paso -> %u (%s)",
                         pcTaskGetName(nullptr),
                         static_cast<unsigned>(counter.value()),
                         toString(config->direction));
            }

            /* El display NO se escribe aquí; displayRefreshTask() multiplexa. */
        }
    }
}
