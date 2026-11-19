# GPS L86 y RTC

## Implementación en firmware

El módulo GPS está conectado al puerto `UART_232`, que la capa de placa asigna al UART3 (USART3) y a los pines 232 del conector P1. La tarea configura 9600 bit/s, 8 bits, sin paridad y 1 bit de parada (8N1).

El firmware recibe bytes por sondeo, arma líneas NMEA, comprueba el checksum y procesa sentencias `$GPRMC` o `$GNRMC`. Solo acepta una RMC con estado de navegación válido (`A`), campos de fecha/hora utilizables y calendario válido. Si la escritura del RTC y la lectura de comprobación coinciden, informa sincronización UTC y termina la tarea.

La primera ventana de recepción dura hasta 120 s. Si no sincroniza, espera 300 s y hace ventanas de reintento de 30 s. La recepción de bytes UART y de una sentencia RMC con checksum se informa por UART USB.

## Estado de validación

- Recepción UART validada.
- Recepción de mensajes RMC validada.
- La obtención de una RMC válida para sincronizar y la sincronización efectiva del RTC están pendientes de confirmación.
- La implementación no usa 1PPS.
- El GPS no forma parte del mapa Modbus ni del JSON de estado vigente.

Por tanto, el sistema no garantiza actualmente una hora RTC sincronizada por GPS. No se debe confundir la recepción de una RMC con una sincronización RTC completada.

Referencias: [tarea GPS](../../app/src/bpga_gps.c), [mapa de periféricos de placa](../../firmware_v3-r1.3.0/libs/sapi/sapi_v0.5.2/board/inc/sapi_peripheral_map.h), [creación de tarea](../../app/src/app.c).
