# BPGA

Banco de Prueba de Generadores Aeronauticos.

## Plataforma inicial

- EDU-CIAA-NXP
- NXP LPC4337 Cortex-M4F
- CIAA Firmware v3 r1.3.0
- FreeRTOS
- sAPI y LPCOpen
- Comunicacion RS485 prevista
- Adquisicion de sensores y GNSS prevista

## Primera validacion

La aplicacion inicial crea una tarea FreeRTOS que conmuta el LED azul cada 500 ms.
Si el scheduler no puede iniciarse, el firmware conmuta el LED rojo en un bucle de error.

## Compilacion

    make clean
    make -j$(nproc)

Los artefactos quedan en `app/out/`.
