# Protocolo Modbus RTU BPGA

## 1. Objetivo

Este documento define el intercambio de datos entre:

- **ESP32**: cliente Modbus RTU y gateway HTTP REST.
- **EDU-CIAA**: servidor Modbus RTU y autoridad de control del banco.
- **Medio físico**: RS-485 semidúplex de dos hilos.

La EDU-CIAA:

- ejecuta la máquina de estados;
- adquiere las mediciones;
- aplica las protecciones;
- controla las salidas y actuadores;
- conserva la autoridad sobre la seguridad del banco.

El ESP32:

- consulta estados, mediciones y alarmas;
- transmite consignas y comandos;
- expone los datos mediante HTTP REST;
- sirve el frontend web;
- genera telemetría de diagnóstico por UART/USB.

La parada de emergencia y las funciones de seguridad críticas no dependerán de Modbus ni del ESP32.

## 2. Configuración de comunicación

Configuración inicial:

| Parámetro | Valor |
|---|---:|
| Modo | Modbus RTU |
| Medio físico | RS-485, dos hilos |
| ESP32 | Cliente o maestro |
| EDU-CIAA | Servidor o esclavo |
| Dirección inicial del servidor | 1 |
| Velocidad | 115200 bit/s |
| Bits de datos | 8 |
| Paridad | Par |
| Bits de parada | 1 |
| Formato | 8E1 |
| Orden de bytes por registro | Big-endian Modbus |
| Orden de palabras de 32 bits | Palabra alta primero |
| Intervalo inicial de consulta | 100 ms |
| Timeout inicial | 100 ms |
| Reintentos | 2 |

**Nota**: Los tiempos son parámetros iniciales y deberán validarse con el hardware real.

## 3. Convención de direcciones

En este protocolo se utiliza la siguiente notación documental:

- **Referencia convencional**: 3xxxx (Input Registers) o 4xxxx (Holding Registers).
- **Dirección PDU hexadecimal**: basada en cero, utilizada en la trama de comunicación.

### Ejemplos:

- **Input Register 30001** = dirección PDU 0x0000 con FC04 (Read Input Registers)
- **Holding Register 40001** = dirección PDU 0x0000 con FC03 (Read Holding Registers)

### Reglas críticas:

1. La referencia 3xxxx o 4xxxx es **solo notación documental**.
2. La dirección transmitida en la PDU comienza en **cero**.
3. El firmware debe usar **constantes basadas exclusivamente en direcciones PDU**.
4. **No deben usarse** números 30001 o 40001 directamente dentro de las tramas.

### Reconstrucción de uint32 desde dos registros:

```c
uint32_t value = ((uint32_t)register_hi << 16) | (uint32_t)register_lo;
```

## 4. Códigos de función

Implementación inicial:

| Código | Operación | Uso |
|---:|---|---|
| 0x03 | Read Holding Registers | Leer configuración, consignas y confirmaciones |
| 0x04 | Read Input Registers | Leer identificación, estado, telemetría y alarmas |
| 0x06 | Write Single Register | Escribir un valor individual |
| 0x10 (FC16) | Write Multiple Registers | Escribir consignas, configuración o comandos agrupados |

**Restricciones**:

- No utilizar inicialmente coils ni discrete inputs.
- Los estados booleanos se transportarán mediante mapas de bits en registros de 16 bits.

## 5. Input Registers

Los Input Registers se leen con el código de función FC04 (Read Input Registers).

### 5.1 Identificación y compatibilidad

| Referencia | PDU | Nombre | Tipo | Descripción |
|---:|---:|---|---|---|
| 30001 | 0x0000 | DEVICE_MAGIC | uint16 | Firma fija 0x4250, ASCII "BP" |
| 30002 | 0x0001 | PROTOCOL_VERSION | uint16 | Versión mayor en byte alto y menor en byte bajo |
| 30003 | 0x0002 | REGISTER_MAP_VERSION | uint16 | Versión del mapa |
| 30004 | 0x0003 | FIRMWARE_VERSION_MAJOR | uint16 | Versión mayor del firmware EDU-CIAA |
| 30005 | 0x0004 | FIRMWARE_VERSION_MINOR | uint16 | Versión menor |
| 30006 | 0x0005 | FIRMWARE_VERSION_PATCH | uint16 | Revisión |
| 30007 | 0x0006 | DEVICE_TYPE | uint16 | 1 = BPGA EDU-CIAA |
| 30008 | 0x0007 | DEVICE_CAPABILITIES | uint16 | Mapa de capacidades |
| 30009 | 0x0008 | BOOT_COUNT_HI | uint16 | Parte alta del contador de arranques |
| 30010 | 0x0009 | BOOT_COUNT_LO | uint16 | Parte baja del contador de arranques |
| 30011–30016 | 0x000A–0x000F | RESERVED_ID | uint16 | Reservado, lectura cero |

