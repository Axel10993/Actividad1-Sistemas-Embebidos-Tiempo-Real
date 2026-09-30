# Práctica 1 – Doble contador BCD con FreeRTOS (variante C++)

ESP32-S3-DevKitC-1 · ESP-IDF · PlatformIO

## Arquitectura
- `namespace practica1` (y `practica1::config` para pines/tiempos).
- Clases: `BcdCounter` (lógica 0-9) y `SevenSegmentDisplay` (GPIO de un display).
- `struct`: `CounterConfig`, `ButtonEvent`, `SystemState`, `AppContext`, `ButtonTaskParams`.
- `enum class`: `CountDirection`, `CouplingMode`, `RunState`, `SpeedMode`, `ButtonId`.
- Tareas: Counter1/Counter2 (misma `counterTask`), 4 botones (misma `buttonTask`),
  `displayRefreshTask` y `taskManager`.
- El TaskManager guarda los `TaskHandle_t` de Counter1/Counter2 y usa
  `vTaskSuspend()` / `vTaskResume()`.
- Sin queues, semáforos, mutex, Event Groups ni Software Timers.

## Pines
| Señal | GPIO | Señal | GPIO |
|---|---|---|---|
| SEG_A | 4  | DISPLAY_1_EN | 18 |
| SEG_B | 5  | DISPLAY_2_EN | 8  |
| SEG_C | 6  | BTN_START_PAUSE | 9  |
| SEG_D | 7  | BTN_DIRECTION   | 10 |
| SEG_E | 15 | BTN_SPEED       | 11 |
| SEG_F | 16 | BTN_MODE        | 12 |
| SEG_G | 17 | | |

Displays de cátodo común con NPN en cada común. Botones a GND (pull-up interno).

## Comportamiento
| Modo | Dirección base | Display 1 | Display 2 |
|---|---|---|---|
| OPPOSITE | UP   | UP   | DOWN |
| OPPOSITE | DOWN | DOWN | UP   |
| SAME     | UP   | UP   | UP   |
| SAME     | DOWN | DOWN | DOWN |

Al pasar de OPPOSITE a SAME ambos displays se igualan al dígito menor (DOWN)
o mayor (UP). En pausa se ignoran Dirección, Velocidad y Modo.
