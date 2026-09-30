#ifndef APP_CONFIG_HPP
#define APP_CONFIG_HPP

#include <cstdint>
#include "driver/gpio.h"

namespace practica1::config
{
    /*
     * Pines adaptados a ESP32-S3-DevKitC-1 (N16R8).
     * Se evitan: strapping (0, 3, 45, 46), USB (19, 20), UART0 (43, 44),
     * flash (26..32), PSRAM octal (35..37), LED RGB (38) y JTAG (39..42).
     */

    /* Segmentos compartidos por ambos displays (multiplexados), orden a..g. */
    constexpr gpio_num_t kSegmentPins[7] =
    {
        GPIO_NUM_4,   // a
        GPIO_NUM_5,   // b
        GPIO_NUM_6,   // c
        GPIO_NUM_7,   // d
        GPIO_NUM_15,  // e
        GPIO_NUM_16,  // f
        GPIO_NUM_17   // g
    };

    /* Habilitación de cada display (base del NPN del cátodo común). */
    constexpr gpio_num_t kDisplay1Enable = GPIO_NUM_18;
    constexpr gpio_num_t kDisplay2Enable = GPIO_NUM_8;

    /* Niveles lógicos: cátodo común con NPN en el común. */
    constexpr uint32_t kSegmentOnLevel  = 1U;
    constexpr uint32_t kDisplayOnLevel  = 1U;

    /* Botones a GND (pull-up interno). */
    constexpr gpio_num_t kBtnStartPause = GPIO_NUM_9; //No jala el pin
    constexpr gpio_num_t kBtnDirection  = GPIO_NUM_10; //No jala el pin
    constexpr gpio_num_t kBtnSpeed      = GPIO_NUM_11;
    constexpr gpio_num_t kBtnMode       = GPIO_NUM_12;

    constexpr uint32_t kSlowPeriodMs     = 500U;
    constexpr uint32_t kFastPeriodMs     = 250U;
    constexpr uint32_t kButtonPeriodMs   = 20U;
    constexpr uint32_t kManagerPeriodMs  = 10U;
    constexpr uint32_t kDisplayRefreshMs = 5U;
}

#endif
