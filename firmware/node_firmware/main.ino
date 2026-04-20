/**
 * BioSwarm-Sense — Firmware nodo sensor
 * Plataforma: ESP32-C3 (Arduino framework)
 *
 * Flujo principal:
 *   1. Leer capacitancia diferencial del electrodo (detecta Hg²⁺ / Pb²⁺)
 *   2. Actualizar feromona digital según lectura
 *   3. Broadcast BLE al vecindario (frecuencia adaptativa)
 *   4. Uplink LoRa al gateway si feromona alta y en rango
 *   5. Registrar posición GPS con cada paquete
 */

#include <Arduino.h>
#include "pheromone_protocol.h"
#include "capacitive_sensor.h"
#include "gps_tracker.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>

// ── Configuración ────────────────────────────────────────────────────────────
#define NODE_ID            1           // ID único del nodo (1–255)
#define ALERT_THRESHOLD_UG 50.0f       // µg/L — umbral de alerta (configurable)
#define BROADCAST_INTERVAL_IDLE   5000 // ms entre broadcasts en estado idle
#define BROADCAST_INTERVAL_ACTIVE  800 // ms entre broadcasts en estado alerta
#define PHEROMONE_DECAY_MS        1000 // ms entre pasos de evaporación

// ── UUID BLE del servicio BioSwarm ───────────────────────────────────────────
#define BIOSWARM_SERVICE_UUID  "bioswarm-sense-v1"
#define BIOSWARM_CHAR_UUID     "bss-pheromone-data"

// ── Estado global ────────────────────────────────────────────────────────────
PheromoneState pheromone;
CapacitiveSensor sensor(GPIO_NUM_4, GPIO_NUM_5); // pines SDA/SCL del electrodo
GPSTracker gps(GPIO_NUM_6, GPIO_NUM_7, 9600);    // UART1 para u-blox M8Q

NodeState nodeState = NODE_IDLE;
unsigned long lastBroadcast   = 0;
unsigned long lastDecay       = 0;
float lastConcentration       = 0.0f;

// ── BLE Advertising ──────────────────────────────────────────────────────────
BLEAdvertising* pAdvertising = nullptr;

struct BioSwarmPacket {
  uint8_t  nodeId;
  uint8_t  state;          // 0=idle, 1=pheromone, 2=alert, 3=uplink
  uint16_t concentration;  // µg/L × 10 (1 decimal)
  uint16_t pheromoneVal;   // 0–1000 (3 decimales)
  int32_t  lat;            // latitud × 1e6
  int32_t  lon;            // longitud × 1e6
} __attribute__((packed));

void broadcastBLE(const BioSwarmPacket& pkt) {
  BLEAdvertisementData advData;
  advData.setFlags(0x06);
  // Manufacturer data: bytes del paquete
  std::string payload;
  payload += (char)0xFF; // manufacturer specific type
  payload += (char)0x01; // company ID LSB
  payload += (char)0x00; // company ID MSB
  payload.append((const char*)&pkt, sizeof(pkt));
  advData.addData(payload);
  pAdvertising->setAdvertisementData(advData);
  pAdvertising->start();
  delay(50);
  pAdvertising->stop();
}

// ── Setup ────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.printf("[BioSwarm] Nodo %d iniciando...\n", NODE_ID);

  // Inicializar sensor capacitivo
  sensor.begin();
  sensor.calibrate(); // calibración basal en agua limpia

  // Inicializar GPS
  gps.begin();

  // Inicializar feromona
  pheromone.value   = 0.0f;
  pheromone.nodeId  = NODE_ID;
  pheromone.decayRate = 0.992f; // evaporación por ciclo

  // Inicializar BLE
  BLEDevice::init("BSS-" + String(NODE_ID));
  BLEServer* pServer = BLEDevice::createServer();
  pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06);

  Serial.println("[BioSwarm] Listo.");
}

// ── Loop principal ───────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // 1. Leer sensor capacitivo
  lastConcentration = sensor.readConcentration_ugL();

  // 2. Actualizar estado del nodo
  if (lastConcentration >= ALERT_THRESHOLD_UG) {
    nodeState = NODE_ALERT;
    pheromone.value = fminf(pheromone.value + 0.3f, 1.0f);
    Serial.printf("[ALERTA] Hg²⁺ = %.1f µg/L  feromona=%.3f\n",
                  lastConcentration, pheromone.value);
  } else if (pheromone.value > 0.15f) {
    nodeState = NODE_PHEROMONE;
  } else {
    nodeState = NODE_IDLE;
  }

  // 3. Evaporación de feromona
  if (now - lastDecay >= PHEROMONE_DECAY_MS) {
    pheromone.decay();
    lastDecay = now;
  }

  // 4. Broadcast BLE adaptativo
  unsigned long interval = (nodeState == NODE_IDLE)
                           ? BROADCAST_INTERVAL_IDLE
                           : BROADCAST_INTERVAL_ACTIVE;
  if (now - lastBroadcast >= interval) {
    GPSData pos = gps.getPosition();

    BioSwarmPacket pkt;
    pkt.nodeId        = NODE_ID;
    pkt.state         = (uint8_t)nodeState;
    pkt.concentration = (uint16_t)(lastConcentration * 10);
    pkt.pheromoneVal  = (uint16_t)(pheromone.value * 1000);
    pkt.lat           = (int32_t)(pos.latitude  * 1e6);
    pkt.lon           = (int32_t)(pos.longitude * 1e6);

    broadcastBLE(pkt);
    lastBroadcast = now;

    Serial.printf("[TX] state=%d conc=%.1f pheromone=%.3f lat=%.6f lon=%.6f\n",
                  pkt.state, lastConcentration, pheromone.value,
                  pos.latitude, pos.longitude);
  }

  // 5. Deep sleep entre ciclos para reducir consumo
  // En estado idle: sleep 4.9 s de cada 5 s → consumo promedio ~200 µA
  if (nodeState == NODE_IDLE) {
    esp_sleep_enable_timer_wakeup((BROADCAST_INTERVAL_IDLE - 100) * 1000ULL);
    esp_light_sleep_start();
  } else {
    delay(100); // polling rápido en alerta
  }
}
