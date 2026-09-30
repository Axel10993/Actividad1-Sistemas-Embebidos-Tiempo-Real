#include "app_tasks.hpp"
#include "app_config.hpp"
#include "buttons.hpp"
#include "counter.hpp"
#include "display.hpp"
#include "esp_log.h"
#include "esp_system.h"

namespace practica1
{
    namespace
    {
        const char *TAG = "TASK_MANAGER";

        /* Cada cuánto imprime el TaskManager un resumen del sistema (heartbeat). */
        constexpr uint32_t kHeartbeatMs = 2000U;

        CountDirection opposite(CountDirection direction)
        {
            return (direction == CountDirection::Up) ? CountDirection::Down : CountDirection::Up;
        }

        void applyConfiguration(AppContext &c)
        {
            c.counter1.direction = c.system.masterDirection;

            if (c.system.couplingMode == CouplingMode::Same)
            {
                c.counter2.direction = c.system.masterDirection;
            }
            else
            {
                c.counter2.direction = opposite(c.system.masterDirection);
            }

            const uint32_t period = (c.system.speedMode == SpeedMode::Fast)
                                        ? config::kFastPeriodMs
                                        : config::kSlowPeriodMs;

            c.counter1.periodMs = period;
            c.counter2.periodMs = period;

            ESP_LOGI(TAG, "Config aplicada: C1=%s C2=%s periodo=%lu ms",
                     toString(c.counter1.direction),
                     toString(c.counter2.direction),
                     static_cast<unsigned long>(period));
        }

        void setCountersRunning(AppContext &c, bool running)
        {
            if (running)
            {
                vTaskResume(c.counter1Handle);
                vTaskResume(c.counter2Handle);
                ESP_LOGI(TAG, "vTaskResume(Counter1=%p, Counter2=%p)",
                         static_cast<void *>(c.counter1Handle),
                         static_cast<void *>(c.counter2Handle));
            }
            else
            {
                vTaskSuspend(c.counter1Handle);
                vTaskSuspend(c.counter2Handle);
                ESP_LOGI(TAG, "vTaskSuspend(Counter1=%p, Counter2=%p)",
                         static_cast<void *>(c.counter1Handle),
                         static_cast<void *>(c.counter2Handle));
            }
        }

        /* OPPOSITE -> SAME: DOWN iguala al menor, UP iguala al mayor. */
        void alignCounters(AppContext &c)
        {
            const uint8_t v1 = c.counter1.value;
            const uint8_t v2 = c.counter2.value;
            uint8_t target;

            if (c.system.masterDirection == CountDirection::Down)
            {
                target = (v1 < v2) ? v1 : v2;
            }
            else
            {
                target = (v1 > v2) ? v1 : v2;
            }

            c.counter1.value = target;
            c.counter2.value = target;

            ESP_LOGI(TAG, "ALIGN (%s): %u/%u -> %u",
                     toString(c.system.masterDirection),
                     static_cast<unsigned>(v1),
                     static_cast<unsigned>(v2),
                     static_cast<unsigned>(target));
        }

        const char *taskStateToString(eTaskState s)
        {
            switch (s)
            {
                case eRunning:   return "RUNNING";
                case eReady:     return "READY";
                case eBlocked:   return "BLOCKED";
                case eSuspended: return "SUSPENDED";
                case eDeleted:   return "DELETED";
                default:         return "INVALID";
            }
        }

        void logCounterStates(const AppContext &c)
        {
            ESP_LOGI(TAG, "Counter1=%s  Counter2=%s",
                     taskStateToString(eTaskGetState(c.counter1Handle)),
                     taskStateToString(eTaskGetState(c.counter2Handle)));
        }

        void logHeartbeat(const AppContext &c)
        {
            ESP_LOGI(TAG, "[HB] %s | D1=%u D2=%u | DIR=%s MODE=%s SPEED=%s | C1=%s C2=%s",
                     toString(c.system.runState),
                     static_cast<unsigned>(c.counter1.value),
                     static_cast<unsigned>(c.counter2.value),
                     toString(c.system.masterDirection),
                     toString(c.system.couplingMode),
                     toString(c.system.speedMode),
                     taskStateToString(eTaskGetState(c.counter1Handle)),
                     taskStateToString(eTaskGetState(c.counter2Handle)));
        }

        /* En pausa los eventos se descartan, pero se avisa para que se vea que sí llegaron. */
        void discardIfPending(ButtonEvent &event, const char *name)
        {
            if (event.pending)
            {
                event.pending = false;
                ESP_LOGW(TAG, "Evento %s ignorado (sistema en PAUSED)", name);
            }
        }
    }

