#ifndef APP_TASKS_HPP
#define APP_TASKS_HPP

#include "system_state.hpp"

namespace practica1
{
    void taskManager(void *pvParameters);
    void createApplicationTasks(AppContext &context);
}

#endif
