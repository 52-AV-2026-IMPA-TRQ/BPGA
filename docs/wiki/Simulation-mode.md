# Modo Simulador

## Finalidad

El simulador proporciona una secuencia repetible para desarrollar y comprobar la ruta de datos —firmware, snapshot Modbus, supervisor, API e interfaz— sin presentar muestras como mediciones físicas.

## Recorrido de datos

1. La tarea del simulador genera una muestra cada 20 ms y la escribe en una cola de longitud uno (se conserva la más reciente).
2. La tarea de medición publica la muestra como snapshot protegido por mutex.
3. El controlador actualiza estado, fase, máximos y contador de muestras.
4. El esclavo Modbus expone el snapshot; la Raspberry Pi lo sondea cada 200 ms.
5. La API y Chromium presentan el dato y el estado de comunicación.

## Ciclo programado

El ciclo completo dura 40 s y vuelve al inicio:

| Tiempo relativo | Fase |
|---:|---|
| 0–2 s | Detenido |
| 2–8 s | Acelerando |
| 8–12 s | Sin carga |
| 12–18 s | Aplicando carga simulada |
| 18–28 s | Estable |
| 28–32 s | Retirando carga |
| 32–38 s | Desacelerando |
| 38–40 s | Detenido |

Las muestras de referencia del código simulan hasta 1500 rpm, 220000 mV y 3400 mA de corriente de salida del generador. Esos valores son constantes del modelo didáctico, no límites operativos ni consignas para equipos reales. El simulador etiqueta la fuente como simulada y marca válidas las magnitudes según la lógica del modelo.

## Límites

- No existe en esta ruta una realimentación desde sensores físicos.
- La corriente simulada representa corriente de salida del generador, no corriente del motor de accionamiento ni corriente de excitación.
- El simulador no mueve un motor, no energiza un generador, no acciona cargas y no valida regulación o protecciones.
- El estado de controlador `Listo` se alcanza tras una demora fija de 1 s; el código lo identifica como temporal, pendiente de verificaciones e interbloqueos reales.
- Los máximos de datos simulados no son resultados de ensayos físicos.

Implementación: [simulador](../../app/src/bpga_simulator.c), [modelo de medición](../../app/inc/bpga_measurement.h), [controlador](../../app/src/bpga_controller.c).
