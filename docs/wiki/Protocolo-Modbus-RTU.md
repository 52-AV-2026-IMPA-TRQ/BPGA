# Protocolo Modbus RTU vigente

## Alcance verificado

Esta página sustituye la propuesta anterior, preservada en [Protocolo-Modbus-RTU-historico.md](Protocolo-Modbus-RTU-historico.md). Documenta solo la comunicación que el firmware y el supervisor actuales usan: lectura de un snapshot simulado. No es una especificación de escritura de consignas o control del banco.

## Enlace serial

| Parámetro | Valor observado |
|---|---|
| Medio | RS-485 |
| Rol EDU-CIAA | Esclavo |
| Dirección | 1 |
| Velocidad | 19200 bit/s |
| Trama serie | 8 bits, paridad par, 1 bit de parada (8E1) |
| Función usada | FC04, Read Input Registers |

## Solicitud vigente

La Raspberry Pi consulta **32 registros desde la dirección PDU `0x0010`**, es decir, el bloque `0x0010–0x002F`. Envía una solicitud FC04 con esclavo 1 cada 200 ms (5 Hz). El firmware verifica el CRC Modbus, dirección, función y rango; rechaza una función no implementada con excepción Modbus.

Los registros se serializan con el byte alto primero. La secuencia de inicio y fin del snapshot coincide; el receptor comprueba esa igualdad y que el valor no sea impar antes de aceptarlo. El cliente deriva la secuencia lógica desplazando el valor recibido un bit a la derecha.

## Mapa del snapshot usado

Las direcciones de abajo son direcciones PDU basadas en cero; no referencias 3xxxx/4xxxx.

| Dirección PDU | Campo | Representación en el mapa | Uso/observación actual |
|---|---|---|---|
| `0x0010` | Secuencia de inicio | uint16, secuencia lógica desplazada 1 bit | Delimita el snapshot. |
| `0x0011–0x0012` | Estado y subestado | uint16 | El estado viene del controlador; el subestado sigue la fase simulada. |
| `0x0013–0x0014` | Flags de estado y control | uint16 | Los flags implementados identifican inicialización, listo, ensayo, simulación y validez según el código. |
| `0x0015–0x0017` | Perfil y motivo de transición | uint16 | Campos reservados por estructura; no indican una función de perfiles implementada. |
| `0x0018–0x0019` | Uptime | uint32, palabra alta primero | Derivado de la marca temporal de la muestra. |
| `0x001A–0x001B` | Tiempo de lazo | uint16 cada uno | El firmware actual no alimenta estas métricas; permanecen en cero. |
| `0x001C–0x001F` | Reservado | uint16 | Cero en la imagen actual. |
| `0x0020–0x0021` | RPM | uint32, palabra alta primero | El simulador genera valores que caben en la palabra baja. |
| `0x0022–0x0023` | Tensión | uint32, mV | Valor simulado; palabra alta primero. |
| `0x0024–0x0025` | Corriente | uint32, mA | Corriente de salida simulada; palabra alta primero. |
| `0x0026–0x0027` | Potencia | uint32, W | Calculada a partir de tensión y corriente de la muestra. |
| `0x0028–0x002C` | Excitación, carga y calidades | uint16 | El firmware actual no los alimenta; permanecen en cero. |
| `0x002D` | Edad de muestra | uint16, ms | Calculada por el firmware y saturada a `0xFFFF`. |
| `0x002E` | Flags de medición | uint16 | No se publica actualmente una máscara completa; la imagen queda en cero. |
| `0x002F` | Secuencia de fin | uint16 | Debe coincidir con `0x0010`. |

## Campos publicados y advertencias del cliente

El supervisor convierte tensión, corriente y potencia a enteros de 32 bits; RPM se toma de la palabra baja. La API coloca los valores en JSON y expone los flags de estado, pero no entrega un campo independiente de origen de cada muestra. En el modo vigente, los valores proceden del simulador.

Hay una discrepancia que debe tenerse presente: el parser Python lee `sampleAgeMs` con un helper de 32 bits empezando en el índice 28 del bloque. En el mapa, la edad es el único registro `0x002D` (índice 29 del bloque) y el índice 28 corresponde a calidad de corriente. En el modo actual esa calidad vale cero, por lo que la edad puede coincidir incidentalmente; no asumir que el campo API es correcto para otros valores hasta corregir/verificar el mapeo. La corrección del código queda fuera del alcance de esta reorganización documental.

## No implementado como parte del protocolo actual

- FC03, FC06 y FC16 para configuración o escritura.
- Comandos operativos, perfiles cargables, consignas, reconocimientos o resets remotos.
- Mapas publicados de alarmas, diagnóstico completo, sensor GPS o RTC.
- Confirmación de un handshake/versionado como el propuesto en el documento histórico.

La presencia de direcciones/campos en estructuras o documentos previos no acredita su soporte actual. Fuentes: [esclavo Modbus EDU-CIAA](../../app/src/bpga_modbus_slave.c), [constantes del esclavo](../../app/inc/bpga_modbus_slave.h), [cliente Raspberry Pi](../../raspberry-pi/app/bpga_server.py), [prueba de snapshot](../../raspberry-pi/tests/test_snapshot_60s.py).
