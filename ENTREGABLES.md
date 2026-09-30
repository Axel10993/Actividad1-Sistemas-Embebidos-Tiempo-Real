# Práctica 1 – Doble contador BCD con FreeRTOS (variante C++)

**Materia:** Sistemas Embebidos en Tiempo Real · Ingeniería en Electrónica y Control de Sistemas de Aeronaves (IECSA) · UNAQ
**Plataforma:** ESP32-S3-DevKitC-1 · ESP-IDF · PlatformIO
**Repositorio:** <https://github.com/Axel10993/Actividad1-Sistemas-Embebidos-Tiempo-Real>

## Integrantes del equipo

| # | Nombre completo | Matrícula |
|---|---|---|
| 1 | Axel Ruiz Cuervo | 10993 |
| 2 | Fatima Jimenez Jacobo | 11021 |
| 3 | Paola Yoselin Flores Montes | 10086 |

---

## 1. Objetivo

Implementar dos contadores BCD (0–9) independientes, mostrados en dos displays de 7 segmentos, sobre FreeRTOS. Cada tarea recibe sus datos por `pvParameters`, y un **TaskManager** centraliza la lógica: es el único que interpreta los botones, actualiza la configuración y suspende/reanuda los contadores mediante `TaskHandle_t`. No se usan queues, semáforos, mutex, Event Groups ni Software Timers.

## 2. Hardware y pines

Displays de cátodo común con un NPN en cada común (segmentos compartidos). Botones a GND con pull-up interno.

| Señal | GPIO | Señal | GPIO |
|---|---|---|---|
| SEG_A | 4 | DISPLAY_1_EN | 18 |
| SEG_B | 5 | DISPLAY_2_EN | 8 |
| SEG_C | 6 | BTN_START_PAUSE | 9 |
| SEG_D | 7 | BTN_DIRECTION | 10 |
| SEG_E | 15 | BTN_SPEED | 11 |
| SEG_F | 16 | BTN_MODE | 12 |
| SEG_G | 17 | | |

Se evitaron los pines de strapping (0, 3, 45, 46), USB (19, 20), UART0 (43, 44), flash, PSRAM octal, LED RGB y JTAG.

## 3. Arquitectura del software

```
include/                          src/
  app_config.hpp   (pines/tiempos)  main.cpp          (app_main limpio)
  system_state.hpp (enums/structs)  system_state.cpp  (init + toString)
  counter.hpp      (BcdCounter)     counter.cpp       (BcdCounter + counterTask)
  display.hpp      (Display 7seg)   display.cpp       (Display + displayRefreshTask)
  buttons.hpp      (botones)        buttons.cpp       (buttonTask)
  app_tasks.hpp                     app_tasks.cpp     (TaskManager + creación de tareas)
```

| Elemento | Implementación |
|---|---|
| Namespace | `practica1` y `practica1::config` (pines y tiempos) |
| Clases | `BcdCounter` (lógica 0–9), `SevenSegmentDisplay` (GPIO de un display) |
| `struct` | `CounterConfig`, `ButtonEvent`, `SystemState`, `AppContext`, `ButtonTaskParams` |
| `enum class` | `CountDirection`, `CouplingMode`, `RunState`, `SpeedMode`, `ButtonId` |
| `app_main()` | Solo inicializa el contexto (`systemStateInit`) y crea las tareas (`createApplicationTasks`) |

## 4. Resumen de tareas (pvParameters y TaskHandle_t)

