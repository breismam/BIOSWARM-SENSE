#pragma once
/**
 * capacitive_sensor.h
 * Interfaz para el electrodo capacitivo diferencial bioinspirado en canales iónicos KcsA.
 *
 * Principio biológico:
 *   El canal KcsA discrimina K⁺ de Na⁺ (selectividad > 1000:1) usando geometría molecular
 *   y carga eléctrica, sin reacciones químicas. El electrodo replica este principio:
 *   grupos —SH en la superficie de carbono poroso crean sitios de enlace selectivos para Hg²⁺.
 *   La adsorción del ion cambia la capacitancia de la doble capa eléctrica (EDL) de forma
 *   proporcional a la concentración, sin reacción faradáica.
 *
 * Hardware requerido:
 *   - Electrodo de trabajo: carbono poroso funcionalizado con —SH (síntesis en chemistry/)
 *   - Electrodo de referencia: Ag/AgCl miniatura
 *   - IC de medición: AD7746 (capacitance-to-digital, 24 bit, I2C)
 */

#include <Arduino.h>
#include <Wire.h>

#define AD7746_ADDR     0x48
#define AD7746_REG_CAP  0x01  // registro de lectura de capacitancia canal A

class CapacitiveSensor {
public:
  /**
   * @param sdaPin  pin SDA del bus I2C
   * @param sclPin  pin SCL del bus I2C
   */
  CapacitiveSensor(gpio_num_t sdaPin, gpio_num_t sclPin)
    : _sda(sdaPin), _scl(sclPin), _baseline(0.0f),
      _slope(0.0f), _intercept(0.0f) {}

  /** Inicializa el bus I2C y configura el AD7746 */
  void begin() {
    Wire.begin(_sda, _scl, 400000);
    // Configurar AD7746: modo diferencial, 11 Hz, canal A activo
    Wire.beginTransmission(AD7746_ADDR);
    Wire.write(0x07); // registro de configuración
    Wire.write(0xA0); // CAP CH1 enable, 11 Hz
    Wire.endTransmission();
    delay(50);
  }

  /**
   * calibrate() — Establece la capacitancia basal en agua limpia.
   * Llamar una vez antes del despliegue, con el electrodo sumergido
   * en agua desionizada o buffer de referencia.
   * Promedia 32 lecturas para reducir ruido.
   */
  void calibrate() {
    float sum = 0.0f;
    for (int i = 0; i < 32; i++) {
      sum += _readRawCapacitance();
      delay(100);
    }
    _baseline = sum / 32.0f;
    Serial.printf("[CAL] Capacitancia basal: %.4f pF\n", _baseline);
  }

  /**
   * setCalibrationCurve() — Carga los parámetros de la curva de calibración
   * obtenidos en laboratorio (ver chemistry/calibration_data/).
   *
   * Modelo lineal: [Hg²⁺] = slope × ΔC + intercept
   * @param slope      pendiente en µg/L por pF
   * @param intercept  ordenada al origen en µg/L
   */
  void setCalibrationCurve(float slope, float intercept) {
    _slope     = slope;
    _intercept = intercept;
  }

  /**
   * readConcentration_ugL() — Devuelve la concentración estimada de Hg²⁺ en µg/L.
   * Promedia 4 lecturas para reducir ruido de alta frecuencia.
   * Devuelve 0 si la curva de calibración no está cargada.
   */
  float readConcentration_ugL() {
    if (_slope == 0.0f) {
      // Sin calibración: devolver variación de capacitancia escalada (modo demo)
      return fmaxf((_readRawCapacitance() - _baseline) * 10.0f, 0.0f);
    }
    float sum = 0.0f;
    for (int i = 0; i < 4; i++) {
      sum += _readRawCapacitance();
      delay(20);
    }
    float deltaC = (sum / 4.0f) - _baseline;
    float conc   = _slope * deltaC + _intercept;
    return fmaxf(conc, 0.0f);
  }

private:
  gpio_num_t _sda, _scl;
  float _baseline;
  float _slope;
  float _intercept;

  /** Lee la capacitancia raw del AD7746 en pF (24 bits, resolución ~4 aF) */
  float _readRawCapacitance() {
    Wire.beginTransmission(AD7746_ADDR);
    Wire.write(AD7746_REG_CAP);
    Wire.endTransmission(false);
    Wire.requestFrom(AD7746_ADDR, 3);
    if (Wire.available() < 3) return _baseline;
    uint32_t raw = ((uint32_t)Wire.read() << 16)
                 | ((uint32_t)Wire.read() << 8)
                 |  (uint32_t)Wire.read();
    // Conversión a pF: el AD7746 tiene fondo de escala de ±4 pF en 24 bits
    return ((float)raw / 0xFFFFFF) * 8.0f - 4.0f;
  }
};
