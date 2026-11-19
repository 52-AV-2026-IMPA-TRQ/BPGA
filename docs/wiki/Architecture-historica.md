# Arquitectura anterior (histórica/reemplazada)

> **Antecedente, no arquitectura vigente.** Esta página preserva el contenido de la antigua propuesta de arquitectura. Sus afirmaciones sobre adquisición real, autoridad de seguridad y protección no deben tomarse como implementadas ni validadas. La arquitectura vigente está en [Architecture.md](Architecture.md).

## Capas propuestas en el documento anterior

1. **EDU-CIAA-NXP**: adquisición real o simulada, validación, alarmas, secuencia de ensayo y esclavo Modbus RTU.
2. **ESP32**: maestro Modbus RTU, caché de datos, API HTTP REST y telemetría SSE.
3. **Raspberry Pi 3B**: navegador Linux en modo kiosco; el texto anterior proponía que podía sustituirse por otro ordenador o tableta.

## Flujo propuesto anteriormente

```text
Sensores o simulador
        |
EDU-CIAA / FreeRTOS
        |
Modbus RTU sobre RS-485
        |
ESP32 gateway HTTP
        |
Wi-Fi
        |
Raspberry Pi 3B / navegador kiosco
```

La propuesta anterior afirmaba que la protección y el control temporal permanecerían en la EDU-CIAA. Esa afirmación no prueba que tales protecciones estén implementadas: el firmware actual documentado opera en Modo Simulador y no valida la etapa electromecánica o de potencia.

Consulta [diseños históricos](Historical-designs.md) para el estado de las alternativas y [estado actual](Current-status.md) para la implementación confirmada.