| Tarea | Función | Prio | Stack | `pvParameters` | Handle guardado |
|---|---|---|---|---|---|
| Counter1 | `counterTask` | 2 | 3072 | `&c.counter1` (`CounterConfig*`) | `counter1Handle` |
| Counter2 | `counterTask` | 2 | 3072 | `&c.counter2` (`CounterConfig*`) | `counter2Handle` |
| BtnStart | `buttonTask` | 2 | 3072 | `&startParams` (`ButtonTaskParams*`) | no |
| BtnDirection | `buttonTask` | 2 | 3072 | `&directionParams` | no |
| BtnSpeed | `buttonTask` | 2 | 3072 | `&speedParams` | no |
| BtnMode | `buttonTask` | 2 | 3072 | `&modeParams` | no |
| DisplayRefresh | `displayRefreshTask` | 2 | 3072 | `&c` (`AppContext*`) | no |
| TaskManager | `taskManager` | 3 | 3072 | `&c` (`AppContext*`) | `managerHandle` |

> La práctica incluye 7 tareas (2 contadores, 4 botones y el TaskManager). Se agregó `DisplayRefresh` como octava tarea porque los displays comparten las líneas de segmentos y deben multiplexarse continuamente; así los contadores solo modifican su valor y nunca escriben GPIO.

## 5. Comportamiento implementado

| Modo | Dirección base (D1) | Display 1 | Display 2 |
|---|---|---|---|
| OPPOSITE | UP | UP | DOWN |
| OPPOSITE | DOWN | DOWN | UP |
| SAME | UP | UP | UP |
| SAME | DOWN | DOWN | DOWN |

- **Start/Pause:** el TaskManager alterna `RunState` y llama `vTaskResume()` o `vTaskSuspend()` sobre `counter1Handle` y `counter2Handle`. Cada contador conserva su valor y su dirección.
- **Dirección / Velocidad / Modo:** solo se procesan en `RUNNING`. En `PAUSED` el evento se descarta y se registra en UART.
- **Velocidad:** alterna entre 500 ms (`SLOW`) y 250 ms (`FAST`).
- **OPPOSITE → SAME:** el TaskManager suspende ambos contadores, ajusta los valores (DOWN: el menor de los dos; UP: el mayor), aplica la configuración y los reanuda.
- **Botones:** cada `buttonTask` muestrea cada 20 ms (antirrebote simple), detecta el flanco de pulsación y publica `pending = true`.

## 6. Cuestionario

**1. ¿Por qué Counter1 y Counter2 pueden ejecutar la misma función `counterTask()` y comportarse diferente?**
Porque el código es uno solo, pero cada tarea tiene su propia pila, sus variables locales (por ejemplo, su `BcdCounter`) y, sobre todo, su propio puntero `pvParameters`. Counter1 recibe `&c.counter1` y Counter2 recibe `&c.counter2`; cada `CounterConfig` tiene distinto `value`, `direction` y `displayId`, así que la misma función opera sobre datos distintos.

**2. ¿Qué información recibe cada tarea mediante `pvParameters`?**
Las tareas de conteo reciben un `CounterConfig*` (`value`, `direction`, `periodMs`, `displayId`). Cada tarea de botón recibe un `ButtonTaskParams*` (el `pin` y el puntero al `ButtonEvent` que debe publicar). `DisplayRefresh` y `TaskManager` reciben el `AppContext*` completo.

**3. ¿Qué representa un `TaskHandle_t` y por qué el Task Manager necesita conservarlo?**
Es una referencia opaca al bloque de control de la tarea (TCB) que crea el kernel. `xTaskCreate` lo devuelve por su último parámetro. El TaskManager lo necesita para controlar tareas ajenas desde fuera: `vTaskSuspend()`, `vTaskResume()` y `eTaskGetState()`. Sin el handle no hay forma de identificar a qué tarea aplicar la operación.

**4. ¿Qué diferencia existe entre BLOCKED y SUSPENDED en esta práctica?**
BLOCKED es un estado que la propia tarea provoca y que termina solo: los contadores pasan la mayor parte del tiempo en `vTaskDelay()` y el tick del kernel los despierta al vencer el tiempo. SUSPENDED lo impone otra tarea (el TaskManager) con `vTaskSuspend()`; no tiene timeout y la tarea no se planifica hasta que alguien llame `vTaskResume()`. En `RUNNING` los contadores se ven como BLOCKED/READY; en `PAUSED` se ven como SUSPENDED.

