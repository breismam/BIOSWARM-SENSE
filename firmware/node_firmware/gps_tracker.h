#pragma once
/**
 * gps_tracker.h
 * Interfaz para el módulo GPS u-blox M8Q (UART).
 * Parsea tramas NMEA $GPRMC para extraer latitud, longitud y timestamp.
 *
 * Consumo:
 *   - Modo activo:  ~25 mA (adquisición continua)
 *   - Modo Power Save (1 Hz): ~15 µA promedio
 *   Se usa el modo Power Save durante el estado NODE_IDLE.
 */

#include <Arduino.h>

struct GPSData {
  float    latitude;   // grados decimales (norte positivo)
  float    longitude;  // grados decimales (este positivo)
  bool     valid;      // true si el fix es válido
  uint32_t timestamp;  // millis() del último fix válido
};

class GPSTracker {
public:
  /**
   * @param rxPin  pin RX del ESP32 (conectado a TX del u-blox)
   * @param txPin  pin TX del ESP32 (conectado a RX del u-blox)
   * @param baud   velocidad UART (u-blox M8Q: 9600 por defecto)
   */
  GPSTracker(gpio_num_t rxPin, gpio_num_t txPin, uint32_t baud)
    : _rx(rxPin), _tx(txPin), _baud(baud) {
    _data = { 0.0f, 0.0f, false, 0 };
  }

  void begin() {
    _serial.begin(_baud, SERIAL_8N1, _rx, _tx);
    // Enviar comando UBX para activar Power Save Mode
    _sendUBXPowerSave();
  }

  /**
   * update() — Procesa tramas NMEA disponibles en el buffer UART.
   * Llamar en cada iteración del loop cuando se necesite posición.
   */
  void update() {
    while (_serial.available()) {
      char c = _serial.read();
      _buffer[_bufIdx++] = c;
      if (_bufIdx >= sizeof(_buffer)) _bufIdx = 0;
      if (c == '\n') {
        _buffer[_bufIdx] = '\0';
        _parseNMEA(_buffer);
        _bufIdx = 0;
      }
    }
  }

  /**
   * getPosition() — Devuelve la última posición GPS válida.
   * Si no hay fix, devuelve los últimos valores conocidos.
   */
  GPSData getPosition() {
    update();
    return _data;
  }

  /** Activa el modo de bajo consumo (1 fix por segundo, sleep entre fixes) */
  void enablePowerSave() { _sendUBXPowerSave(); }

  /** Activa el modo de adquisición continua (durante despliegue inicial) */
  void enableContinuous() { _sendUBXContinuous(); }

private:
  HardwareSerial _serial{ 1 }; // UART1 del ESP32
  gpio_num_t _rx, _tx;
  uint32_t   _baud;
  GPSData    _data;
  char       _buffer[128];
  uint8_t    _bufIdx = 0;

  /** Parsea trama $GPRMC: mínima para obtener lat/lon */
  void _parseNMEA(const char* line) {
    if (strncmp(line, "$GPRMC", 6) != 0) return;
    // Campo 2: estado (A=válido, V=inválido)
    char* tok = strtok(const_cast<char*>(line), ",");
    int field = 0;
    float rawLat = 0, rawLon = 0;
    char latDir = 'N', lonDir = 'E', status = 'V';
    while (tok && field <= 6) {
      switch (field) {
        case 2: status = tok[0]; break;
        case 3: rawLat = atof(tok); break;
        case 4: latDir = tok[0]; break;
        case 5: rawLon = atof(tok); break;
        case 6: lonDir = tok[0]; break;
      }
      tok = strtok(nullptr, ",");
      field++;
    }
    if (status == 'A') {
      // Convertir DDDMM.MMMM → grados decimales
      int latDeg = (int)(rawLat / 100);
      int lonDeg = (int)(rawLon / 100);
      _data.latitude  = latDeg + (rawLat - latDeg * 100) / 60.0f;
      _data.longitude = lonDeg + (rawLon - lonDeg * 100) / 60.0f;
      if (latDir == 'S') _data.latitude  = -_data.latitude;
      if (lonDir == 'W') _data.longitude = -_data.longitude;
      _data.valid     = true;
      _data.timestamp = millis();
    }
  }

  /** Comando UBX-CFG-PMS: Power Save Mode 1 Hz */
  void _sendUBXPowerSave() {
    const uint8_t cmd[] = {
      0xB5, 0x62, 0x06, 0x86, 0x08, 0x00,
      0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x97, 0x6F
    };
    _serial.write(cmd, sizeof(cmd));
  }

  /** Comando UBX-CFG-PMS: Full Power (adquisición continua) */
  void _sendUBXContinuous() {
    const uint8_t cmd[] = {
      0xB5, 0x62, 0x06, 0x86, 0x08, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x94, 0x5A
    };
    _serial.write(cmd, sizeof(cmd));
  }
};
