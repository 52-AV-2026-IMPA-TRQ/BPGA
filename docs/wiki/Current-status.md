# Estado actual

## Resumen

El estado descrito es un demostrador basado en EDU-CIAA, FreeRTOS, Modo Simulador, Modbus RTU/RS-485 y un supervisor Raspberry Pi con API HTTP local y Chromium en kiosco. No es un banco de prueba certificado ni una etapa de potencia validada.

| Clasificación | Elemento | Estado y límites |
|---|---|---|
| Implementado y validado | Firmware EDU-CIAA y FreeRTOS | Tareas de medición, controlador, simulador, Modbus, GPS y latido creadas en el firmware. |
| Implementado y validado | Modo Simulador | Ciclo determinista de 40 s con RPM, tensión y corriente simuladas. No mide magnitudes físicas. |
| Implementado y validado | Modbus RTU / RS-485 | Esclavo 1, 19200 bit/s, 8E1; FC04 y lectura de snapshot desde 0x0010 por 32 registros. |
| Implementado y validado | Supervisor Raspberry Pi | Sondeo serial a 5 Hz, caché de estado, API HTTP local en puerto 8080 y servicio `bpga.service`. |
| Implementado y validado | Interfaz | Cuatro pantallas, modo Chromium kiosco, indicación de datos congelados y recuperación automática de la lectura al restablecerse el enlace. |
| Implementado y validado | Prueba integral de software | Evidencia de 30 minutos aprobada; valida continuidad de API/web y recepción de datos simulados, no la operación del banco electromecánico. |
| En integración / validación | GPS L86 y RTC | Conectado a USART3 a 9600 bit/s, 8N1; UART y mensajes RMC recibidos. Sincronizar el RTC con una RMC válida sigue pendiente. |
| Futuro | Sensores físicos y adquisición completa | RPM, tensión y corriente físicos todavía no constituyen una medición completa validada. |
| Futuro | Motor, generador y potencia | La etapa electromecánica y de potencia requiere diseño e integración futuros. No está validada ni se documenta su energización aquí. |
| Futuro | Regulación, cargas, PLC y protecciones | Regulador de excitación, banco de cargas, PLC y protecciones para operación real no son funciones operativas actuales. |
| Histórico o reemplazado | Gateway ESP32 como arquitectura principal | Fue una propuesta/alternativa previa. No pertenece al flujo vigente. Que exista código en el repositorio no acredita que opere como gateway del sistema actual. |
| Histórico o reemplazado | ADS1115, Arduino Uno y Raspberry Pi Pico | Alternativas previas, no integrantes de la arquitectura vigente. |

## Documentos existentes auditados

| Documento | Clasificación | Observación |
|---|---|---|
| [Home](Home.md) | Vigente | Portada y navegación actualizadas. |
| [Architecture](Architecture.md) | Vigente | Refleja el recorrido actual, sin gateway ESP32 principal. |
| [Architecture-historica](Architecture-historica.md) | Histórico o reemplazado | Preserva la propuesta anterior con advertencia de que no describe el estado actual. |
| [Protocolo-Modbus-RTU](Protocolo-Modbus-RTU.md) | Vigente | Reducido a lo respaldado por el esclavo EDU-CIAA y el cliente Raspberry Pi. |
| [Measurement-model](Measurement-model.md) | Parcialmente vigente | Magnitudes y estados simulados existen; no todos los metadatos están expuestos por el protocolo actual. |
| [Development](Development.md) | Histórico o reemplazado | Conservado, pero sus instrucciones son de una etapa previa. |
| [Roadmap](Roadmap.md) | Histórico o reemplazado | Conservado como planificación anterior; no equivale al plan vigente. |
| [Protocolo-Modbus-RTU-historico](Protocolo-Modbus-RTU-historico.md) | Histórico o reemplazado | Se preserva la propuesta anterior separada de la especificación vigente. |

Las páginas nuevas de esta Wiki documentan el estado vigente, la integración en curso, las propuestas futuras y los antecedentes por separado. Ningún documento se deja pendiente de reescritura en esta reorganización.

## Referencias de implementación y evidencia

- [Firmware principal](../../app/src/app.c)
- [Servidor Raspberry Pi](../../raspberry-pi/app/bpga_server.py)
- [Resumen corregido de la prueba de 30 minutos](../../evidencias/prueba-integral-30m/resultado_corregido.txt)
- [Alcance de pruebas y limitaciones](Testing-and-validation.md)
