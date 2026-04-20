# Síntesis del Electrodo Capacitivo Bioinspirado (—SH)

## Principio biológico

El canal de potasio KcsA discrimina K⁺ de Na⁺ con selectividad > 1000:1 usando geometría
molecular y distribución de cargas, sin reacciones químicas. El electrodo replica este
principio: grupos tiol (—SH) en la superficie del carbono poroso crean sitios de enlace
altamente selectivos para Hg²⁺, por la afinidad de Hg²⁺ con el azufre (Principio HSAB).

La adsorción del ion cambia la capacitancia de la doble capa eléctrica (EDL) de forma
proporcional a la concentración, sin transferencia de electrones (sin reacción faradáica).

---

## Materiales

| Reactivo | Cantidad | Proveedor sugerido | Notas |
|---|---|---|---|
| Carbono activado (< 50 µm) | 2 g | Sigma-Aldrich 242276 | BET > 1000 m²/g |
| 3-Mercaptopropionic acid (3-MPA) | 5 mL | Sigma-Aldrich 57260 | Fuente de grupos —SH |
| NaOH (0.1 M) | 50 mL | Merck | Ajuste de pH |
| HCl (0.1 M) | 20 mL | Merck | Lavado ácido |
| Nafion 5% en etanol | 1 mL | Sigma-Aldrich 70160 | Ligante del electrodo |
| ITO sobre vidrio (1×2 cm) | 5 unidades | Thermo Fisher | Sustrato conductor |
| HgCl₂ (patrón certificado) | 1 mL (100 mg/L) | Merck Suprapur | Solo en laboratorio certificado |
| Pb(NO₃)₂ (patrón certificado) | 1 mL (100 mg/L) | Merck Suprapur | Para pruebas de selectividad |

---

## Protocolo de síntesis

### Paso 1 — Funcionalización del carbono (—SH)

1. Pesar 500 mg de carbono activado en vaso de precipitados de 100 mL.
2. Agregar 25 mL de HCl 0.1 M. Agitar con barra magnética 30 min a temperatura ambiente.
   *Objetivo: remover impurezas metálicas superficiales.*
3. Filtrar y lavar con agua desionizada (3 × 10 mL) hasta pH neutro.
4. Redispersar en 20 mL de agua desionizada. Ajustar pH a 5.0 con NaOH 0.1 M.
5. Agregar 200 µL de ácido 3-mercaptopropiónico (3-MPA). Agitar 12 h a 25°C en oscuridad.
   *El 3-MPA se adsorbe sobre el carbono mediante interacciones π–π y enlaces C–S.*
6. Filtrar y lavar con agua desionizada (3 × 10 mL) hasta que el filtrado no huela a 3-MPA.
7. Secar en horno de vacío a 60°C durante 4 h.
8. Conservar en frasco ámbar a 4°C hasta uso.

**Verificación:** La presencia de grupos —SH se confirma por FTIR (banda S—H a ~2550 cm⁻¹).

---

### Paso 2 — Preparación de la tinta de electrodo

1. Pesar 10 mg del carbono funcionalizado (—SH).
2. Agregar 200 µL de Nafion 5% en etanol.
3. Sonicar 30 min en baño de ultrasonido hasta obtener suspensión homogénea.

---

### Paso 3 — Deposición sobre sustrato ITO

1. Limpiar el sustrato ITO con etanol y agua desionizada. Secar con N₂.
2. Depositar 10 µL de la tinta de electrodo mediante drop-casting.
3. Secar al aire 2 h a temperatura ambiente.
4. Curar en horno a 80°C durante 1 h.
5. Almacenar en desecador hasta uso.

---

## Curva de calibración (protocolo)

### Preparación de patrones

| Concentración (µg/L) | Preparación |
|---|---|
| 0 (blanco) | Agua ultrapura o agua de río sintética |
| 0.1 | Dilución del patrón 100 mg/L: 1:1,000,000 |
| 0.5 | Dilución 1:200,000 |
| 1.0 | Dilución 1:100,000 (límite OMS) |
| 5.0 | Dilución 1:20,000 |
| 10.0 | Dilución 1:10,000 |
| 50.0 | Dilución 1:2,000 |

Para agua de río sintética: ajustar pH a 5.5 con HCl 0.1 M + agregar 5 mg/L de ácido
húmico (simula la materia orgánica disuelta del Amazonas).

### Medición EIS/capacitiva

- Instrumento: potenciostato de impedancia (ej. Autolab PGSTAT204 o Palmsens4)
- Configuración: EIS a circuito abierto, frecuencia 100 Hz, amplitud 10 mV
- Extraer: capacitancia de la doble capa (Cdl) del circuito equivalente Randles
- Calcular: ΔCdl = Cdl(muestra) − Cdl(blanco)
- Ajuste lineal: [Hg²⁺] = a × ΔCdl + b (R² esperado > 0.99)

---

## Pruebas de selectividad

Ensayar la respuesta del electrodo en presencia de iones competidores a concentraciones
típicas del río Amazonas:

| Ion competidor | Concentración ensayada | Referencia ambiental |
|---|---|---|
| Fe³⁺ | 0.5 mg/L | Aguas ácidas amazónicas típicas |
| Cu²⁺ | 0.1 mg/L | CONAMA resolución 357 |
| Ca²⁺ | 50 mg/L | Dureza típica del agua amazónica |
| Mg²⁺ | 10 mg/L | Idem |
| Pb²⁺ | 1 µg/L | Verificar cruce de señal |

Calcular el coeficiente de selectividad potenciométrica Kij para cada ion competidor.
Criterio de aceptación: Kij < 0.05 (interferencia < 5%) para todos los iones evaluados.

---

## Seguridad y disposición de residuos

- Trabajar con HgCl₂ únicamente en campana extractora con EPP completo (guantes de nitrilo doble, respirador).
- Los residuos con Hg²⁺ deben recolectarse en recipiente etiquetado "Residuos de mercurio" y entregarse al gestor de residuos peligrosos de la institución.
- El 3-MPA es irritante — evitar inhalación y contacto con piel.

---

## Referencias

- Doyle, D. A. et al. (1998). The structure of the potassium channel. *Science*, 280, 69–77.
- Li, M. et al. (2022). Electrochemical sensors for heavy metal detection. *TrAC*, 142, 116351.
- Jariwala, D. et al. (2021). Functionalized porous carbon electrodes for capacitive heavy metal ion detection. *ACS Applied Materials & Interfaces*, 13(8), 9810–9822.
