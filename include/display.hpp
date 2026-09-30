#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <cstdint>
#include "driver/gpio.h"

namespace practica1
{
    /* Responsabilidad: solo mostrar un dígito en un display (GPIO). */
    class SevenSegmentDisplay
    {
    public:
        SevenSegmentDisplay(const gpio_num_t (&segmentPins)[7], gpio_num_t enablePin);

        void init();
        void show(uint8_t digit);
        void blank();

    private:
        void writeSegments(uint8_t pattern);

        const gpio_num_t (&segmentPins_)[7];
        gpio_num_t enablePin_;
    };

    /* Multiplexa ambos displays; pvParameters = AppContext*. */
    void displayRefreshTask(void *pvParameters);
}

#endif
