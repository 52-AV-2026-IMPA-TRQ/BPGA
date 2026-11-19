# Firmware EDU-CIAA

## Organización

El firmware inicializa la placa, el módulo de medición, el controlador y el esclavo Modbus. Luego crea las tareas y arranca FreeRTOS. Si falla la inicialización o la creación de una tarea, entra en un estado de error señalado por el LED rojo intermitente.

El período de tick configurado es 1 ms. Las prioridades siguientes son los desplazamientos sobre `tskIDLE_PRIORITY` observados en el código:

| Tarea | Prioridad relativa | Comportamiento confirmado |
|---|---:|---|
| `BPGA_MODBUS` | +5 | Recibe tramas por RS-485, detecta silencio de trama, valida CRC y atiende FC04. |
| `BPGA_SIM` | +4 | Genera y publica una medición simulada cada 20 ms. |
| `BPGA_CTRL` | +3 | Actualiza el snapshot de control cada 20 ms y acumula máximos y muestras. |
| `BPGA_DATA` | +2 | Recibe la cola de muestras y actualiza el snapshot compartido. |
| `BPGA_LED` | +1 | Alterna el LED azul cada 500 ms como latido. |
| `BPGA_GPS` | +1 | Lee UART3, valida RMC y trata de sincronizar el RTC. |

El kernel se configura con siete niveles de prioridad (`configMAX_PRIORITIES = 7`), mutexes, colas, preempción y detección de desbordamiento de pila nivel 2.

## Módulos

- `app.c`: inicialización de placa y creación de tareas.
- `bpga_simulator.c`: ciclo simulado de 40 s; no es una adquisición física.
- `bpga_measurement.c`: cola de longitud uno y snapshot de medición protegido con mutex.
- `bpga_controller.c`: snapshot de estado; pasa de inicializando a listo tras la demora fija de 1 s.
- `bpga_modbus_slave.c`: servidor Modbus RTU sobre UART RS-485.
- `bpga_gps.c`: recepción RMC y posible escritura/verificación del RTC.

## Estado y límites de control

El firmware actual implementa la lectura del snapshot y estados básicos del simulador. No implementa en esta ruta un controlador operativo del motor, excitación, banco de cargas ni PLC. Aunque algunas estructuras y posiciones del mapa describen campos de perfiles, control, alarmas o consignas, su presencia en el esquema no demuestra que esas funciones estén implementadas.

El comentario del controlador declara explícitamente temporales la transición automática a listo y reserva la parada segura para una futura implementación de verificaciones e interbloqueos. Por tanto, no atribuir al firmware las protecciones del banco.

Referencias: [arranque y tareas](../../app/src/app.c), [controlador](../../app/src/bpga_controller.c), [configuración FreeRTOS](../../app/inc/FreeRTOSConfig.h).
