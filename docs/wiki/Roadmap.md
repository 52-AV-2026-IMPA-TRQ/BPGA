# Hoja de ruta (documento histórico/reemplazado)

> **Clasificación: histórico o reemplazado.** La secuencia de implementación que sigue refleja una planificación anterior: varios hitos se completaron y la propuesta de gateway ESP32 ya no representa la arquitectura vigente. Se conserva como antecedente, no como plan aprobado. Consulta [desarrollo futuro](Future-development.md) y [diseños históricos](Historical-designs.md).

1. Simulador determinista y modelo común de mediciones.
2. Mapa de registros Modbus RTU.
3. Esclavo Modbus RTU sobre el RS-485 de la EDU-CIAA.
4. Pruebas desde Linux con adaptador USB-RS485.
5. Gateway ESP32 con SP3485 de 3,3 V.
6. API HTTP REST y telemetría SSE.
7. Interfaz web y Raspberry Pi 3B en modo kiosco.
8. Integración gradual de sensores reales de RPM, tensión y corriente.
