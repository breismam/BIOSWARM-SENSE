# 🐜 BioSwarm-Sense

**Red Distribuida de Sensores Capacitivos Bioinspirados para Monitoreo de Contaminantes en Ríos Amazónicos**

> Ideatón de Innovación Inspirada en la Inteligencia de la Naturaleza  
> Oxford TIDE Centre — CAF Banco de Desarrollo de América Latina y el Caribe  
> Convocatoria 2025–2026

---

## Resumen ejecutivo

BioSwarm-Sense es una red distribuida de micro-sensores capacitivos para monitoreo de contaminantes (Hg²⁺, Pb²⁺, microplásticos) en ríos amazónicos. Cada nodo emula el comportamiento de una hormiga exploradora: detecta gradientes químicos de forma autónoma, comunica hallazgos a nodos vecinos mediante un **protocolo de feromona digital**, y genera un mapa de contaminación en tiempo real sin infraestructura central.

El sistema combina **dos niveles de bioinspiración**:
1. **Inteligencia colectiva de colonias de hormigas** — arquitectura de comunicación distribuida y tolerante a fallos
2. **Selectividad de canales iónicos de membrana celular (KcsA)** — detección capacitiva sin reactivos químicos

---

## Problema que resuelve

La cuenca amazónica enfrenta una crisis de contaminación hídrica por mercurio (Hg²⁺) asociada a la minería ilegal de oro. Las concentraciones detectadas superan **10–50× los límites de la OMS** (1 µg/L). El monitoreo tradicional es puntual, lento y costoso. BioSwarm-Sense propone un monitoreo continuo, distribuido y de bajo costo.

| Monitoreo tradicional | BioSwarm-Sense |
|---|---|
| Muestreo manual cada semanas | Monitoreo continuo 24/7 |
| Resultado en días (laboratorio) | Alerta local en segundos |
| 2–10 estaciones fijas | Decenas de nodos móviles |
| Requiere personal especializado | Autónomo una vez desplegado |
| USD 500–2000 por punto | USD 25–50 por nodo |

---

## Principio bioinspirado

### Capa 1 — Colonias de hormigas (Formicidae)
Cada nodo sensor replica el comportamiento de una hormiga exploradora:

| Hormiga | Nodo sensor |
|---|---|
| Quimiosensor en antenas (ppb) | Electrodo capacitivo diferencial |
| Depósito de feromona | Incremento de frecuencia de broadcast BLE/LoRa |
| Refuerzo del rastro | Peso de enrutamiento proporcional a la lectura |
| Evaporación gradual | Decaimiento exponencial de la feromona digital |
| Levy walk (exploración aleatoria) | Deriva pasiva en la corriente del río |

### Capa 2 — Canales iónicos KcsA
El electrodo sensor imita la selectividad del canal de potasio KcsA (selectividad K⁺/Na⁺ > 1000:1): una superficie de carbono poroso funcionalizado con grupos —SH genera variación de capacitancia interfacial al acercarse los iones objetivo, **sin reacción faradáica ni reactivos**.

---

## Arquitectura del sistema

```
[FUENTE Hg²⁺]
      ↓ pluma deriva con corriente
╭─────────────────────────────────╮  ORILLA
│                                  │  [GW] Gateway solar + LoRa
│   CAUCE DEL RÍO (nodos flotantes)│     ↓ internet
│                                  │  [NUBE] Dashboard público
│  (N1)──(N2)──(N3)──(N4)         │
│    \      |      |    /          │
│     (N5)─(N6)─(N7)              │
│             ↓ alerta             │
╰─────────────────────────────────╯
  ▶▶▶  corriente del río  ▶▶▶
```

Cada nodo incorpora:
- **ESP32-C3** — microprocesador ultra bajo consumo con BLE integrado
- **Electrodo capacitivo** — carbono poroso funcionalizado (—SH / —COOH)
- **GPS u-blox M8Q** — georeferenciación en tiempo real
- **Harvesting piezoeléctrico/térmico** — energía de la corriente del río
- **Amarre flexible** — cabo de polietileno 3–10 m a boya fija (evita arrastre)
- **Carcasa HDPE IP68** — sellado hermético para inmersión

---

## Estructura del repositorio

