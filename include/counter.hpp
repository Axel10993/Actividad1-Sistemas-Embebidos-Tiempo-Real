#ifndef COUNTER_HPP
#define COUNTER_HPP

#include <cstdint>
#include "system_state.hpp"

namespace practica1
{
    /* Responsabilidad: solo la lógica BCD 0-9 (no sabe de GPIO ni de FreeRTOS). */
    class BcdCounter
    {
    public:
        explicit BcdCounter(uint8_t initialValue = 0U);

        void    set(uint8_t newValue);
        void    step(CountDirection direction);
        uint8_t value() const;

    private:
        uint8_t value_;
    };

    /* Misma función para Counter1 y Counter2; pvParameters = CounterConfig*. */
    void counterTask(void *pvParameters);
}

#endif
