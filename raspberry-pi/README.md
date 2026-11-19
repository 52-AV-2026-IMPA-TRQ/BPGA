# BPGA Raspberry Pi

Supervisor del Banco de Pruebas de Generadores Aeronáuticos.

## Arquitectura

EDU-CIAA -> Modbus RTU/RS-485 -> GmM FT232R -> USB -> Raspberry Pi 3

## Comunicación validada

- Modbus RTU
- 19200 bit/s
- 8E1
- Esclavo 1
- Función FC04
- Snapshot desde 0x0010, 32 registros
- Sondeo a 5 Hz

## Validación

- 301 lecturas en 60,06 segundos
- Frecuencia real: 5,01 Hz
- 0 timeouts
- 0 errores CRC
- 0 snapshots inconsistentes
- 0 secuencias repetidas
- Latencia media: 56,19 ms

## Componentes

- `app/bpga_server.py`: Modbus, caché, API y monitor web.
- `kiosk/kiosk.sh`: Chromium en modo quiosco.
- `kiosk/bash_profile`: inicio de Xorg en tty1.
- `kiosk/xinitrc`: sesión Openbox.
- `kiosk/openbox-autostart`: inicio del quiosco.
- `systemd/bpga.service`: servicio automático.
- `tests/test_snapshot_60s.py`: prueba continua del snapshot.

## Despliegue

Aplicación:

`/home/admin/bpga/`

Servicio:

`/etc/systemd/system/bpga.service`

El adaptador se identifica mediante `/dev/serial/by-id/`.
