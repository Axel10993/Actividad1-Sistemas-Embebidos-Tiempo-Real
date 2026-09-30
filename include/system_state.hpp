#ifndef SYSTEM_STATE_HPP
#define SYSTEM_STATE_HPP

#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace practica1
{
    enum class CountDirection : uint8_t
    {
        Up = 0,
        Down
    };

    enum class CouplingMode : uint8_t
    {
        Opposite = 0,
        Same
    };

    enum class RunState : uint8_t
    {
        Paused = 0,
        Running
    };

    enum class SpeedMode : uint8_t
    {
        Slow = 0,
        Fast
    };

    enum class ButtonId : uint8_t
    {
        StartPause = 0,
        Direction,
        Speed,
        Mode
    };

    /* Lo que recibe cada tarea de conteo por pvParameters. */
    struct CounterConfig
    {
        volatile uint8_t        value;
        volatile CountDirection direction;
        volatile uint32_t       periodMs;
        uint8_t                 displayId;
    };

    /* Flag simple que publica una tarea de botón y consume el TaskManager. */
    struct ButtonEvent
    {
        volatile bool pending;
        ButtonId      id;
    };

    struct SystemState
    {
        volatile RunState       runState;
        volatile CountDirection masterDirection;
        volatile CouplingMode   couplingMode;
        volatile SpeedMode      speedMode;
    };

    /* Contexto compartido de la aplicación. */
    struct AppContext
    {
        SystemState system;

        CounterConfig counter1;
        CounterConfig counter2;

        ButtonEvent startPauseEvent;
        ButtonEvent directionEvent;
        ButtonEvent speedEvent;
        ButtonEvent modeEvent;

        TaskHandle_t counter1Handle;
        TaskHandle_t counter2Handle;
        TaskHandle_t managerHandle;
    };

    void systemStateInit(AppContext &context);

    const char *toString(CountDirection direction);
    const char *toString(CouplingMode mode);
    const char *toString(RunState state);
    const char *toString(SpeedMode speed);
}

#endif
