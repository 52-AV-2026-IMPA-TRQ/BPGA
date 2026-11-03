# BPGA

Banco de Prueba de Generadores Aeronáuticos basado en EDU-CIAA-NXP, FreeRTOS, RS-485 y un gateway ESP32.

## Estado actual

- Compilación y programación de EDU-CIAA-NXP validadas.
- FreeRTOS ejecutándose sobre Cortex-M4F.
- Simulador determinista de RPM, tensión y corriente en desarrollo.
- ESP32 elegido como gateway Modbus RTU a HTTP REST/SSE.
- Raspberry Pi 3B elegida como terminal Linux en modo kiosco.

## Páginas

- [Arquitectura](Architecture.md)
- [Modelo de mediciones](Measurement-model.md)
- [Desarrollo](Development.md)
- [Hoja de ruta](Roadmap.md)
