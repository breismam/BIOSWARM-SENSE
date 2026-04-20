# Esquemático del Nodo Sensor BioSwarm-Sense v1

> El archivo KiCad (.kicad_sch) se generará durante los días 3–4 del plan de implementación.  
> Este documento describe las conexiones de forma textual para referencia del equipo.

---

## Diagrama de bloques

```
[ENERGÍA]                    [SENSADO]               [COMUNICACIÓN]
    |                            |                         |
[Piezo 28mm] ──► [Rectif.]   [Electrodo —SH]         [BLE 5.0]
[LiPo 500mAh] ──► [MPPT] ──► [3.3V LDO] ──► [ESP32-C3] ──► [LoRa SX1276]
[Supercap 1F] ──►             |        |          |
                          [AD7746] [GPS M8Q]   [LED estado]
                              |
                         (I2C: SDA/SCL)
```

---

## Conexiones pin a pin — ESP32-C3

| Pin ESP32-C3 | Función | Conectado a | Notas |
|---|---|---|---|
| GPIO4 (SDA) | I2C SDA | AD7746 SDA + GPS SDA | Pull-up 4.7 kΩ a 3.3V |
| GPIO5 (SCL) | I2C SCL | AD7746 SCL + GPS SCL | Pull-up 4.7 kΩ a 3.3V |
| GPIO6 (RX1) | UART1 RX | GPS TX (u-blox M8Q) | Nivel 3.3V |
| GPIO7 (TX1) | UART1 TX | GPS RX (u-blox M8Q) | Nivel 3.3V |
| GPIO10 (SPI MOSI) | SPI MOSI | LoRa SX1276 MOSI | — |
| GPIO8  (SPI MISO) | SPI MISO | LoRa SX1276 MISO | — |
| GPIO9  (SPI CLK)  | SPI CLK  | LoRa SX1276 SCK  | — |
| GPIO2 (CS) | SPI CS | LoRa SX1276 NSS | Active LOW |
| GPIO3 (RST) | Reset LoRa | LoRa SX1276 RST | Active LOW |
| GPIO1 (IRQ) | Interrupt | LoRa SX1276 DIO0 | Flanco subida |
| GPIO0 | LED estado | LED + 220Ω → GND | Verde=idle, Rojo=alerta |
| 3V3 | Alimentación | AD7746 VDD, GPS VCC, LoRa VCC | Max 3.6V |
| GND | Tierra común | Todos los GND | Estrella desde LDO |

---

## Circuito de energía

### Harvesting piezoeléctrico
```
[Piezo 28mm]
    |  AC (vibraciones corriente fluvial)
[Puente rectificador 4× 1N5819]
    |  DC ~3.0–4.5V
[Supercapacitor 1F 5V] ──── buffer de energía
    |
[MCP73831 — cargador LiPo] ──── [LiPo 3.7V 500 mAh]
    |
[AMS1117-3.3 — LDO 3.3V, 800 mA] ──── VCC del sistema
```

### Consumo estimado por estado

| Estado del nodo | Consumo promedio |
|---|---|
| Deep sleep (entre broadcasts idle) | ~50 µA |
| Activo — medición + BLE broadcast | ~28 mA (50 ms) |
| Alerta — broadcast continuo + LoRa | ~42 mA |
| GPS activo (adquisición) | +25 mA |
| GPS Power Save Mode | +15 µA |

**Autonomía estimada (sin harvesting):**  
LiPo 500 mAh — ciclo idle (broadcast cada 5 s, 50 ms activo):  
Consumo promedio ≈ 0.28 mA → autonomía > **70 días**

---

## Electrodo capacitivo — conexión al AD7746

```
[Electrodo de trabajo (carbono —SH)]  ──► AD7746 CIN1(+)
[Electrodo de referencia (Ag/AgCl)]   ──► AD7746 CIN1(-)
[Blindaje coaxial del cable]          ──► AD7746 SHLD (guard drive)
```

La medición diferencial entre los dos electrodos cancela el efecto capacitivo del agua
(variación de temperatura, conductividad) y retiene solo la señal de adsorción del Hg²⁺.

---

## Notas de diseño para la PCB (KiCad)

- Pistas del electrodo: usar cobre recubierto con ENIG (no HASL) para evitar oxidación.
- Separar el plano de tierra analógico (AD7746) del digital (ESP32) con ferrite bead 600Ω @ 100 MHz.
- El oscilador del GPS requiere área libre de vías de 5 mm alrededor.
- Antena BLE del ESP32-C3: no cubrir con plano de cobre en la cara de componentes.
- Conector del electrodo: JST SH 1.0 mm 4 pines (WE, RE, SHLD, GND), sellado con epoxi marino.