#### DEVICE_CAPABILITIES (registro 30008)

| Bit | Capacidad |
|---:|---|
| 0 | Medición de RPM |
| 1 | Medición de tensión |
| 2 | Medición de corriente |
| 3 | Control de excitación |
| 4 | Perfiles de generador |
| 5 | Alarmas enclavadas |
| 6 | Simulación en EDU-CIAA |
| 7 | Reinicio remoto admitido |
| 8–15 | Reservado |

### 5.2 Estado operativo

| Referencia | PDU | Nombre | Tipo | Descripción |
|---:|---:|---|---|---|
| 30017 | 0x0010 | SNAPSHOT_SEQUENCE_BEGIN | uint16 | Secuencia al inicio del bloque |
| 30018 | 0x0011 | SYSTEM_STATE | uint16 | Estado principal |
| 30019 | 0x0012 | SYSTEM_SUBSTATE | uint16 | Subestado o etapa |
| 30020 | 0x0013 | STATUS_FLAGS | uint16 | Estados booleanos |
| 30021 | 0x0014 | CONTROL_FLAGS | uint16 | Estado de salidas |
| 30022 | 0x0015 | ACTIVE_PROFILE_SLOT | uint16 | Posición activa 0–9; 0xFFFF = ninguna |
| 30023 | 0x0016 | ACTIVE_PROFILE_REVISION | uint16 | Revisión del perfil cargado |
| 30024 | 0x0017 | LAST_TRANSITION_REASON | uint16 | Motivo de la última transición |
| 30025 | 0x0018 | UPTIME_HI | uint16 | Tiempo encendido, parte alta |
| 30026 | 0x0019 | UPTIME_LO | uint16 | Tiempo encendido, parte baja |
| 30027 | 0x001A | LOOP_TIME_US | uint16 | Duración de la última iteración |
| 30028 | 0x001B | LOOP_TIME_MAX_US | uint16 | Máximo desde el arranque |
| 30029–30032 | 0x001C–0x001F | RESERVED_STATE | uint16 | Reservado |

#### SYSTEM_STATE (registro 30018) - Propuesto

| Valor | Estado |
|---:|---|
| 0 | Inicializando |
| 1 | Seguro o detenido |
| 2 | Preparado |
| 3 | Arranque |
| 4 | Ensayo en curso |
| 5 | Detención controlada |
| 6 | Ensayo completado |
| 7 | Alarma |
| 8 | Emergencia |
| 9 | Mantenimiento |
| 10 | Simulación |
| 11–65534 | Reservado |
| 65535 | Estado no válido |

**Nota**: La enumeración definitiva deberá coincidir con la máquina de estados canónica de la EDU-CIAA.

#### STATUS_FLAGS (registro 30020)

| Bit | Estado |
|---:|---|
| 0 | Sistema inicializado |
| 1 | Listo para recibir orden |
| 2 | Ensayo activo |
| 3 | Generador en movimiento |
| 4 | Perfil válido cargado |
| 5 | Datos de medición válidos |
| 6 | Comunicación con sensores válida |
| 7 | Simulación activa |
| 8 | Advertencia activa |
| 9 | Alarma activa |
| 10 | Alarma enclavada |
| 11 | Emergencia activa |
| 12 | Parada solicitada |
| 13 | Comando pendiente |
| 14 | Mantenimiento activo |
| 15 | Reservado |

#### CONTROL_FLAGS (registro 30021)

| Bit | Estado |
|---:|---|
| 0 | Habilitación general activa |
| 1 | Motor habilitado |
| 2 | Generador habilitado |
| 3 | Excitación habilitada |
| 4 | Carga habilitada |
| 5 | Salida de seguridad activa |
| 6–15 | Reservado |

### 5.3 Telemetría instantánea

Las magnitudes se transmiten como enteros escalados. **No se utiliza float en el protocolo.**

