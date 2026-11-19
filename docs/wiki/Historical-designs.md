# Diseños históricos y alternativas

Esta página clasifica propuestas anteriores. Ninguna de las opciones siguientes debe confundirse con la arquitectura vigente, que es EDU-CIAA → Modbus RTU/RS-485 → Raspberry Pi → API HTTP local → Chromium kiosco.

| Alternativa o idea | Clasificación | Estado/documentación |
|---|---|---|
| ESP32 como gateway principal Modbus a HTTP REST/SSE | Histórico o reemplazado | Aparece en la arquitectura y planificación antiguas. No es el gateway principal actual. La existencia de una carpeta o código ESP32 en el repositorio no acredita que forme parte del despliegue vigente. |
| ADS1115 | Histórico o reemplazado | Alternativa previamente considerada, no parte del flujo actual. Detalles de diseño/validación pendientes de confirmación. |
| Arduino Uno | Histórico o reemplazado | Alternativa previamente considerada, no parte del flujo actual. Detalles de diseño/validación pendientes de confirmación. |
| Raspberry Pi Pico | Histórico o reemplazado | Alternativa previamente considerada, no parte del flujo actual. Detalles de diseño/validación pendientes de confirmación. |
| PLC | Futuro/no operativo | No hay un PLC operativo en la arquitectura actual. La planificación de su incorporación está pendiente. |
| Regulador de excitación, banco de cargas y etapa de potencia | Futuro/no operativo | No están implementados/validados como parte del BPGA descrito. |

Los documentos previos se preservan y etiquetan:

- [Arquitectura previa](Architecture-historica.md) se conserva como antecedente; sus afirmaciones de control/protección no están verificadas y no describen la arquitectura o las protecciones actuales.
- [Propuesta Modbus antigua](Protocolo-Modbus-RTU-historico.md) se conserva íntegra con aviso de que no es la especificación vigente.
- [Hoja de ruta antigua](Roadmap.md) y [guía de desarrollo antigua](Development.md) siguen disponibles como antecedentes, no como instrucciones actuales.
- [Modelo de mediciones](Measurement-model.md) es parcialmente vigente: conserva magnitudes y estados simulados, pero no define por completo la interfaz actual.

Los detalles comparativos, fechas de selección y resultados técnicos de ADS1115, Arduino Uno y Raspberry Pi Pico no pueden verificarse en la documentación inspeccionada y quedan pendientes de confirmación; no se infieren aquí.
