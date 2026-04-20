#pragma once
/**
 * uplink_manager.h
 * Gestión del canal de uplink del gateway hacia la nube.
 * Soporta LoRa (primario) con fallback a WiFi/4G.
 */

#include <Arduino.h>
#include <LoRa.h>

class UplinkManager {
public:
  UplinkManager(int ssPin, int rstPin, int dio0Pin)
    : _ss(ssPin), _rst(rstPin), _dio0(dio0Pin) {}

  void begin() {
    LoRa.setPins(_ss, _rst, _dio0);
    if (!LoRa.begin(868E6)) {
      Serial.println("[UPL] LoRa init falló — modo WiFi fallback");
      _loraOk = false;
    } else {
      LoRa.setSpreadingFactor(9);  // SF9: balance alcance/velocidad
      LoRa.setSignalBandwidth(125E3);
      LoRa.setCodingRate4(5);
      LoRa.setTxPower(17);         // 17 dBm (~50 mW)
      Serial.println("[UPL] LoRa OK @ 868 MHz SF9 BW125");
      _loraOk = true;
    }
  }

  /** Envía payload JSON al servidor remoto */
  void sendPayload(const String& json) {
    if (_loraOk) {
      LoRa.beginPacket();
      LoRa.print(json);
      LoRa.endPacket();
    } else {
      // TODO: implementar envío por WiFi/MQTT
      Serial.println("[UPL] WiFi fallback: " + json.substring(0, 60));
    }
    _txCount++;
  }

  /** Envío inmediato de alerta crítica (prioridad alta) */
  void sendAlert(uint8_t nodeId, float concentration, float lat, float lon) {
    String alert = "{\"alert\":1,\"node\":" + String(nodeId) +
                   ",\"conc\":" + String(concentration, 1) +
                   ",\"lat\":"  + String(lat, 6) +
                   ",\"lon\":"  + String(lon, 6) + "}";
    sendPayload(alert);
    Serial.println("[UPL] ALERTA enviada: " + alert);
  }

  uint32_t txCount() const { return _txCount; }

private:
  int      _ss, _rst, _dio0;
  bool     _loraOk  = false;
  uint32_t _txCount = 0;
};
