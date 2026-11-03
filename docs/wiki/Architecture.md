# Arquitectura

## Capas

1. **EDU-CIAA-NXP**: adquisición real o simulada, validación, alarmas, secuencia de ensayo y esclavo Modbus RTU.
2. **ESP32**: maestro Modbus RTU, caché de datos, API HTTP REST y telemetría SSE.
3. **Raspberry Pi 3B**: navegador Linux en modo kiosco. Puede sustituirse por otro ordenador o tableta.

## Flujo

```text
Sensores o simulador
        |
EDU-CIAA / FreeRTOS
        |
Modbus RTU sobre RS-485
        |
ESP32 gateway HTTP
        |
Wi-Fi
        |
Raspberry Pi 3B / navegador kiosco
```

La protección y el control temporal permanecen en la EDU-CIAA. La pérdida del gateway o de la pantalla no debe detener la lógica local del banco.
