# Desarrollo futuro

Los siguientes puntos son trabajo pendiente y no deben describirse como funciones implementadas. Su orden, diseño final, requisitos y validación quedan por confirmar.

| Área | Trabajo futuro |
|---|---|
| Sensores reales | Integrar y validar adquisición física de RPM, tensión y corriente; separar y documentar corrientes de salida, motor y excitación. |
| RPM | Definir el sensor, acondicionamiento, método de medición y validación de velocidad. |
| Regulador de excitación | Diseñar e integrar una regulación apropiada; hoy no hay regulador operativo. |
| Banco de cargas | Diseñar e integrar cargas y su medición/control; no hay banco implementado. |
| PLC | Definir si se requiere, su interfaz y responsabilidades; no hay PLC operativo. |
| Protecciones | Diseñar y verificar las protecciones de seguridad e interbloqueos antes de cualquier operación electromecánica. No están validadas por el firmware descrito. |
| Etapa electromecánica y de potencia | Completar diseño, integración y validación competente antes de afirmar operación. El motor alimentado a 220 V, el generador energizado y la etapa de potencia no están validados. |
| GPS/RTC | Confirmar una RMC válida, sincronización y lectura de comprobación del RTC; definir cómo se integrará la hora. |
| Protocolo | Corregir/verificar el parser de edad de muestra y acordar qué campos se publicarán realmente antes de ampliar el mapa. |

Este documento no especifica procedimientos para energizar el motor o la etapa de potencia. Consulta [estado actual](Current-status.md) y [seguridad/alcance](Home.md).
