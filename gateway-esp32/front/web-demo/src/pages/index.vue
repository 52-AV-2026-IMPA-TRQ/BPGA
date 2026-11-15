<template>
  <div class="bpga-container">
    <div class="bpga-header">
      <div class="bpga-header-left">
        <div class="bpga-aircraft-symbol">✈</div>
        <div class="bpga-header-text">
          <h1 class="bpga-title">Banco de Pruebas de Generadores Aeronáuticos</h1>
          <p class="bpga-subtitle">Monitor didáctico de ensayo y evaluación</p>
          <p class="bpga-institution">E.E.S.T. Nº7 «Taller Regional Quilmes» • IMPA · Aeronáutica y Aviónica</p>
        </div>
      </div>
      <div class="bpga-header-right">
        <div class="bpga-status-chip">
          <div :class="['bpga-chip', isOnline ? 'bpga-chip--online' : 'bpga-chip--offline']">
            {{ isOnline ? '● Banco conectado' : '● Sin comunicación' }}
          </div>
          <div :class="['bpga-chip', isDataValid ? 'bpga-chip--valid' : 'bpga-chip--invalid']">
            {{ isDataValid ? '● Datos válidos' : '● Datos incompletos' }}
          </div>
          <div :class="['bpga-chip', state.simulationActive ? 'bpga-chip--sim' : 'bpga-chip--hw']">
            {{ state.simulationActive ? '● Simulación' : '● Hardware' }}
          </div>
          <div class="bpga-chip bpga-chip--time">
            ⏱ {{ formattedLastUpdate }}
          </div>
        </div>
      </div>
    </div>

    <v-container class="pa-4 pa-md-6" fluid>
      <v-alert v-if="error" class="mb-4" type="error" variant="tonal" icon="$alert">
        {{ errorMessage }}
        <span v-if="lastUpdate"> Última lectura válida: {{ timeSinceLastUpdate }}.</span>
      </v-alert>

      <v-alert v-if="data && !isDataValid" class="mb-4" type="warning" variant="tonal" icon="$warning">
        Datos incompletos. Los bloques que se muestran como inválidos requieren atención.
      </v-alert>

      <v-card class="mb-4 bpga-section" elevation="2">
        <v-card-title class="bpga-section-title">Enlace de datos</v-card-title>
        <v-divider />
        <v-card-text>
          <v-row dense>
            <v-col cols="12" sm="6" md="3" v-for="block in validityBlocks" :key="block.name">
              <div class="bpga-validity-item">
                <span class="bpga-label">{{ formatBlockName(block.name) }}</span>
                <span :class="['bpga-validity-badge', block.valid ? 'bpga-valid' : 'bpga-invalid']">
                  {{ block.valid ? '✓ Válido' : '✗ Inválido' }}
                </span>
              </div>
            </v-col>
          </v-row>
          <v-row dense class="mt-2" v-if="lastError !== 0">
            <v-col cols="12">
              <div class="bpga-error-info">
                <strong>Último error:</strong> {{ lastError }}
              </div>
            </v-col>
          </v-row>
        </v-card-text>
      </v-card>

      <v-row>
        <v-col cols="12" lg="4">
          <v-card class="bpga-section h-100" elevation="2">
            <v-card-title class="bpga-section-title">Condición operativa</v-card-title>
            <v-divider />
            <v-card-text class="bpga-state-section">
              <div class="bpga-state-item">
                <span class="bpga-label">Estado del sistema</span>
                <span class="bpga-value">{{ systemState }}</span>
              </div>
              <div class="bpga-state-item">
                <span class="bpga-label">Fase operativa</span>
                <span class="bpga-value">{{ systemSubstate }}</span>
              </div>
              <div class="bpga-state-item">
                <span class="bpga-label">Mediciones válidas</span>
                <span :class="['bpga-value', state.measurementsValid ? 'bpga-text-success' : 'bpga-text-error']">
                  {{ state.measurementsValid ? 'Sí' : 'No' }}
                </span>
              </div>
              <div class="bpga-state-item">
                <span class="bpga-label">Perfil activo</span>
                <span class="bpga-value">{{ String(profile.slot ?? 0) }}</span>
              </div>
              <div class="bpga-state-item">
                <span class="bpga-label">Revisión</span>
                <span class="bpga-value">{{ String(profile.revision ?? 0) }}</span>
              </div>
              <div class="bpga-state-item">
                <span class="bpga-label">Flags de estado</span>
                <span class="bpga-value bpga-small">{{ statusFlags }}</span>
              </div>
              <div class="bpga-state-item">
                <span class="bpga-label">Flags de control</span>
                <span class="bpga-value bpga-small">{{ controlFlags }}</span>
              </div>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" lg="8">
          <v-row>
            <v-col v-for="measurement in measurements" :key="measurement.title" cols="12" sm="6">
              <v-card class="bpga-instrument h-100" elevation="3">
                <v-card-text class="bpga-instrument-content">
                  <div class="bpga-instrument-label">{{ measurement.title }}</div>
                  <div class="bpga-instrument-value">{{ measurement.value }}</div>
                  <div class="bpga-instrument-unit">{{ measurement.unit }}</div>
                  <div class="bpga-instrument-desc">{{ measurement.description }}</div>
                </v-card-text>
              </v-card>
            </v-col>
          </v-row>
        </v-col>
      </v-row>

      <v-card class="mt-4 bpga-section" elevation="2">
        <v-card-title class="bpga-section-title">Valores nominales de referencia</v-card-title>
        <v-divider />
        <v-card-text>
          <p class="bpga-nominal-text">Valores nominales del banco didáctico:</p>
          <div class="bpga-nominal-grid">
            <div class="bpga-nominal-item">
              <span class="bpga-nominal-value">1500</span>
              <span class="bpga-nominal-unit">rpm</span>
            </div>
            <div class="bpga-nominal-item">
              <span class="bpga-nominal-value">220</span>
              <span class="bpga-nominal-unit">V</span>
            </div>
            <div class="bpga-nominal-item">
              <span class="bpga-nominal-value">3,4</span>
              <span class="bpga-nominal-unit">A</span>
            </div>
            <div class="bpga-nominal-item">
              <span class="bpga-nominal-value">745</span>
              <span class="bpga-nominal-unit">W</span>
            </div>
          </div>
        </v-card-text>
      </v-card>

      <v-row class="mt-4">
        <v-col cols="12" lg="6">
          <v-card class="bpga-section h-100" elevation="2">
            <v-card-title class="bpga-section-title">Máximos y acumulados</v-card-title>
            <v-divider />
            <v-card-text>
              <v-row dense>
                <v-col v-for="total in totals" :key="total.title" cols="12" sm="6">
                  <div class="bpga-total-item">
                    <span class="bpga-label">{{ total.title }}</span>
                    <span class="bpga-value">{{ total.value }}</span>
                  </div>
                </v-col>
              </v-row>
            </v-card-text>
          </v-card>
        </v-col>

        <v-col cols="12" lg="6">
          <v-card class="bpga-section h-100" elevation="2">
            <v-card-title class="bpga-section-title">Diagnóstico de comunicaciones</v-card-title>
            <v-divider />
            <v-card-text>
              <v-row dense>
                <v-col v-for="diagnostic in diagnostics" :key="diagnostic.title" cols="12" sm="6">
                  <div class="bpga-diag-item">
                    <span class="bpga-label">{{ diagnostic.title }}</span>
                    <span class="bpga-value">{{ diagnostic.value }}</span>
                  </div>
                </v-col>
              </v-row>
            </v-card-text>
          </v-card>
        </v-col>
      </v-row>

      <v-card class="mt-4 bpga-section" elevation="2" color="orange-lighten-5">
        <v-card-title class="bpga-section-title">Seguridad de operación</v-card-title>
        <v-divider />
        <v-card-text class="bpga-safety-text">
          <p>
            El motor de arrastre de corriente continua serie debe permanecer mecánicamente acoplado
            durante la operación. No debe funcionar sin carga mecánica. La parada de emergencia y los
            interbloqueos físicos son independientes de esta interfaz de monitorización.
          </p>
          <p class="mt-3">
            <strong>Interfaz de solo lectura.</strong> No sustituye los procedimientos de operación,
            inspección ni seguridad del banco.
          </p>
        </v-card-text>
      </v-card>

      <v-card class="mt-4 bpga-section" elevation="2" color="blue-lighten-5">
        <v-card-title class="bpga-section-title">Contexto técnico</v-card-title>
        <v-divider />
        <v-card-text class="bpga-context-text">
          <p>
            Banco didáctico compuesto por un motor de arrastre de corriente continua serie de 1 HP,
            acoplado mecánicamente a un generador de corriente continua de excitación independiente.
            Diseñado para la enseñanza en las especialidades de Aeronáutica y Aviónica.
          </p>
        </v-card-text>
      </v-card>
    </v-container>
  </div>
