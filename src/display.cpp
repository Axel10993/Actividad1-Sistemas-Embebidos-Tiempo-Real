#include "display.hpp"
#include "app_config.hpp"
#include "system_state.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

namespace practica1
{
    namespace
    {
        const char *TAG = "DISPLAY";

        /* Patrones a..g (bit0 = a) para cátodo común. */
        constexpr uint8_t kDigits[10] =
        {
            0x3FU, 0x06U, 0x5BU, 0x4FU, 0x66U,
            0x6DU, 0x7DU, 0x07U, 0x7FU, 0x6FU
        };

        constexpr uint32_t kSegmentOffLevel = (config::kSegmentOnLevel != 0U) ? 0U : 1U;
        constexpr uint32_t kDisplayOffLevel = (config::kDisplayOnLevel != 0U) ? 0U : 1U;

        /* Autotest al arrancar: cada display muestra 8 (todos los segmentos). */
        constexpr uint32_t kSelfTestMs = 500U;

        /* Cada cuántos ciclos de refresco se imprime que la tarea sigue viva (~10 s). */
        constexpr uint32_t kAliveEveryCycles = 1000U;
    }

    SevenSegmentDisplay::SevenSegmentDisplay(const gpio_num_t (&segmentPins)[7], gpio_num_t enablePin)
        : segmentPins_(segmentPins), enablePin_(enablePin)
    {
    }

    void SevenSegmentDisplay::init()
    {
        for (gpio_num_t pin : segmentPins_)
        {
            gpio_reset_pin(pin);
            gpio_set_direction(pin, GPIO_MODE_OUTPUT);
        }

        gpio_reset_pin(enablePin_);
        gpio_set_direction(enablePin_, GPIO_MODE_OUTPUT);

        blank();
        writeSegments(0U);

        ESP_LOGI(TAG, "Display con enable en GPIO%d inicializado", static_cast<int>(enablePin_));
    }

    void SevenSegmentDisplay::writeSegments(uint8_t pattern)
    {
        for (uint8_t i = 0U; i < 7U; ++i)
        {
            const bool on = ((pattern >> i) & 0x01U) != 0U;
            gpio_set_level(segmentPins_[i], on ? config::kSegmentOnLevel : kSegmentOffLevel);
        }
    }

    void SevenSegmentDisplay::show(uint8_t digit)
    {
        if (digit > 9U)
        {
            blank();
            return;
        }

        writeSegments(kDigits[digit]);
        gpio_set_level(enablePin_, config::kDisplayOnLevel);
    }

    void SevenSegmentDisplay::blank()
    {
        gpio_set_level(enablePin_, kDisplayOffLevel);
    }

    void displayRefreshTask(void *pvParameters)
    {
        auto *context = static_cast<AppContext *>(pvParameters);

        if (context == nullptr)
        {
            ESP_LOGE(TAG, "pvParameters es NULL, se elimina la tarea");
            vTaskDelete(nullptr);
            return;
        }

        SevenSegmentDisplay display1(config::kSegmentPins, config::kDisplay1Enable);
        SevenSegmentDisplay display2(config::kSegmentPins, config::kDisplay2Enable);

        display1.init();
        display2.init();

        /* Autotest: si algún segmento no prende aquí, el problema es de cableado. */
        ESP_LOGI(TAG, "Autotest: Display 1 muestra 8");
        display2.blank();
        display1.show(8U);
        vTaskDelay(pdMS_TO_TICKS(kSelfTestMs));

        ESP_LOGI(TAG, "Autotest: Display 2 muestra 8");
        display1.blank();
        display2.show(8U);
        vTaskDelay(pdMS_TO_TICKS(kSelfTestMs));

        display2.blank();
        ESP_LOGI(TAG, "Autotest terminado");

        /* Con HZ=100, pdMS_TO_TICKS(5) = 0: se fuerza mínimo 1 tick. */
        TickType_t refresh = pdMS_TO_TICKS(config::kDisplayRefreshMs);
        if (refresh == 0U)
        {
            refresh = 1U;
        }

        ESP_LOGI(TAG, "Multiplexado activo: %u tick(s) por display", static_cast<unsigned>(refresh));

        uint32_t cycles = 0U;

        for (;;)
        {
            /* Se apaga el otro antes de cambiar segmentos para evitar "fantasmas". */
            display2.blank();
            display1.show(context->counter1.value);
            vTaskDelay(refresh);

            display1.blank();
            display2.show(context->counter2.value);
            vTaskDelay(refresh);

            cycles++;
            if (cycles >= kAliveEveryCycles)
            {
                cycles = 0U;
                ESP_LOGI(TAG, "Refresco activo: D1=%u D2=%u",
                         static_cast<unsigned>(context->counter1.value),
                         static_cast<unsigned>(context->counter2.value));
            }
        }
    }
}
