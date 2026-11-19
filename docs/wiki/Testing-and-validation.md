# Pruebas y validación

## Alcance de las pruebas disponibles

- [Prueba de snapshot de 60 s](../../raspberry-pi/tests/test_snapshot_60s.py): solicita el bloque de 32 registros a 5 Hz, valida longitud/cabecera/CRC y comprueba la secuencia del snapshot.
- [Prueba integral de 30 min](../../raspberry-pi/tests/test_integral_30m.py): registra una muestra por segundo, consulta API y página, revisa el servicio y cada 10 s comprueba procesos y métricas de Raspberry Pi.
- [CSV de evidencia](../../evidencias/prueba-integral-30m/integral_30m_20261115_164155.csv), [resultado corregido](../../evidencias/prueba-integral-30m/resultado_corregido.txt) y [sumas SHA-256](../../evidencias/prueba-integral-30m/SHA256SUMS).

## Resultado registrado

La evidencia corregida registra 1800,06 s y 1801 muestras. Hubo cero fallos de servicio, API, datos válidos, web, Xorg, Openbox o Chromium; cero secuencias repetidas; tres wraps normales del contador lógico de 15 bits y cero regresiones reales.

En esa ejecución: latencia API 5,14/8,49/31,66 ms (mínima/media/máxima), latencia web 4,62/7,47/20,14 ms y edad de muestra 1/10,83/20 ms. La máscara inicial/final reportada fue `0x50000` y no se registraron eventos eléctricos activos. El resultado corregido es **APROBADA**.

El resumen original contabilizaba los tres wraps como regresiones y decía “REVISAR”; el archivo corregido los interpreta como el retorno normal de un contador lógico de 15 bits. Se conserva el resumen original en la evidencia, junto con la corrección y sus hashes.

## Qué valida y qué no

La prueba integral acredita la continuidad del demostrador software con valores simulados durante esa ejecución concreta. No acredita mediciones físicas, precisión de instrumentos, seguridad funcional, funcionamiento del motor o generador, un banco de cargas, protecciones, PLC, regulador ni etapa de potencia. Tampoco es certificación o garantía de ejecuciones futuras.

Las pruebas requieren EDU-CIAA, enlace serial RS-485 y supervisor Raspberry Pi para ejecutarse en el entorno previsto; esta reorganización documental no vuelve a ejecutar una prueba de hardware.
