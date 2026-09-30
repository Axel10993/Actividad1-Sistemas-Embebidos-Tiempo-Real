#include "system_state.hpp"
#include "app_config.hpp"
#include "esp_log.h"

namespace practica1
{
    namespace
    {
        const char *TAG = "SYSTEM_STATE";
    }

    void systemStateInit(AppContext &c)
    {
        /* Estado inicial del sistema */
        c.system.runState        = RunState::Paused;
        c.system.masterDirection = CountDirection::Up;
        c.system.couplingMode    = CouplingMode::Opposite;
        c.system.speedMode       = SpeedMode::Slow;

        /* Counter 1 */
        c.counter1.value     = 0U;
        c.counter1.direction = CountDirection::Up;
        c.counter1.periodMs  = config::kSlowPeriodMs;
        c.counter1.displayId = 1U;

        /* Counter 2 */
        c.counter2.value     = 9U;
        c.counter2.direction = CountDirection::Down;
        c.counter2.periodMs  = config::kSlowPeriodMs;
        c.counter2.displayId = 2U;

        /* Eventos */
        c.startPauseEvent = { false, ButtonId::StartPause };
        c.directionEvent  = { false, ButtonId::Direction };
        c.speedEvent      = { false, ButtonId::Speed };
        c.modeEvent       = { false, ButtonId::Mode };

        /* Handles */
        c.counter1Handle = nullptr;
        c.counter2Handle = nullptr;
        c.managerHandle  = nullptr;

        ESP_LOGI(TAG, "Contexto OK: STATE=%s DIR=%s MODE=%s SPEED=%s",
                 toString(c.system.runState),
                 toString(c.system.masterDirection),
                 toString(c.system.couplingMode),
                 toString(c.system.speedMode));
        ESP_LOGI(TAG, "Counter1: valor=%u dir=%s periodo=%lu ms display=%u",
                 static_cast<unsigned>(c.counter1.value),
                 toString(c.counter1.direction),
                 static_cast<unsigned long>(c.counter1.periodMs),
                 static_cast<unsigned>(c.counter1.displayId));
        ESP_LOGI(TAG, "Counter2: valor=%u dir=%s periodo=%lu ms display=%u",
                 static_cast<unsigned>(c.counter2.value),
                 toString(c.counter2.direction),
                 static_cast<unsigned long>(c.counter2.periodMs),
                 static_cast<unsigned>(c.counter2.displayId));
    }

    const char *toString(CountDirection d) { return (d == CountDirection::Up) ? "UP" : "DOWN"; }
    const char *toString(CouplingMode m)   { return (m == CouplingMode::Same) ? "SAME" : "OPPOSITE"; }
    const char *toString(RunState s)       { return (s == RunState::Running) ? "RUNNING" : "PAUSED"; }
    const char *toString(SpeedMode s)      { return (s == SpeedMode::Fast) ? "FAST" : "SLOW"; }
}