    void taskManager(void *pvParameters)
    {
        auto *ctx = static_cast<AppContext *>(pvParameters);

        if (ctx == nullptr)
        {
            ESP_LOGE(TAG, "pvParameters es NULL, se elimina la tarea");
            vTaskDelete(nullptr);
            return;
        }

        AppContext &c = *ctx;

        ESP_LOGI(TAG, "TaskManager iniciado (context=%p)", static_cast<void *>(ctx));

        if ((c.counter1Handle == nullptr) || (c.counter2Handle == nullptr))
        {
            ESP_LOGE(TAG, "Handles de contadores invalidos, no se puede continuar");
            vTaskDelete(nullptr);
            return;
        }

        applyConfiguration(c);
        setCountersRunning(c, false);

        ESP_LOGI(TAG, "STATE=%s DIR=%s SPEED=%s MODE=%s",
                 toString(c.system.runState),
                 toString(c.system.masterDirection),
                 toString(c.system.speedMode),
                 toString(c.system.couplingMode));
        logCounterStates(c);
        ESP_LOGI(TAG, ">>> Sistema listo. Presiona START para comenzar <<<");

        const uint32_t heartbeatLoops = kHeartbeatMs / config::kManagerPeriodMs;
        uint32_t loops = 0U;

        for (;;)
        {
            if (c.startPauseEvent.pending)
            {
                c.startPauseEvent.pending = false;

                c.system.runState = (c.system.runState == RunState::Running) ? RunState::Paused : RunState::Running;

                setCountersRunning(c, c.system.runState == RunState::Running);

                ESP_LOGI(TAG, "STATE=%s", toString(c.system.runState));
                logCounterStates(c);
            }

            /* Dirección, velocidad y modo se ignoran estando en PAUSE. */
            if (c.system.runState == RunState::Running)
            {
                if (c.directionEvent.pending)
                {
                    c.directionEvent.pending = false;
                    c.system.masterDirection = opposite(c.system.masterDirection);
                    applyConfiguration(c);
                    ESP_LOGI(TAG, "DIR=%s", toString(c.system.masterDirection));
                }

                if (c.speedEvent.pending)
                {
                    c.speedEvent.pending = false;
                    c.system.speedMode = (c.system.speedMode == SpeedMode::Slow) ? SpeedMode::Fast : SpeedMode::Slow;
                    applyConfiguration(c);
                    ESP_LOGI(TAG, "SPEED=%s", toString(c.system.speedMode));
                }

                if (c.modeEvent.pending)
                {
                    c.modeEvent.pending = false;

                    /* Se suspenden para que ningún contador escriba mientras se ajustan. */
                    setCountersRunning(c, false);

                    c.system.couplingMode = (c.system.couplingMode == CouplingMode::Opposite)
                                                ? CouplingMode::Same
                                                : CouplingMode::Opposite;

                    if (c.system.couplingMode == CouplingMode::Same)
                    {
                        alignCounters(c);
                    }

                    applyConfiguration(c);
                    setCountersRunning(c, true);

                    ESP_LOGI(TAG, "MODE=%s", toString(c.system.couplingMode));
                }
            }
            else
            {
                discardIfPending(c.directionEvent, "DIRECTION");
                discardIfPending(c.speedEvent, "SPEED");
                discardIfPending(c.modeEvent, "MODE");
            }

            loops++;
            if (loops >= heartbeatLoops)
            {
                loops = 0U;
                logHeartbeat(c);
            }

            vTaskDelay(pdMS_TO_TICKS(config::kManagerPeriodMs));
        }
    }

    void createApplicationTasks(AppContext &c)
    {
        static ButtonTaskParams startParams     { config::kBtnStartPause, nullptr };
        static ButtonTaskParams directionParams { config::kBtnDirection,  nullptr };
        static ButtonTaskParams speedParams     { config::kBtnSpeed,      nullptr };
        static ButtonTaskParams modeParams      { config::kBtnMode,       nullptr };

        startParams.event     = &c.startPauseEvent;
        directionParams.event = &c.directionEvent;
        speedParams.event     = &c.speedEvent;
        modeParams.event      = &c.modeEvent;

        ESP_LOGI(TAG, "Heap libre antes de crear tareas: %lu bytes",
                 static_cast<unsigned long>(esp_get_free_heap_size()));

        uint32_t created = 0U;
        constexpr uint32_t kTotalTasks = 8U;

        auto check = [&created](BaseType_t result, const char *name)
        {
            if (result == pdPASS)
            {
                created++;
                ESP_LOGI(TAG, "  [OK] Tarea %s creada", name);
            }
            else
            {
                ESP_LOGE(TAG, "  [FALLO] No se pudo crear %s (sin memoria)", name);
            }
        };

        /* MISMA función counterTask() --> dos instancias, distinto pvParameters. */
        check(xTaskCreate(counterTask, "Counter1", 3072, &c.counter1, 2, &c.counter1Handle), "Counter1");
        check(xTaskCreate(counterTask, "Counter2", 3072, &c.counter2, 2, &c.counter2Handle), "Counter2");

        /* MISMA función buttonTask() --> cuatro botones distintos. */
        check(xTaskCreate(buttonTask, "BtnStart",     3072, &startParams,     2, nullptr), "BtnStart");
        check(xTaskCreate(buttonTask, "BtnDirection", 3072, &directionParams, 2, nullptr), "BtnDirection");
        check(xTaskCreate(buttonTask, "BtnSpeed",     3072, &speedParams,     2, nullptr), "BtnSpeed");
        check(xTaskCreate(buttonTask, "BtnMode",      3072, &modeParams,      2, nullptr), "BtnMode");

        check(xTaskCreate(displayRefreshTask, "DisplayRefresh", 3072, &c, 2, nullptr), "DisplayRefresh");
        check(xTaskCreate(taskManager, "TaskManager", 3072, &c, 3, &c.managerHandle), "TaskManager");

        if (created == kTotalTasks)
        {
            ESP_LOGI(TAG, "Todas las tareas creadas (%lu/%lu). Heap libre: %lu bytes",
                     static_cast<unsigned long>(created),
                     static_cast<unsigned long>(kTotalTasks),
                     static_cast<unsigned long>(esp_get_free_heap_size()));
        }
        else
        {
            ESP_LOGE(TAG, "Solo se crearon %lu/%lu tareas",
                     static_cast<unsigned long>(created),
                     static_cast<unsigned long>(kTotalTasks));
        }
    }
}
