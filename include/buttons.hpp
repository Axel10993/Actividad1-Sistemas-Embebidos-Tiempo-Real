#ifndef BUTTONS_HPP
#define BUTTONS_HPP

#include "driver/gpio.h"
#include "system_state.hpp"

namespace practica1
{
    /* Lo que recibe cada tarea de botón por pvParameters. */
    struct ButtonTaskParams
    {
        gpio_num_t   pin;
        ButtonEvent *event;
    };

    void buttonInit(gpio_num_t pin);
    bool buttonIsPressed(gpio_num_t pin);

    /* Misma función para los cuatro botones. */
    void buttonTask(void *pvParameters);
}

#endif