| Referencia | PDU | Nombre | Tipo | Unidad o escala |
|---:|---:|---|---|---|
| 30033 | 0x0020 | RPM_HI | uint16 | Palabra alta |
| 30034 | 0x0021 | RPM_LO | uint16 | Palabra baja, 1 rpm/LSB |
| 30035 | 0x0022 | VOLTAGE_MV_HI | uint16 | Palabra alta |
| 30036 | 0x0023 | VOLTAGE_MV_LO | uint16 | Palabra baja, milivoltios |
| 30037 | 0x0024 | CURRENT_MA_HI | uint16 | Palabra alta |
| 30038 | 0x0025 | CURRENT_MA_LO | uint16 | Palabra baja, miliamperios |
| 30039 | 0x0026 | POWER_W_HI | uint16 | Palabra alta |
| 30040 | 0x0027 | POWER_W_LO | uint16 | Palabra baja, vatios |
| 30041 | 0x0028 | EXCITATION_PERMILLE | uint16 | 0–1000 equivale a 0–100,0 % |
| 30042 | 0x0029 | LOAD_PERMILLE | uint16 | 0–1000 equivale a 0–100,0 % |
| 30043 | 0x002A | RPM_QUALITY | uint16 | Calidad 0–1000 |
| 30044 | 0x002B | VOLTAGE_QUALITY | uint16 | Calidad 0–1000 |
| 30045 | 0x002C | CURRENT_QUALITY | uint16 | Calidad 0–1000 |
| 30046 | 0x002D | SAMPLE_AGE_MS | uint16 | Antigüedad de la muestra |
| 30047 | 0x002E | MEASUREMENT_FLAGS | uint16 | Validez y saturación |
| 30048 | 0x002F | SNAPSHOT_SEQUENCE_END | uint16 | Secuencia al final del bloque |

#### MEASUREMENT_FLAGS (registro 30047)

| Bit | Estado |
|---:|---|
| 0 | RPM válida |
| 1 | Tensión válida |
| 2 | Corriente válida |
| 3 | Potencia válida |
| 4 | RPM fuera de rango |
| 5 | Tensión fuera de rango |
| 6 | Corriente fuera de rango |
| 7 | Entrada RPM saturada |
| 8 | Entrada de tensión saturada |
| 9 | Entrada de corriente saturada |
| 10 | Datos simulados |
| 11 | Filtro estabilizado |
| 12–15 | Reservado |

### 5.4 Valores acumulados y extremos

| Referencia | PDU | Nombre | Tipo | Unidad |
|---:|---:|---|---|---|
| 30049–30050 | 0x0030–0x0031 | TEST_ELAPSED_MS | uint32 | ms |
| 30051–30052 | 0x0032–0x0033 | RPM_MAX | uint32 | rpm |
| 30053–30054 | 0x0034–0x0035 | VOLTAGE_MAX_MV | uint32 | mV |
| 30055–30056 | 0x0036–0x0037 | CURRENT_MAX_MA | uint32 | mA |
| 30057–30058 | 0x0038–0x0039 | ENERGY_WH | uint32 | Wh |
| 30059–30060 | 0x003A–0x003B | SAMPLE_COUNTER | uint32 | muestras |
| 30061–30064 | 0x003C–0x003F | RESERVED_TOTALS | uint16 | Reservado |

### 5.5 Alarmas y diagnóstico

| Referencia | PDU | Nombre | Tipo | Descripción |
|---:|---:|---|---|---|
| 30065 | 0x0040 | ACTIVE_WARNINGS | uint16 | Advertencias activas |
| 30066 | 0x0041 | ACTIVE_ALARMS | uint16 | Alarmas activas |
| 30067 | 0x0042 | LATCHED_ALARMS | uint16 | Alarmas enclavadas |
| 30068 | 0x0043 | PRIMARY_FAULT_CODE | uint16 | Código principal |
| 30069 | 0x0044 | LAST_FAULT_CODE | uint16 | Último fallo |
| 30070–30071 | 0x0045–0x0046 | LAST_FAULT_UPTIME_MS | uint32 | Instante desde arranque |
| 30072 | 0x0047 | MODBUS_VALID_REQUESTS | uint16 | Solicitudes válidas |
| 30073 | 0x0048 | MODBUS_CRC_ERRORS | uint16 | Errores CRC |
| 30074 | 0x0049 | MODBUS_EXCEPTIONS | uint16 | Excepciones enviadas |
| 30075 | 0x004A | WATCHDOG_RESET_COUNT | uint16 | Reinicios por watchdog |
| 30076–30080 | 0x004B–0x004F | RESERVED_DIAGNOSTICS | uint16 | Reservado |

