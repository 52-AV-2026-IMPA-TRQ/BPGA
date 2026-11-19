# Modelo de mediciones (parcialmente vigente)

> **Clasificación: parcialmente vigente.** RPM, tensión en mV, corriente en mA, secuencia, marca temporal, fuente simulada y fases del simulador existen en el firmware. No todos los metadatos de este modelo se publican o calculan actualmente por Modbus; no interpretes este texto como especificación de un sensor físico. Consulta [Modo Simulador](Simulation-mode.md) y [Protocolo Modbus RTU](Protocolo-Modbus-RTU.md).

## Magnitudes básicas

- Velocidad: RPM, entero sin signo.
- Tensión de salida: milivoltios (`mV`), entero sin signo.
- Corriente de salida: miliamperios (`mA`), entero sin signo.

## Metadatos

Cada muestra incluye secuencia, tiempo desde arranque, origen real/simulado, estado del simulador y banderas de validez.

## Simulación inicial

El ciclo dura 40 segundos y recorre: detenido, aceleración, marcha sin carga, aplicación de carga, régimen estable, descarga y desaceleración. Los valores son deterministas para permitir pruebas reproducibles.
