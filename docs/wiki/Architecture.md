# Arquitectura vigente

## Flujo real implementado

```text
Simulador de mediciones
        ↓
EDU-CIAA + tareas FreeRTOS
        ↓ esclavo Modbus RTU, RS-485
Raspberry Pi + sondeo serie a 5 Hz
        ↓ API HTTP local, puerto 8080
Chromium en modo kiosco
```

La EDU-CIAA genera el snapshot simulado y responde las lecturas Modbus. La Raspberry Pi consulta el bloque del snapshot, valida trama y secuencia, conserva el estado más reciente en memoria y lo publica en la API HTTP. Chromium solicita `/api/status` y presenta las pantallas de supervisión.

## Responsabilidades

| Componente | Responsabilidad vigente |
|---|---|
| EDU-CIAA | Ejecuta FreeRTOS, genera mediciones simuladas, mantiene el snapshot y atiende Modbus RTU. |
| Enlace RS-485 | Lleva solicitudes Modbus del supervisor y respuestas del esclavo. |
| Raspberry Pi | Sondea a 5 Hz, valida la respuesta, mantiene la caché y sirve la API HTTP local y la página. |
| Chromium | Presenta la página web en modo kiosco; no ejecuta el lazo de adquisición. |
| GPS L86 | Integración en curso por USART3; aún no se publica en el mapa Modbus. |

No hay un ESP32 como gateway principal en el flujo actual. La arquitectura tampoco incluye adquisición física completa, motor, generador energizado, cargas, PLC, regulador de excitación ni etapa de potencia validada. Consulta [estado actual](Current-status.md), [supervisor](Raspberry-Pi-supervisor.md) y [antecedentes históricos](Historical-designs.md).

## Límites

El estado y la medición presentados como simulados no prueban comportamiento electromecánico. La pérdida de la comunicación entre Pi y EDU-CIAA se señala en la interfaz; no se debe interpretar el sistema como una protección certificada o como control de seguridad de un banco energizado.

## Implementación de referencia

- [Arranque y creación de tareas FreeRTOS](../../app/src/app.c)
- [Simulador](../../app/src/bpga_simulator.c)
- [Esclavo Modbus](../../app/src/bpga_modbus_slave.c)
- [Supervisor y API](../../raspberry-pi/app/bpga_server.py)
- [Servicio systemd](../../raspberry-pi/systemd/bpga.service)
- [Interfaz web](../../raspberry-pi/web/index.html)