#### Mapa inicial de alarmas

| Bit | Alarma |
|---:|---|
| 0 | Parada de emergencia |
| 1 | Sobrevelocidad |
| 2 | Sobretensión |
| 3 | Sobrecorriente |
| 4 | Pérdida de señal RPM |
| 5 | Fallo de medición de tensión |
| 6 | Fallo de medición de corriente |
| 7 | Perfil no válido |
| 8 | Timeout de etapa |
| 9 | Fallo de excitación |
| 10 | Fallo de actuador |
| 11 | Error interno |
| 12 | Watchdog |
| 13 | Configuración no válida |
| 14 | Fallo de seguridad |
| 15 | Reservado |

## 6. Holding Registers

Los Holding Registers se leen con el código de función FC03 (Read Holding Registers) y se escriben con FC06 o FC16 (0x10) (Write Single Register o Write Multiple Registers).

### 6.1 Configuración

| Referencia | PDU | Nombre | Tipo | Rango |
|---:|---:|---|---|---|
| 40001 | 0x0000 | MODBUS_SLAVE_ADDRESS | uint16 | 1–247 |
| 40002 | 0x0001 | MODBUS_BAUD_CODE | uint16 | Código de velocidad |
| 40003 | 0x0002 | MODBUS_FORMAT_CODE | uint16 | Formato serie |
| 40004 | 0x0003 | TELEMETRY_PERIOD_MS | uint16 | 20–5000 ms |
| 40005 | 0x0004 | COMM_WATCHDOG_MS | uint16 | 0 = deshabilitado |
| 40006 | 0x0005 | OPERATING_MODE | uint16 | Real, simulación o mantenimiento |
| 40007–40016 | 0x0006–0x000F | RESERVED_CONFIG | uint16 | Reservado |

**Nota**: Los cambios de dirección y formato se aplicarán únicamente después de guardar configuración y reiniciar.

### 6.2 Selección de perfil

| Referencia | PDU | Nombre | Tipo | Rango |
|---:|---:|---|---|---|
| 40017 | 0x0010 | REQUESTED_PROFILE_SLOT | uint16 | 0–9 |
| 40018 | 0x0011 | PROFILE_EXPECTED_REVISION | uint16 | Revisión esperada |
| 40019 | 0x0012 | PROFILE_SELECTION_FLAGS | uint16 | Opciones |
| 40020 | 0x0013 | PROFILE_SELECTION_RESULT | uint16 | Resultado |
| 40021–40024 | 0x0014–0x0017 | RESERVED_PROFILE | uint16 | Reservado |
| 40025–40032 | 0x0018–0x001F | RESERVED_PROFILE_EXPANSION | uint16 | Reservado |

**Restricción**: La EDU-CIAA rechazará cambios de perfil durante un ensayo activo.

### 6.3 Consignas operativas

| Referencia | PDU | Nombre | Tipo | Unidad |
|---:|---:|---|---|---|
| 40033–40034 | 0x0020–0x0021 | RPM_SETPOINT | uint32 | rpm |
| 40035–40036 | 0x0022–0x0023 | VOLTAGE_SETPOINT_MV | uint32 | mV |
| 40037–40038 | 0x0024–0x0025 | CURRENT_LIMIT_MA | uint32 | mA |
| 40039 | 0x0026 | EXCITATION_SETPOINT | uint16 | 0–1000 |
| 40040 | 0x0027 | LOAD_SETPOINT | uint16 | 0–1000 |
| 40041–40042 | 0x0028–0x0029 | TEST_DURATION_MS | uint32 | ms |
| 40043 | 0x002A | RAMP_TIME_MS | uint16 | ms |
| 40044 | 0x002B | SETPOINT_FLAGS | uint16 | Validez |
| 40045–40048 | 0x002C–0x002F | RESERVED_SETPOINTS | uint16 | Reservado |
| 40049–40064 | 0x0030–0x003F | RESERVED_CONTROL_EXPANSION | uint16 | Reservado |

**Importante**: Las consignas escritas no se aplicarán directamente. Se validarán y se aplicarán mediante el comando APPLY_SETPOINTS.

## 7. Canal de comandos

### 7.1 Solicitud

