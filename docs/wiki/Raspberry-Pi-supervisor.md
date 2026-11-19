# Supervisor Raspberry Pi

## Adquisición

El proceso `bpga_server.py` abre el puerto serie FTDI configurado en la ruta estable `/dev/serial/by-id/...`, con 19200 bit/s, 8E1, timeout de 150 ms y sin control de flujo. Envía FC04 al esclavo 1 para leer 32 registros desde `0x0010` cada 200 ms (5 Hz).

Comprueba tamaño y cabecera de respuesta, CRC, coherencia y paridad de la secuencia. Si ocurre un error, marca `online` y `dataValid` como falsos, registra el último error e incrementa el contador; espera 1 s antes de reintentar. Al restablecerse el puerto y las lecturas, vuelve a marcar los datos válidos. La última muestra permanece en caché mientras no haya nueva respuesta.

## API HTTP local

El servidor escucha en `0.0.0.0:8080`.

| Ruta | Resultado |
|---|---|
| `GET /api/status` | JSON de conectividad, validez, último error, instante de actualización, errores consecutivos y mediciones. |
| `GET /` o `/index...` | Página web del supervisor. Si no se encuentra el archivo, responde 503. |
| Otras rutas | 404. |

La implementación es una API HTTP local; no es una API REST de control ni telemetría SSE. Los endpoints son de lectura.

## Servicio y quiosco

- [Unidad `bpga.service`](../../raspberry-pi/systemd/bpga.service): servicio simple bajo systemd, con `Restart=always` y espera de 2 s. El archivo de referencia usa rutas instaladas bajo `/home/admin/bpga`; deben corresponder al despliegue de la Raspberry Pi.
- [Lanzador de Chromium](../../raspberry-pi/kiosk/kiosk.sh): espera a que `/api/status` responda y ejecuta Chromium con `--kiosk`; si el navegador termina, lo relanza tras 2 s.
- La página actual se sirve desde [web/index.html](../../raspberry-pi/web/index.html).

## Recuperación y límites

El sondeo reabre el puerto serie después de un error y continúa, y el lanzador vuelve a iniciar Chromium si el proceso termina. La prueba integral registrada comprueba continuidad de servicio, API, página y recepción de datos simulados.

La API no activa ni controla equipos. La detección de pérdida/congelamiento de la página es supervisión visual; no equivale a protección de seguridad. La API entrega HTTP sin autenticación observada en este código y debe entenderse como servicio local del demostrador.