**5. ¿Qué ocurre con `vTaskDelay()` cuando una tarea es suspendida?**
La tarea sale de la lista de tareas retrasadas y el tiempo restante del retardo se descarta. Al reanudarla pasa a READY y `vTaskDelay()` retorna de inmediato, de modo que el contador da su siguiente paso casi en cuanto se reanuda, y no después de un periodo completo. Conviene verificarlo en hardware y mencionarlo en la documentación, porque afecta la sensación de "continuar desde el último valor".

**6. ¿Qué ventaja aporta `enum class` frente a constantes enteras para representar estados?**
Tipado fuerte: un `CountDirection` no se puede mezclar por accidente con un `RunState` ni con un entero, y el compilador marca el error. Además los nombres quedan dentro de su ámbito (`CountDirection::Up`), sin colisiones, y el código se lee mejor. Esto se complementa con las sobrecargas `toString()` para los logs.

**7. ¿Qué responsabilidad tiene `BcdCounter` y cuál `SevenSegmentDisplay`?**
`BcdCounter` solo contiene la lógica del conteo 0–9 (`set`, `step` con desbordamiento circular y `value`); no sabe nada de GPIO ni de FreeRTOS, por lo que puede probarse sin RTOS. `SevenSegmentDisplay` solo se encarga del hardware de un display: inicializa los pines, traduce un dígito a segmentos y habilita o apaga el display (`init`, `show`, `blank`).

**8. ¿Por qué `volatile` no resuelve por sí solo los problemas de concurrencia?**
`volatile` solo evita que el compilador guarde la variable en un registro y elimine lecturas o escrituras. No da atomicidad en operaciones compuestas (leer-modificar-escribir), no garantiza coherencia entre varios campos que deben cambiar juntos y no ofrece orden de memoria ni exclusión mutua entre núcleos. En el ESP32-S3, que tiene dos núcleos, dos tareas pueden correr realmente en paralelo.

**9. ¿Qué cambiaría en el diseño cuando posteriormente se permitan queues o semáforos?**
- Los flags `ButtonEvent.pending` se sustituirían por una queue de eventos de botón; el TaskManager se bloquearía esperando en la queue en lugar de sondear cada 10 ms.
- El `AppContext` compartido se protegería con un mutex, o mejor aún, el TaskManager enviaría comandos a los contadores por queue.
- Start/Pause podría hacerse con notificaciones de tarea o Event Groups, sin depender de `vTaskSuspend()`, y los contadores podrían usar `vTaskDelayUntil()` para un periodo más estable.
- El antirrebote podría moverse a interrupciones GPIO que envíen el evento a la queue.

**10. Si ambas tareas tienen la misma prioridad, ¿cómo interviene el scheduler de FreeRTOS?**
Las tareas READY de la misma prioridad se reparten el CPU por turnos (round-robin con *time slicing*) en cada tick. En esta práctica casi todas las tareas pasan su tiempo bloqueadas en `vTaskDelay()`, así que rara vez agotan su rebanada. Como el TaskManager tiene prioridad 3, desplaza a las demás apenas está listo. Además, en ESP-IDF (FreeRTOS SMP) las tareas sin afinidad pueden ejecutarse simultáneamente en ambos núcleos.

---

## 7. Reflexión

**Qué resolvió `pvParameters`.**
Permitió reutilizar una sola función de tarea con datos distintos: `counterTask` para dos contadores y `buttonTask` para cuatro botones. Con ello no se duplicó código ni se necesitaron variables globales por instancia; cada tarea recibe exactamente lo que necesita (su configuración, su pin y su evento).

**Qué resolvió `TaskHandle_t`.**
Dio al TaskManager una forma de controlar el ciclo de vida de los contadores desde fuera: pausar y reanudar ambos de manera simultánea con `vTaskSuspend()`/`vTaskResume()` y consultar su estado con `eTaskGetState()`. Con esto el control queda centralizado en una sola tarea y los contadores no necesitan saber que existe una pausa.