| Referencia | PDU | Nombre | Tipo | Descripción |
|---:|---:|---|---|---|
| 40065 | 0x0040 | COMMAND_SEQUENCE | uint16 | Identificador |
| 40066 | 0x0041 | COMMAND_CODE | uint16 | Orden |
| 40067 | 0x0042 | COMMAND_ARGUMENT_0 | uint16 | Argumento |
| 40068 | 0x0043 | COMMAND_ARGUMENT_1 | uint16 | Argumento |
| 40069 | 0x0044 | COMMAND_ARGUMENT_2 | uint16 | Argumento |
| 40070 | 0x0045 | COMMAND_CONTROL | uint16 | Confirmación 0xA55A |
| 40071–40072 | 0x0046–0x0047 | RESERVED_COMMAND | uint16 | Reservado |

#### Reglas de procesamiento:

- El ESP32 escribirá 40065–40070 mediante una única operación **FC16 (0x10)** (Write Multiple Registers).
- La EDU-CIAA procesará el comando solamente si:
  - `COMMAND_CONTROL == 0xA55A`
  - `COMMAND_SEQUENCE != última secuencia procesada`
- Los reintentos con la misma secuencia **no volverán a ejecutar** la orden.

### 7.2 Resultado

| Referencia | PDU | Nombre | Tipo | Descripción |
|---:|---:|---|---|---|
| 40073 | 0x0048 | COMMAND_ACK_SEQUENCE | uint16 | Última secuencia procesada |
| 40074 | 0x0049 | COMMAND_STATUS | uint16 | Estado |
| 40075 | 0x004A | COMMAND_RESULT | uint16 | Resultado |
| 40076 | 0x004B | COMMAND_DETAIL | uint16 | Detalle |
| 40077–40080 | 0x004C–0x004F | RESERVED_COMMAND_RESULT | uint16 | Reservado |

#### COMMAND_STATUS (registro 40074)

| Valor | Estado |
|---:|---|
| 0 | Sin comando |
| 1 | Recibido |
| 2 | En ejecución |
| 3 | Completado |
| 4 | Rechazado |
| 5 | Fallido |

#### COMMAND_RESULT (registro 40075)

| Valor | Resultado |
|---:|---|
| 0 | Correcto |
| 1 | Comando desconocido |
| 2 | Estado incompatible |
| 3 | Argumento fuera de rango |
| 4 | Perfil no válido |
| 5 | Alarma activa |
| 6 | Emergencia activa |
| 7 | Sistema no preparado |
| 8 | Comando duplicado |
| 9 | Timeout |
| 10 | Error interno |

#### COMMAND_CODE (registro 40066)

| Valor | Comando |
|---:|---|
| 0x0000 | Ninguno |
| 0x0001 | Preparar banco |
| 0x0002 | Iniciar ensayo |
| 0x0003 | Solicitar detención controlada |
| 0x0004 | Cancelar preparación |
| 0x0005 | Aplicar consignas |
| 0x0006 | Cargar perfil seleccionado |
| 0x0007 | Reconocer advertencias |
| 0x0008 | Resetear alarmas enclavadas |
| 0x0009 | Entrar en mantenimiento |
| 0x000A | Salir de mantenimiento |
| 0x000B | Activar simulación |
| 0x000C | Desactivar simulación |
| 0x000D | Guardar configuración |
| 0x000E | Reiniciar controlador |
| 0x000F–0x7FFF | Reservado |
| 0x8000–0xFFFF | Diagnóstico y desarrollo |

**Aclaración**: La parada de emergencia no se implementa mediante comandos Modbus y permanece bajo control local cableado en la EDU-CIAA.

## 8. Coherencia de telemetría

El **ESP32 leerá los registros 30017–30048** (bloque de estado y telemetría) mediante una sola operación **FC04**.

#### Protocolo de actualización en EDU-CIAA:

1. Incrementar internamente la secuencia.
2. Escribir un valor **impar** en SNAPSHOT_SEQUENCE_BEGIN.
3. Actualizar estado, flags y mediciones.
4. Escribir el nuevo valor **par** en SNAPSHOT_SEQUENCE_BEGIN.
5. Copiar el mismo valor en SNAPSHOT_SEQUENCE_END.

#### Validación en ESP32:

El ESP32 aceptará la lectura solamente si:

- `SNAPSHOT_SEQUENCE_BEGIN == SNAPSHOT_SEQUENCE_END`
- `(SNAPSHOT_SEQUENCE_BEGIN & 1) == 0` (valor par)

