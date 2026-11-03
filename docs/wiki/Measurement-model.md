# Modelo de mediciones

## Magnitudes básicas

- Velocidad: RPM, entero sin signo.
- Tensión de salida: milivoltios (`mV`), entero sin signo.
- Corriente de salida: miliamperios (`mA`), entero sin signo.

## Metadatos

Cada muestra incluye secuencia, tiempo desde arranque, origen real/simulado, estado del simulador y banderas de validez.

## Simulación inicial

El ciclo dura 40 segundos y recorre: detenido, aceleración, marcha sin carga, aplicación de carga, régimen estable, descarga y desaceleración. Los valores son deterministas para permitir pruebas reproducibles.