**Qué riesgo queda al no usar mecanismos de sincronización.**
La comunicación se hace con memoria compartida (`AppContext`) y variables `volatile`, y eso deja condiciones de carrera posibles:

- **Flags de botón:** la tarea de botón escribe `pending = true` y el TaskManager la limpia. Si una pulsación llega entre la lectura y la limpieza, puede perderse el evento.
- **Campos relacionados:** `direction`, `periodMs` y `value` se actualizan por separado, sin garantía de que un contador los lea como un conjunto coherente.
- **Suspensión no instantánea:** el TaskManager suspende los contadores antes de ajustar sus valores en el cambio a SAME, pero con dos núcleos un contador que corre en el otro núcleo podría terminar un paso en curso justo después del ajuste.
- **`volatile` no es sincronización:** garantiza lecturas frescas, no atomicidad ni orden entre núcleos.

En esta práctica el riesgo es bajo (datos de 8/32 bits, un solo escritor por campo en la mayoría de los casos y tiempos lentos), pero en un sistema real con datos de mayor tamaño o más productores se necesitarían queues, mutex o notificaciones.

## 8. Conclusión técnica individual

### Axel Ruiz Cuervo (10993)

La práctica permitió aplicar FreeRTOS con una arquitectura modular en C++: `namespace`, clases con responsabilidad única (`BcdCounter` para la lógica y `SevenSegmentDisplay` para el hardware), `struct` para la configuración compartida y `enum class` para los estados. El uso de `pvParameters` hizo posible ejecutar la misma función para dos contadores y cuatro botones sin duplicar código. Durante las pruebas en hardware el principal reto fue el multiplexado de los displays: al compartir las líneas de segmentos, cualquier retardo en la tarea de refresco producía parpadeo visible, por lo que se ajustó el periodo de refresco y se verificó la polaridad de la habilitación de cada cátodo respecto al transistor NPN. También fue necesario revisar el antirrebote, ya que los pulsadores mecánicos generaban varias lecturas por pulsación antes de estabilizarse.

### Fatima Jimenez Jacobo (11021)

`TaskHandle_t` me permitió ver la diferencia práctica entre BLOCKED y SUSPENDED: con `eTaskGetState()` en UART se observa cómo Counter1 y Counter2 cambian a SUSPENDED al pausar, y el TaskManager concentra todo el control sin que los contadores sepan que existe una pausa. También comprobé que los botones a GND con pull-up interno requieren revisar el cableado: en el código se detecta cuando un pin lee 0 en reposo de forma permanente, lo que indica que falta el pull-up o que el pin está mal conectado. Los pines 9 y 10, al estar próximos a otras señales conmutadas, necesitaron revisarse para descartar ruido, y el antirrebote por muestreo cada 20 ms resultó suficiente para eliminar las pulsaciones falsas.

### Paola Yoselin Flores Montes (10086)

La parte más interesante fue la ausencia de mecanismos de sincronización: la comunicación entre botones, TaskManager, contadores y display depende de memoria compartida con variables `volatile`. Funciona porque los datos son pequeños y casi cada campo tiene un solo escritor, pero es frágil. En el cambio de OPPOSITE a SAME fue necesario suspender los contadores antes de ajustar sus valores para evitar escrituras simultáneas, y aun así se comprobó que la suspensión no es instantánea cuando un contador corre en el otro núcleo. También verifiqué que los valores iniciales y los modos por defecto (dirección UP, modo OPPOSITE, velocidad SLOW) deben quedar fijados antes de crear las tareas, ya que de lo contrario arrancan con basura. 

---

## 9. Cómo compilar y ejecutar

```bash
pio run                # compilar
pio run -t upload      # cargar al ESP32-S3
pio device monitor     # monitor serie a 115200
```
