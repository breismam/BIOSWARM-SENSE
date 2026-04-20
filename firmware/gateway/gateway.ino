/**
 * BioSwarm-Sense — Firmware gateway
 * Plataforma: ESP32-S3 (Arduino framework)
 *
 * El gateway se ubica en la orilla del río (tierra firme).
 * Recibe paquetes BLE de los nodos del cauce y los agrega
 * para enviarlos a la nube vía LoRa / WiFi / 4G.
 *
 * Hardware:
 *   - ESP32-S3 + módulo LoRa SX1276 (868 MHz)
 *   - Panel solar 10W + LiPo 3000 mAh
 *   - Antena LoRa exterior (ganancia 3 dBi)
 */

#include <Arduino.h>
#include "uplink_manager.h"
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <ArduinoJson.h>

#define GATEWAY_ID        "GW-001"
#define LORA_SS_PIN       5
#define LORA_RST_PIN      14
#define LORA_DIO0_PIN     2
#define SCAN_INTERVAL_MS  500   // ms entre escaneos BLE
#define UPLINK_INTERVAL   30000 // ms entre envíos a la nube

// ── Estructura de datos recibida del nodo ────────────────────────────────────
struct BioSwarmPacket {
  uint8_t  nodeId;
  uint8_t  state;
  uint16_t concentration;  // µg/L × 10
  uint16_t pheromoneVal;
  int32_t  lat;
  int32_t  lon;
} __attribute__((packed));

// ── Buffer de lecturas agregadas ─────────────────────────────────────────────
#define MAX_NODES 64
struct NodeRecord {
  uint8_t  nodeId;
  float    concentration;
  float    pheromone;
  float    lat, lon;
  uint32_t lastSeen;
  uint8_t  state;
  bool     active;
};
NodeRecord registry[MAX_NODES] = {};

UplinkManager uplink(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
unsigned long lastUplink = 0;

// ── Callback BLE: procesa cada paquete recibido ──────────────────────────────
class BioSwarmCallback : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice dev) override {
    if (!dev.haveManufacturerData()) return;
    std::string mdata = dev.getManufacturerData();
    // Verificar que sea un paquete BioSwarm (company ID 0x0001)
    if (mdata.size() < sizeof(BioSwarmPacket) + 2) return;
    if (mdata[0] != 0x01 || mdata[1] != 0x00) return;

    const BioSwarmPacket* pkt = reinterpret_cast<const BioSwarmPacket*>(mdata.data() + 2);

    // Registrar lectura del nodo
    int idx = pkt->nodeId % MAX_NODES;
    registry[idx].nodeId        = pkt->nodeId;
    registry[idx].concentration = pkt->concentration / 10.0f;
    registry[idx].pheromone     = pkt->pheromoneVal  / 1000.0f;
    registry[idx].lat           = pkt->lat / 1e6f;
    registry[idx].lon           = pkt->lon / 1e6f;
    registry[idx].state         = pkt->state;
    registry[idx].lastSeen      = millis();
    registry[idx].active        = true;

    // Alerta inmediata si el nodo reporta lectura crítica
    if (pkt->state == 2) { // NODE_ALERT
      Serial.printf("[ALERTA CRITICA] Nodo %d: %.1f µg/L en (%.6f, %.6f)\n",
                    pkt->nodeId,
                    registry[idx].concentration,
                    registry[idx].lat,
                    registry[idx].lon);
      uplink.sendAlert(pkt->nodeId, registry[idx].concentration,
                       registry[idx].lat, registry[idx].lon);
    }
  }
};

BLEScan* pBLEScan = nullptr;
BioSwarmCallback bleCallback;

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("[GW] BioSwarm-Sense Gateway iniciando...");

  // BLE scan
  BLEDevice::init("BioSwarm-GW");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(&bleCallback, false);
  pBLEScan->setActiveScan(false); // modo pasivo para ahorrar energía
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(90);

  // LoRa + uplink
  uplink.begin();

  Serial.println("[GW] Listo. Escaneando red BioSwarm...");
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop() {
  // Escaneo BLE continuo (500 ms por ciclo)
  pBLEScan->start(SCAN_INTERVAL_MS / 1000, false);
  pBLEScan->clearResults();

  // Marcar nodos inactivos (sin señal en los últimos 2 minutos)
  uint32_t now = millis();
  for (auto& rec : registry) {
    if (rec.active && (now - rec.lastSeen) > 120000) {
      Serial.printf("[WARN] Nodo %d sin señal por >2 min. Última pos: (%.6f, %.6f)\n",
                    rec.nodeId, rec.lat, rec.lon);
      rec.active = false;
    }
  }

  // Uplink periódico: enviar agregado de todas las lecturas activas
  if (now - lastUplink >= UPLINK_INTERVAL) {
    StaticJsonDocument<2048> doc;
    doc["gw"]  = GATEWAY_ID;
    doc["ts"]  = now;
    JsonArray arr = doc.createNestedArray("nodes");
    for (const auto& rec : registry) {
      if (!rec.active) continue;
      JsonObject n = arr.createNestedObject();
      n["id"]    = rec.nodeId;
      n["conc"]  = rec.concentration;
      n["ph"]    = rec.pheromone;
      n["state"] = rec.state;
      n["lat"]   = rec.lat;
      n["lon"]   = rec.lon;
      n["age_s"] = (now - rec.lastSeen) / 1000;
    }
    String payload;
    serializeJson(doc, payload);
    uplink.sendPayload(payload);
    lastUplink = now;
    Serial.println("[GW] Uplink enviado: " + payload.substring(0, 80) + "...");
  }
}
