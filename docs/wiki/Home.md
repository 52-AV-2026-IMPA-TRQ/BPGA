# BPGA: documentación técnica

## Propósito y alcance

Esta documentación describe el estado verificable del Banco de Prueba de Generadores Aeronáuticos (BPGA): firmware EDU-CIAA con FreeRTOS, simulación de mediciones, comunicación Modbus RTU sobre RS-485, supervisión Raspberry Pi, API HTTP local e interfaz en Chromium kiosco.

Describe el demostrador de adquisición y supervisión disponible. No certifica un banco, no valida una etapa electromecánica o de potencia y no implica que haya mediciones físicas completas.

## Estado actual

El recorrido implementado es **EDU-CIAA → Modbus RTU/RS-485 → Raspberry Pi → API HTTP local → interfaz web**. El firmware opera en Modo Simulador; la lectura supervisora se realiza a 5 Hz. La prueba integral registrada durante 30 minutos aprobó los criterios de continuidad del servicio, API, página web y datos válidos.

La integración GPS L86 está en curso: hay recepción UART y de mensajes RMC validada; la sincronización RTC requiere una RMC válida y sigue pendiente de confirmación.

## Navegación

- [Arquitectura vigente](Architecture.md)
- [Estado actual y matriz](Current-status.md)
- [Modo Simulador](Simulation-mode.md)
- [Firmware EDU-CIAA](EDU-CIAA-firmware.md)
- [Protocolo Modbus RTU](Protocolo-Modbus-RTU.md)
- [Supervisor Raspberry Pi](Raspberry-Pi-supervisor.md)
- [Interfaz de usuario](User-interface.md)
- [GPS L86 y RTC](GPS-L86-and-RTC.md)
- [Pruebas y validación](Testing-and-validation.md)
- [Desarrollo futuro](Future-development.md)
- [Diseños históricos](Historical-designs.md)
- [Índice lateral](_Sidebar.md)

## Seguridad y lectura de estados

El BPGA documentado aquí es un demostrador educativo, no un banco certificado ni una autorización para operar equipos eléctricos o mecánicos. El motor a 220 V, el generador energizado, el banco de cargas, el PLC, el regulador de excitación y la etapa de potencia no están operativos ni validados. Esta Wiki no contiene procedimientos para energizar dichos elementos.

Las páginas distinguen entre **arquitectura vigente**, **desarrollos futuros** y **antecedentes históricos**. Las propuestas antiguas se conservan clasificadas, pero no describen funciones actuales.