Si no se cumple, el ESP32 repetirá la lectura completa.

## 9. Ciclo de consulta del ESP32

| Periodo | Bloque | Rango de registros |
|---:|---|---|
| 100 ms | Estado y telemetría | 30017–30048 |
| 500 ms | Alarmas y diagnóstico | 30065–30080 |
| 5 s | Identificación | 30001–30016 |
| Bajo demanda | Holding registers y resultado de comandos | 40001+ |

#### Reglas de agrupación:

- Las lecturas normales deben **agrupar registros contiguos**.
- **No realizar polling** registro por registro.
- Utilizar las operaciones FC04 o FC03 para leer bloques completos.

## 10. Pérdida de comunicación

El ESP32 considerará **perdida la comunicación** si:

- fallan todos los reintentos;
- la secuencia no cambia durante el tiempo configurado;
- `SAMPLE_AGE_MS` supera el límite;
- `DEVICE_MAGIC` no es válido;
- la versión mayor del mapa no es compatible.

#### Comportamiento del ESP32 ante pérdida de comunicación:

- Conserva la última muestra marcada como **obsoleta**.
- Publica `connected = false`.
- **No sustituye** datos reales por simulados.
- **Bloquea comandos operativos** desde la interfaz web.
- Mantiene disponibles los **diagnósticos locales**.

#### Responsabilidad de la EDU-CIAA:

- Podrá usar un **watchdog de comunicación** solo como protección operativa complementaria.
- **La seguridad crítica debe permanecer local y cableada**, independiente de Modbus.

## 11. Versionado

La compatibilidad se determinará mediante:

- **PROTOCOL_VERSION** (registro 30002)
- **REGISTER_MAP_VERSION** (registro 30003)

#### Reglas:

1. Un cambio **menor** puede añadir registros en zonas reservadas.
2. Un cambio **mayor** cambia dirección, tipo, escala, orden o semántica.
3. El ESP32 rechazará la operación normal ante una versión mayor incompatible.
4. Un registro publicado no podrá reutilizarse con otro significado.

## 12. Reglas generales

1. Los valores multirregistro serán **uint32** salvo indicación distinta.
2. Los valores negativos futuros usarán **complemento a dos**.
3. **No utilizar float32** en el enlace.
4. Utilizar **unidades enteras**: rpm, mV, mA, ms, W y Wh.
5. Los registros reservados devolverán **cero**.
6. Las **escrituras en registros reservados serán rechazadas**.
7. Una escritura **fuera de rango no cambiará** el valor anterior.
8. Las consignas se validarán **por completo** antes de aplicarlas.
9. Los comandos usarán **secuencia** para tolerar reintentos.
10. La **parada de emergencia queda fuera de Modbus**.
11. El **ESP32 no será la autoridad** de control del banco.
12. La **EDU-CIAA conservará la última decisión** sobre actuadores y seguridad.

## Mapeo de direcciones - Resumen visual

### Input Registers (FC04)

| Bloque | Inicio | Fin | Descripción |
|---|---:|---:|---|
| Identificación | 30001 | 30016 | DEVICE_MAGIC, versiones, capacidades, contador arranques |
| Estado operativo | 30017 | 30032 | Secuencia, máquina de estados, flags, perfiles |
| Telemetría instantánea | 30033 | 30048 | RPM, tensión, corriente, potencia, calidad, flags |
| Acumulados y extremos | 30049 | 30064 | Tiempos, máximos, energía, contador muestras |
| Alarmas y diagnóstico | 30065 | 30080 | Alarmas, fallos, estadísticas Modbus, watchdog |

### Holding Registers (FC03/FC06/FC16)

| Bloque | Inicio | Fin | Descripción |
|---|---:|---:|---|
| Configuración | 40001 | 40016 | Modbus, telemetría y modo operativo |
| Selección de perfil | 40017 | 40024 | Perfil solicitado, revisión y resultado |
| Reserva para perfiles | 40025 | 40032 | Expansión futura |
| Consignas operativas | 40033 | 40048 | RPM, tensión, corriente, excitación, carga, duración y rampa |
| Reserva para control | 40049 | 40064 | Expansión futura |
| Canal de comandos | 40065 | 40080 | Solicitud y resultado de comandos |

---

**Versión del documento**: 1.0
**Última actualización**: 2026-11-05
**Estado**: Propuesta inicial para revisión
