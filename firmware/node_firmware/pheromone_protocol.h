#pragma once
/**
 * pheromone_protocol.h
 * Implementación del protocolo de feromona digital bioinspirado en Formicidae.
 *
 * Analogía biológica:
 *   - Depósito de feromona  → incremento de pheromone.value al detectar contaminante
 *   - Evaporación gradual   → decay() reduce el valor exponencialmente
 *   - Refuerzo por rastro   → receive() suma la feromona de nodos vecinos atenuada por distancia
 *   - Olvido                → sin lecturas positivas, el valor converge a 0
 */

#include <Arduino.h>

// Estado del nodo (mapeado a colores en la simulación)
enum NodeState : uint8_t {
  NODE_IDLE      = 0,  // verde — escucha pasiva
  NODE_PHEROMONE = 1,  // naranja — feromona activa recibida de vecinos
  NODE_ALERT     = 2,  // rojo — lectura supera umbral
  NODE_UPLINK    = 3,  // morado — paquete en tránsito al gateway
};

struct PheromoneState {
  uint8_t  nodeId;
  float    value;      // 0.0 – 1.0
  float    decayRate;  // factor multiplicativo por ciclo (ej. 0.992)

  /**
   * decay() — Evaporación de feromona.
   * Llamar una vez por segundo.
   * Análogo biológico: la feromona se evapora en ausencia de refuerzo.
   */
  void decay() {
    value *= decayRate;
    if (value < 0.001f) value = 0.0f;
  }

  /**
   * receive() — Recibir feromona de un nodo vecino.
   * @param incomingValue  valor de feromona del vecino (0.0–1.0)
   * @param distance_m     distancia al vecino en metros
   * @param commRadius_m   radio máximo de comunicación en metros
   *
   * La atenuación por distancia replica el gradiente de concentración
   * de feromonas en la naturaleza: mayor concentración cerca de la fuente.
   */
  void receive(float incomingValue, float distance_m, float commRadius_m) {
    if (distance_m >= commRadius_m) return;
    float attenuation = 1.0f - (distance_m / commRadius_m);
    float contribution = incomingValue * attenuation * 0.4f;
    value = fminf(value + contribution, 1.0f);
  }

  /**
   * deposit() — Depositar feromona al detectar contaminante.
   * @param concentration_ugL  lectura del sensor en µg/L
   * @param threshold_ugL      umbral de alerta configurado
   *
   * La cantidad depositada es proporcional al exceso sobre el umbral,
   * replicando el comportamiento de refuerzo de rastro de las hormigas.
   */
  void deposit(float concentration_ugL, float threshold_ugL) {
    if (concentration_ugL < threshold_ugL) return;
    float excess = (concentration_ugL - threshold_ugL) / threshold_ugL;
    float amount = fminf(excess * 0.3f + 0.1f, 0.5f);
    value = fminf(value + amount, 1.0f);
  }

  /** Devuelve el intervalo de broadcast en ms según el nivel de feromona */
  unsigned long broadcastInterval_ms(unsigned long idleMs, unsigned long activeMs) const {
    if (value < 0.05f) return idleMs;
    // Interpolación lineal: a mayor feromona, menor intervalo
    float t = fminf(value / 0.5f, 1.0f);
    return (unsigned long)(idleMs - t * (idleMs - activeMs));
  }
};
