# Desarrollo (documento histórico/reemplazado)

> **Clasificación: histórico o reemplazado.** Estas instrucciones pertenecen a una etapa anterior y no constituyen el procedimiento vigente de desarrollo o despliegue. Conservadas como antecedente; consulta [estado actual](Current-status.md) y [firmware EDU-CIAA](EDU-CIAA-firmware.md).

## Compilar

```bash
make clean
make -j$(nproc)
```

## Programar

Con el FT2232H adjunto a WSL y la EDU-CIAA conectada al puerto USB DEBUG:

```bash
sudo make download
```

## Indicadores

- LED azul: latido del scheduler, cambia cada 500 ms.
- LED verde: simulador activo.
- LED rojo intermitente: fallo fatal de inicialización.

## Artefactos

Los binarios se generan en `app/out/` y no se versionan.