```
bioswarm-sense/
├── README.md                        ← Este archivo
├── docs/
│   ├── BioSwarm_Sense_Propuesta.docx  ← Propuesta completa
│   └── references/
│       └── bibliografia_anotada.md
├── simulation/
│   ├── index.html                   ← Simulación interactiva (abrir en browser)
│   ├── simulation.js                ← Lógica del sistema
│   └── README.md
├── firmware/
│   ├── node_firmware/
│   │   ├── main.ino
│   │   ├── pheromone_protocol.h
│   │   ├── capacitive_sensor.h
│   │   └── gps_tracker.h
│   └── gateway/
│       ├── gateway.ino
│       └── uplink_manager.h
├── hardware/
│   ├── schematic/
│   │   └── node_v1_schematic.md
│   └── bom/
│       └── bill_of_materials.csv
├── chemistry/
│   ├── electrode_synthesis.md
│   └── calibration_data/
│       └── hg_calibration_curve.csv
└── team/
    └── team_profile.md
```

---

## Demo rápida

Para ejecutar la simulación localmente:

```bash
git clone https://github.com/breismam/BIOSWARM-SENSE.git
cd BIOSWARM-SENSE/simulation
# Abrir index.html en cualquier navegador moderno
```

No requiere servidor ni dependencias adicionales — es HTML/JS puro.

---

## Validación técnica

### Selectividad y sensibilidad
- Electrodos —SH: límite de detección Hg²⁺ reportado en literatura: **0.1–2.0 µg/L** (< límite OMS)
- Protocolo de validación propuesto: curvas de calibración en 3 matrices (agua ultrapura, agua de grifo, agua amazónica sintética pH 5.5) + comparación con ICP-MS en campo
- Métrica de éxito: error relativo < 20%, falsos positivos < 10% a concentraciones ≥ 2 µg/L

### Georeferenciación
- GPS en cada nodo transmite coordenadas con cada paquete de datos
- Dashboard muestra posición en tiempo real sobre mapa satelital
- Última posición GPS registrada en caso de desconexión

### Tolerancia a pérdida de nodos
- Amarre flexible: nodo contenido en radio limitado del cauce, sin riesgo de arrastre irreversible
- Redundancia swarm: opera correctamente con hasta 40% de nodos perdidos
- Alerta automática si un nodo no transmite en > 15 minutos (GPS permite localización física)

---

## Equipo

| Integrante | Formación | Contribución |
|---|---|---|
| Breismam Alfonso Rueda Díaz | MSc Ingeniería Electrónica y de Computadores, Uniandes | Hardware embebido, firmware, simulación, prototipo ESP32 |
| Sergio Esteban Reyes Henao | PhD Ingeniería Química, Uniandes | Síntesis electrodo, caracterización FTIR/SEM/BET, curvas calibración, selectividad |
| Cielo Maritza León Ramos | PhD Biología (Biología Ambiental / Ecotoxicología) | Fundamentación biológica, etología Formicidae, ecotoxicología, vinculación conocimiento indígena |

---

## Alineación con el Ideatón

| Requisito | Cumplimiento |
|---|---|
| Bioinspiración explícita | Formicidae (comunicación) + KcsA (detección) |
| Área de desafío | Área 7 (contaminación) + Área 10 (inteligencia colectiva) |
| Impacto ALC | Hg²⁺ por minería ilegal: Colombia, Perú, Brasil |
| Prototipo / prueba de concepto | Simulación interactiva + firmware ESP32 + datos electrodo |
| Equipo ≥ 2 integrantes ALC | 3 integrantes colombianos |
| Interdisciplinariedad (extra) | Electrónica + Química + Biología |

---

## Referencias clave

- Benyus, J. M. (1997). *Biomimicry: Innovation inspired by nature.* Morrow.
- Dorigo, M. & Gambardella, L. M. (1997). Ant colony system. *IEEE Trans. Evolutionary Computation*, 1(1), 53–66.
- Doyle, D. A. et al. (1998). Structure of the potassium channel. *Science*, 280(5360), 69–77.
- Li, M. et al. (2022). Electrochemical sensors for heavy metal detection. *TrAC*, 142, 116351.
- UNEP (2018). *Global Mercury Assessment 2018.*
- Procuraduría General de la Nación (2024). *Minería ilegal y contaminación por mercurio en Colombia.*

---

## Contacto

**tide@qeh.ox.ac.uk** — Asunto: Nature's Intelligence Ideathon 2025 – Consulta

*Bogotá, Colombia — Abril 2026*
