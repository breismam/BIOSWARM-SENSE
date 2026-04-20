# Simulación BioSwarm-Sense

Simulación interactiva del protocolo de feromona digital en JavaScript/Canvas puro.  
No requiere instalación ni servidor — abre `index.html` directamente en el navegador.

## Cómo ejecutar

```bash
# Opción 1: abrir directamente
open simulation/index.html   # macOS
xdg-open simulation/index.html  # Linux

# Opción 2: servidor local simple (evita restricciones CORS)
cd simulation
python3 -m http.server 8080
# Abrir http://localhost:8080 en el navegador
```

## Qué simula

| Elemento | Descripción |
|---|---|
| Cauce del río | Curva sinuosa con orillas diferenciadas |
| Fuente Hg²⁺ | Pluma gaussiana que deriva con la corriente |
| Nodos sensor | 8–36 nodos con movimiento Levy walk dentro del cauce |
| Gateway | Nodo fijo en la orilla con LoRa / internet |
| Protocolo feromona | Propagación en tiempo real con decaimiento exponencial |
| Paquetes animados | Visualización de broadcast BLE/LoRa entre nodos |
| Heatmap | Mapa de concentración Hg²⁺ en tiempo real |
| Gráfico histórico | Concentración máxima detectada vs. umbral OMS |

## Estados de los nodos

| Color | Estado | Descripción |
|---|---|---|
| Verde oscuro | Idle | Escucha pasiva, consumo mínimo |
| Naranja | Feromona activa | Recibe propagación de vecinos |
| Rojo + halo | Alerta | Lectura supera umbral configurado |
| Morado | Uplink | Paquete en tránsito hacia el gateway |
| Cyan (rect) | Gateway | Nodo fijo en la orilla |

## Parámetros configurables (panel derecho)

| Parámetro | Rango | Efecto |
|---|---|---|
| Nodos | 8 – 36 | Mayor densidad → menor tiempo de detección |
| Radio comunicación | 50 – 200 px | Alcance BLE/LoRa simulado |
| Intensidad Hg²⁺ | 0 – 500 µg/L | Concentración de la fuente contaminante |
| Umbral alerta | 10 – 300 µg/L | OMS recomienda ≤ 1 µg/L para agua de consumo |

## Resultado clave

Con 18 nodos y radio de 100 px, la alerta se propaga por toda la red en < 3 s simulados.  
La feromona digital elimina la necesidad de enrutamiento centralizado.
