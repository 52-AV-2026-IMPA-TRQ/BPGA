# Interfaz de usuario

La página se entrega desde la API local y está diseñada para Chromium en modo kiosco. La actualización consulta `/api/status` cada 200 ms.

## Pantallas

| Pantalla | Contenido actual |
|---|---|
| **Monitor** | RPM, tensión de salida, corriente de salida y potencia de la muestra recibida. |
| **Diagnóstico** | Estado del enlace, secuencia, edad informada, errores consecutivos, último error, estado/fase y flags. |
| **Arquitectura** | EDU-CIAA, enlace Modbus RTU/RS-485, adaptador serial FTDI, Raspberry Pi, 5 Hz y distribución de responsabilidades. |
| **Resultados** | Valores actuales, máximos, muestras vistas y tiempo de sesión calculados en el navegador. No son un informe certificado ni almacenamiento persistente. |

## Navegación y estado de comunicación

La barra inferior permite seleccionar las cuatro vistas. Las flechas izquierda/derecha cambian de pantalla, `Home` vuelve al Monitor y una vista distinta de Monitor vuelve automáticamente a Monitor después de 60 s.

Al recibir datos válidos, el indicador muestra **BANCO CONECTADO**. Tras 800 ms sin recepción válida, la página señala **DATOS CONGELADOS**. A los 2 s desde la última recepción válida, muestra **COMUNICACIÓN PERDIDA**. Si aún no se ha recibido ningún dato válido, muestra inmediatamente **SIN COMUNICACIÓN / SIN DATOS VÁLIDOS**. Los valores visuales se conservan; no se sustituyen por mediciones nuevas ni se deben considerar vigentes durante una pérdida.

El estado visible refleja conectividad y validez de datos del software, no confirma que exista un motor/generador operativo. Las mediciones de la configuración documentada proceden del simulador. La edad de muestra que muestra Diagnóstico depende del campo recibido por la API; véase la discrepancia de parser en [Protocolo Modbus RTU](Protocolo-Modbus-RTU.md).

Implementación: [interfaz y estados](../../raspberry-pi/web/index.html), [API](../../raspberry-pi/app/bpga_server.py), [lanzador de quiosco](../../raspberry-pi/kiosk/kiosk.sh).