</template>

<script setup>
  import { computed } from 'vue'
  import { useBpgaStatus } from '@/composables/useApi'

  const { data, error, lastUpdate } = useBpgaStatus(500)

  const validity = computed(() => data.value?.validity ?? {})
  const identification = computed(() => data.value?.identification ?? {})
  const state = computed(() => data.value?.state ?? {})
  const profile = computed(() => data.value?.profile ?? {})
  const measurement = computed(() => data.value?.measurement ?? {})
  const totalsData = computed(() => data.value?.totals ?? {})
  const diagnosticsData = computed(() => data.value?.diagnostics ?? {})

  const isOnline = computed(() => data.value?.online === true)
  const isDataValid = computed(() => data.value?.dataValid === true)
  const lastError = computed(() => data.value?.lastError ?? 0)

  const validityBlocks = computed(() => ['identification', 'snapshot', 'totals', 'diagnostics'].map(name => ({
    name,
    valid: validity.value[name] === true,
  })))

  const systemState = computed(() => stateLabel(state.value.systemState))
  const systemSubstate = computed(() => substateLabel(state.value.systemSubstate))
  const statusFlags = computed(() => formatFlag(state.value.statusFlags))
  const controlFlags = computed(() => formatFlag(state.value.controlFlags))

  const measurements = computed(() => [
    {
      title: 'RPM',
      value: toNumber(measurement.value.rpm).toFixed(0),
      unit: 'rpm',
      description: 'Velocidad instantánea',
    },
    {
      title: 'Tensión de salida',
      value: ((measurement.value.voltageMv ?? 0) / 1000).toFixed(1),
      unit: 'V',
      description: 'Generador bajo ensayo',
    },
    {
      title: 'Corriente de salida del generador',
      value: ((measurement.value.currentMa ?? 0) / 1000).toFixed(2),
      unit: 'A',
      description: 'Medición instantánea',
    },
    {
      title: 'Potencia',
      value: toNumber(measurement.value.powerW).toFixed(0),
      unit: 'W',
      description: 'Potencia eléctrica instantánea',
    },
    {
      title: 'Edad de la muestra',
      value: toNumber(measurement.value.sampleAgeMs).toFixed(0),
      unit: 'ms',
      description: 'Tiempo desde la última captura',
    },
  ])

  const totals = computed(() => [
    { title: 'RPM máxima', value: toNumber(totalsData.value.rpmMax).toFixed(0) + ' rpm' },
    { title: 'Tensión máxima', value: ((totalsData.value.voltageMaxMv ?? 0) / 1000).toFixed(1) + ' V' },
    { title: 'Corriente máxima', value: ((totalsData.value.currentMaxMa ?? 0) / 1000).toFixed(2) + ' A' },
    { title: 'Muestras procesadas', value: toNumber(totalsData.value.sampleCounter).toFixed(0) },
    { title: 'Tiempo de ensayo', value: formatElapsed(totalsData.value.testElapsedMs) },
    {
      title: 'Energía',
      value: toNumber(totalsData.value.energyWh) === 0 ? 'Pendiente' : toNumber(totalsData.value.energyWh).toFixed(1) + ' Wh',
    },
  ])

  const diagnostics = computed(() => [
    { title: 'Advertencias activas', value: toNumber(diagnosticsData.value.activeWarnings) },
    { title: 'Alarmas activas', value: toNumber(diagnosticsData.value.activeAlarms) },
    { title: 'Alarmas retenidas', value: toNumber(diagnosticsData.value.latchedAlarms) },
    { title: 'Fallo primario', value: formatFaultCode(diagnosticsData.value.primaryFaultCode) },
    { title: 'Último fallo', value: formatFaultCode(diagnosticsData.value.lastFaultCode) },
    { title: 'Solicitudes Modbus', value: toNumber(diagnosticsData.value.modbusValidRequests) },
    { title: 'Errores CRC', value: toNumber(diagnosticsData.value.modbusCrcErrors) },
    { title: 'Excepciones Modbus', value: toNumber(diagnosticsData.value.modbusExceptions) },
  ])

  const formattedLastUpdate = computed(() =>
    lastUpdate.value ? lastUpdate.value.toLocaleTimeString('es-AR') : '—'
  )

  const timeSinceLastUpdate = computed(() => {
    if (!lastUpdate.value) return 'sin datos'
    const seconds = Math.max(0, Math.floor((Date.now() - lastUpdate.value.getTime()) / 1000))
    return seconds < 60 ? `hace ${seconds} s` : `hace ${Math.floor(seconds / 60)} min`
  })

  const errorMessage = computed(() => error.value?.message || 'No se pudo actualizar el estado del banco')

  function formatBlockName (name) {
    const names = {
      identification: 'Identificación',
      snapshot: 'Medición instantánea',
      totals: 'Totales',
      diagnostics: 'Diagnósticos',
    }
    return names[name] ?? name
  }

  function stateLabel (value) {
    const labels = { 0: 'Inicializando', 1: 'Parada segura', 2: 'Listo' }
    return value == null ? '—' : (labels[value] ?? `Desconocido (${value})`)
  }

  function substateLabel (value) {
    const labels = {
      0: 'Detenido',
      1: 'Acelerando',
      2: 'Sin carga eléctrica',
      3: 'Aplicando carga',
      4: 'Estable',
      5: 'Retirando carga',
      6: 'Desacelerando',
    }
    return value == null ? '—' : (labels[value] ?? `Desconocido (${value})`)
  }

  function formatFlag (value) {
    return value == null ? '—' : `${value} (0x${toNumber(value).toString(16).toUpperCase().padStart(2, '0')})`
  }

  function formatFaultCode (value) {
    return value == null || value === 0 ? '—' : String(value)
  }

  function toNumber (value) {
    const number = Number(value)
    return Number.isFinite(number) ? number : 0
  }

  function formatElapsed (value) {
    const milliseconds = toNumber(value)
    const totalSeconds = Math.floor(milliseconds / 1000)
    const minutes = Math.floor(totalSeconds / 60)
    const seconds = totalSeconds % 60
    return `${minutes} min ${String(seconds).padStart(2, '0')} s`
  }
</script>

<style scoped>
.bpga-container {
  min-height: 100vh;
  background: linear-gradient(135deg, #eceff1 0%, #cfd8dc 100%);
}

.bpga-header {
  background: linear-gradient(90deg, #01579b 0%, #0277bd 100%);
  color: white;
  padding: 2rem;
  display: flex;
  justify-content: space-between;
  align-items: center;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.15);
  flex-wrap: wrap;
  gap: 2rem;
}

.bpga-header-left {
  display: flex;
  gap: 1.5rem;
  align-items: flex-start;
  flex: 1;
}

.bpga-aircraft-symbol {
  font-size: 3rem;
  opacity: 0.9;
}

.bpga-header-text h1 {
  margin: 0;
  font-size: 1.75rem;
  font-weight: 600;
  letter-spacing: 0.5px;
}

.bpga-header-text p {
  margin: 0.3rem 0 0 0;
  font-size: 0.95rem;
  opacity: 0.95;
}

.bpga-subtitle {
  font-weight: 500;
  font-size: 1rem;
}

.bpga-institution {
  font-size: 0.85rem;
  opacity: 0.85;
  margin-top: 0.5rem;
}

.bpga-header-right {
  display: flex;
  justify-content: flex-end;
  min-width: 300px;
}

.bpga-status-chip {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.bpga-chip {
  padding: 0.5rem 1rem;
  border-radius: 4px;
  font-size: 0.9rem;
  font-weight: 500;
  background: rgba(255, 255, 255, 0.15);
  border: 1px solid rgba(255, 255, 255, 0.3);
}

.bpga-chip--online {
  background: rgba(76, 175, 80, 0.2);
  color: #c8e6c9;
}

.bpga-chip--offline {
  background: rgba(244, 67, 54, 0.2);
  color: #ffcdd2;
}

.bpga-chip--valid {
  background: rgba(76, 175, 80, 0.2);
  color: #c8e6c9;
}

.bpga-chip--invalid {
  background: rgba(255, 152, 0, 0.2);
  color: #ffe0b2;
}

.bpga-chip--sim {
  background: rgba(33, 150, 243, 0.2);
  color: #bbdefb;
}

.bpga-chip--hw {
  background: rgba(76, 175, 80, 0.2);
  color: #c8e6c9;
}

.bpga-chip--time {
  background: rgba(255, 255, 255, 0.1);
  color: rgba(255, 255, 255, 0.9);
}

.bpga-section {
  border: 1px solid #b0bec5;
  background: white;
}

.bpga-section-title {
  background: linear-gradient(90deg, #f5f5f5 0%, #eeeeee 100%);
  font-weight: 600;
  font-size: 1.1rem;
  color: #01579b;
}

.bpga-validity-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 1rem;
  padding: 0.75rem;
  background: #fafafa;
  border-radius: 4px;
  border-left: 3px solid #90caf9;
}

.bpga-validity-badge {
  font-weight: 500;
  font-size: 0.85rem;
  padding: 0.25rem 0.75rem;
  border-radius: 12px;
  white-space: nowrap;
}

.bpga-valid {
  background: #c8e6c9;
  color: #2e7d32;
}

.bpga-invalid {
  background: #ffccbc;
  color: #d84315;
}

.bpga-error-info {
  padding: 0.75rem;
  background: #ffebee;
  border-left: 3px solid #ef5350;
  border-radius: 4px;
  color: #b71c1c;
  font-size: 0.9rem;
}

.bpga-state-section {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.bpga-state-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0.75rem 0;
  border-bottom: 1px solid #e0e0e0;
}

.bpga-state-item:last-child {
  border-bottom: none;
}

.bpga-label {
  font-weight: 500;
  color: #424242;
  font-size: 0.9rem;
}

.bpga-value {
  font-weight: 600;
  color: #01579b;
  font-size: 0.95rem;
}

.bpga-value.bpga-small {
  font-family: 'Courier New', monospace;
  font-size: 0.8rem;
}

.bpga-text-success {
  color: #2e7d32;
}

.bpga-text-error {
  color: #d84315;
}

.bpga-instrument {
  background: linear-gradient(135deg, #ffffff 0%, #f5f5f5 100%);
  border: 2px solid #90caf9;
  border-radius: 8px;
  transition: all 0.3s ease;
}

.bpga-instrument:hover {
  box-shadow: 0 4px 12px rgba(1, 87, 155, 0.2);
}

.bpga-instrument-content {
  text-align: center;
  padding: 1.5rem 1rem;
}

.bpga-instrument-label {
  font-size: 0.85rem;
  color: #666;
  font-weight: 500;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  margin-bottom: 0.5rem;
}

.bpga-instrument-value {
  font-size: 2rem;
  font-weight: 700;
  color: #01579b;
  font-family: 'Courier New', monospace;
}

.bpga-instrument-unit {
  font-size: 0.9rem;
  color: #0277bd;
  font-weight: 600;
  margin-top: 0.25rem;
}

.bpga-instrument-desc {
  font-size: 0.75rem;
  color: #999;
  margin-top: 0.75rem;
}

.bpga-nominal-text {
  margin: 0 0 1rem 0;
  color: #424242;
  font-size: 0.95rem;
}

.bpga-nominal-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(120px, 1fr));
  gap: 1rem;
}

.bpga-nominal-item {
  padding: 1rem;
  background: linear-gradient(135deg, #e8f5e9 0%, #c8e6c9 100%);
  border-radius: 6px;
  text-align: center;
  border-left: 3px solid #4caf50;
}

.bpga-nominal-value {
  display: block;
  font-size: 1.5rem;
  font-weight: 700;
  color: #2e7d32;
  font-family: 'Courier New', monospace;
}

.bpga-nominal-unit {
  display: block;
  font-size: 0.85rem;
  color: #558b2f;
  font-weight: 500;
  margin-top: 0.3rem;
}

.bpga-total-item,
.bpga-diag-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0.75rem;
  background: #fafafa;
  border-radius: 4px;
  border-bottom: 1px solid #e0e0e0;
}

.bpga-total-item .bpga-value,
.bpga-diag-item .bpga-value {
  font-family: 'Courier New', monospace;
  font-size: 0.9rem;
}

.bpga-safety-text,
.bpga-context-text {
  line-height: 1.6;
  color: #333;
  font-size: 0.95rem;
}

.bpga-safety-text p:first-child,
.bpga-context-text p:first-child {
  margin-top: 0;
}

.bpga-safety-text p:last-child,
.bpga-context-text p:last-child {
  margin-bottom: 0;
}

@media (max-width: 960px) {
  .bpga-header {
    flex-direction: column;
    gap: 1rem;
  }

  .bpga-header-left {
    width: 100%;
  }

  .bpga-header-right {
    width: 100%;
    justify-content: flex-start;
  }

  .bpga-header-text h1 {
    font-size: 1.5rem;
  }

  .bpga-aircraft-symbol {
    font-size: 2rem;
  }

  .bpga-status-chip {
    flex-direction: row;
    flex-wrap: wrap;
    gap: 0.5rem;
  }

  .bpga-chip {
    font-size: 0.8rem;
    padding: 0.4rem 0.8rem;
  }
}

@media (max-width: 600px) {
  .bpga-header {
    padding: 1rem;
  }

  .bpga-header-text h1 {
    font-size: 1.2rem;
  }

  .bpga-header-text p {
    font-size: 0.8rem;
  }

  .bpga-aircraft-symbol {
    font-size: 1.5rem;
  }

  .bpga-instrument-value {
    font-size: 1.5rem;
  }

  .bpga-nominal-grid {
    grid-template-columns: repeat(2, 1fr);
  }
}
</style>
