/**
 * BioSwarm-Sense — Simulación del protocolo de feromona digital
 * Modela la dinámica de una red swarm de sensores de Hg²⁺ en un río amazónico.
 *
 * Principio bioinspirado:
 *   - Colonias de hormigas (Formicidae): comunicación por feromonas, Levy walk
 *   - Canales iónicos KcsA: detección capacitiva selectiva sin reactivos
 */

// ─── Canvas principal ────────────────────────────────────────────────────────
const canvas = document.getElementById('sim');
const ctx    = canvas.getContext('2d');
const W = canvas.width;
const H = canvas.height;

// ─── Canvas histórico ────────────────────────────────────────────────────────
const chartCanvas = document.getElementById('chart');
const cctx = chartCanvas.getContext('2d');
const CW = chartCanvas.width;
const CH = chartCanvas.height;

// ─── Parámetros globales ─────────────────────────────────────────────────────
let NUM_NODES     = 18;
let COMM_RADIUS   = 100;
let HG_INTENSITY  = 180;
let ALERT_THRESH  = 50;
let paused        = false;
let totalPackets  = 0;
let detectionTime = null;   // ms desde inicio
let startTime     = Date.now();

// ─── Controles UI ────────────────────────────────────────────────────────────
function bindControl(sliderId, valId, prop, setter) {
  const sl = document.getElementById(sliderId);
  const vl = document.getElementById(valId);
  sl.addEventListener('input', () => {
    vl.textContent = sl.value;
    setter(+sl.value);
  });
}
bindControl('r-nodes', 'v-nodes', 'NUM_NODES',    v => { NUM_NODES = v; init(); });
bindControl('r-radio', 'v-radio', 'COMM_RADIUS',  v => { COMM_RADIUS = v; });
bindControl('r-conc',  'v-conc',  'HG_INTENSITY', v => { HG_INTENSITY = v; });
bindControl('r-thr',   'v-thr',   'ALERT_THRESH', v => {
  ALERT_THRESH = v;
  document.getElementById('thr-label').textContent = `Umbral: ${v} µg/L`;
});
document.getElementById('thr-label').textContent = `Umbral: ${ALERT_THRESH} µg/L`;

document.getElementById('btn-reset').addEventListener('click', () => { init(); });
document.getElementById('btn-pause').addEventListener('click', function () {
  paused = !paused;
  this.textContent = paused ? '▶ Reanudar' : '⏸ Pausar';
});

// ─── Geometría del río ───────────────────────────────────────────────────────
// El cauce se define como una curva de Bezier segmentada.
// riverPath: array de puntos centrales del cauce (px)
// riverWidth: ancho del cauce en px
const RIVER_WIDTH = 120;

function buildRiverSpine() {
  return [
    { x: 0,   y: 240 },
    { x: 120, y: 200 },
    { x: 200, y: 280 },
    { x: 320, y: 180 },
    { x: 440, y: 300 },
    { x: 560, y: 200 },
    { x: 660, y: 260 },
    { x: 760, y: 220 },
  ];
}
const SPINE = buildRiverSpine();

/** Devuelve la posición interpolada a lo largo del eje del río (t ∈ [0,1]) */
function riverPoint(t) {
  const segs = SPINE.length - 1;
  const seg  = Math.min(Math.floor(t * segs), segs - 1);
  const lt   = (t * segs) - seg;
  const a    = SPINE[seg];
  const b    = SPINE[seg + 1];
  return { x: a.x + (b.x - a.x) * lt, y: a.y + (b.y - a.y) * lt };
}

/** Comprueba si un punto (x,y) está dentro del cauce del río */
function inRiver(x, y) {
  for (let t = 0; t <= 1; t += 0.02) {
    const p = riverPoint(t);
    if (Math.hypot(x - p.x, y - p.y) < RIVER_WIDTH / 2) return true;
  }
  return false;
}

/** Genera un punto aleatorio dentro del cauce */
function randomRiverPoint() {
  for (let i = 0; i < 200; i++) {
    const t = 0.05 + Math.random() * 0.9;
    const c = riverPoint(t);
    const r = (Math.random() - 0.5) * RIVER_WIDTH * 0.8;
    // Perpendicular al segmento
    const seg = Math.min(Math.floor(t * (SPINE.length-1)), SPINE.length-2);
    const dx  = SPINE[seg+1].x - SPINE[seg].x;
    const dy  = SPINE[seg+1].y - SPINE[seg].y;
    const len = Math.hypot(dx, dy) || 1;
    const px  = c.x + (-dy / len) * r;
    const py  = c.y + ( dx / len) * r;
    if (inRiver(px, py)) return { x: px, y: py };
  }
  return riverPoint(0.5); // fallback
}

// ─── Heatmap de concentración ─────────────────────────────────────────────────
const HM_RES  = 20;  // px por celda del heatmap
const HM_COLS = Math.ceil(W / HM_RES);
const HM_ROWS = Math.ceil(H / HM_RES);
const heatmap = new Float32Array(HM_COLS * HM_ROWS);

// ─── Fuente contaminante ──────────────────────────────────────────────────────
const source = {
  x: 60, y: 230,
  vx: 1.2,   // deriva con la corriente
  sigma: 60, // radio gaussiano de la pluma (px)
};

function concentration(x, y) {
  const d2 = (x - source.x) ** 2 + (y - source.y) ** 2;
  return HG_INTENSITY * Math.exp(-d2 / (2 * source.sigma ** 2));
}

function updateHeatmap() {
  for (let r = 0; r < HM_ROWS; r++) {
    for (let c = 0; c < HM_COLS; c++) {
      const px = c * HM_RES + HM_RES / 2;
      const py = r * HM_RES + HM_RES / 2;
      heatmap[r * HM_COLS + c] = inRiver(px, py) ? concentration(px, py) : 0;
    }
  }
}

function drawHeatmap() {
  const maxC = HG_INTENSITY || 1;
  for (let r = 0; r < HM_ROWS; r++) {
    for (let c = 0; c < HM_COLS; c++) {
      const v = heatmap[r * HM_COLS + c];
      if (v < 1) continue;
      const alpha = Math.min(v / maxC, 1) * 0.55;
      // Color: verde→amarillo→rojo según concentración
      const ratio = v / maxC;
      const R = Math.round(255 * Math.min(ratio * 2, 1));
      const G = Math.round(255 * Math.min((1 - ratio) * 2, 1));
      ctx.fillStyle = `rgba(${R},${G},20,${alpha})`;
      ctx.fillRect(c * HM_RES, r * HM_RES, HM_RES, HM_RES);
    }
  }
}

// ─── Paquetes animados ────────────────────────────────────────────────────────
const packets = [];

function spawnPacket(x0, y0, x1, y1, type) {
  packets.push({ x: x0, y: y0, tx: x1, ty: y1, t: 0, type });
  totalPackets++;
}

function updatePackets() {
  for (let i = packets.length - 1; i >= 0; i--) {
    const p = packets[i];
    p.t += 0.04;
    p.x = p.x + (p.tx - p.x) * 0.04 * 2;
    p.y = p.y + (p.ty - p.y) * 0.04 * 2;
    if (p.t >= 1) packets.splice(i, 1);
  }
}

function drawPackets() {
  for (const p of packets) {
    const colors = { pheromone: '#44ee88', alert: '#ff4444', uplink: '#cc66ff' };
    ctx.beginPath();
    ctx.arc(p.x, p.y, 3, 0, Math.PI * 2);
    ctx.fillStyle = colors[p.type] || '#ffffff';
    ctx.fill();
  }
}

// ─── Gateway ──────────────────────────────────────────────────────────────────
const gateway = { x: W - 28, y: 40 };

function drawGateway() {
  ctx.save();
  ctx.fillStyle = '#00ccff';
  ctx.strokeStyle = '#0088aa';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.roundRect(gateway.x - 16, gateway.y - 10, 32, 20, 4);
  ctx.fill(); ctx.stroke();
  ctx.fillStyle = '#003344';
  ctx.font = 'bold 9px monospace';
  ctx.textAlign = 'center';
  ctx.fillText('GW', gateway.x, gateway.y + 3);
  ctx.restore();
}

// ─── Nodos sensor ─────────────────────────────────────────────────────────────
let nodes = [];

class SensorNode {
  constructor(id) {
    this.id       = id;
    const p       = randomRiverPoint();
    this.x        = p.x;
    this.y        = p.y;
    this.vx       = 0;
    this.vy       = 0;
    this.pheromone = 0;       // 0.0 – 1.0
    this.state    = 'idle';   // idle | pheromone | alert | uplink
    this.reading  = 0;        // µg/L
    this.broadcastTimer = 0;
    this.uplinkTimer    = 0;
    this.levySteps      = 0;
    this.levyDx         = 0;
    this.levyDy         = 0;
  }

  sampleConcentration() {
    this.reading = concentration(this.x, this.y) + (Math.random() - 0.5) * 3;
  }

  update(dt, nodes) {
    // ── Derive pasiva con la corriente (Levy walk contenida en el cauce) ──
    if (this.levySteps <= 0) {
      const angle = Math.random() * Math.PI * 2;
      const dist  = 0.5 + Math.random() * 2.5;
      this.levyDx = Math.cos(angle) * dist * 0.4 + 0.3; // sesgo río abajo
      this.levyDy = Math.sin(angle) * dist * 0.15;
      this.levySteps = 20 + Math.floor(Math.random() * 60);
    }
    this.levySteps--;
    const nx = this.x + this.levyDx;
    const ny = this.y + this.levyDy;
    if (inRiver(nx, ny)) { this.x = nx; this.y = ny; }
    // rebote suave si sale del cauce
    if (this.x > W - 10) this.x = 30 + Math.random() * 60;

    // ── Muestreo del sensor ──
    this.sampleConcentration();

    // ── Evaporación de feromona ──
    this.pheromone *= 0.992;

    // ── Estado del nodo ──
    if (this.reading >= ALERT_THRESH) {
      this.state = 'alert';
      this.pheromone = Math.min(this.pheromone + 0.3, 1.0);
      if (detectionTime === null) detectionTime = Date.now() - startTime;
    } else if (this.pheromone > 0.15) {
      this.state = 'pheromone';
    } else {
      this.state = 'idle';
    }

    // ── Broadcast de feromona a vecinos ──
    this.broadcastTimer--;
    const bcastInterval = this.state === 'alert' ? 8 : this.state === 'pheromone' ? 20 : 50;
    if (this.broadcastTimer <= 0) {
      this.broadcastTimer = bcastInterval + Math.floor(Math.random() * 10);
      for (const nb of nodes) {
        if (nb === this) continue;
        const d = Math.hypot(nb.x - this.x, nb.y - this.y);
        if (d < COMM_RADIUS) {
          const type = this.state === 'alert' ? 'alert' : 'pheromone';
          spawnPacket(this.x, this.y, nb.x, nb.y, type);
          // El vecino recibe la feromona atenuada por distancia
          nb.pheromone = Math.min(nb.pheromone + this.pheromone * (1 - d / COMM_RADIUS) * 0.4, 1.0);
        }
      }
    }

    // ── Uplink al gateway cuando pheromone alta ──
    this.uplinkTimer--;
    if (this.state !== 'idle' && this.uplinkTimer <= 0) {
      const dGW = Math.hypot(gateway.x - this.x, gateway.y - this.y);
      if (dGW < COMM_RADIUS * 1.8) {
        spawnPacket(this.x, this.y, gateway.x, gateway.y, 'uplink');
        this.state = 'uplink';
        this.uplinkTimer = 60 + Math.floor(Math.random() * 40);
      }
    }
  }

  draw() {
    const colors = {
      idle:      '#2a8a4a',
      pheromone: '#e88c20',
      alert:     '#dd3333',
      uplink:    '#8844cc',
    };
    const r = this.state === 'alert' ? 8 : 6;
    ctx.save();

    // Halo en alerta
    if (this.state === 'alert') {
      ctx.beginPath();
      ctx.arc(this.x, this.y, 13, 0, Math.PI * 2);
      ctx.fillStyle = 'rgba(255,80,80,0.18)';
      ctx.fill();
    }

    // Nodo
    ctx.beginPath();
    ctx.arc(this.x, this.y, r, 0, Math.PI * 2);
    ctx.fillStyle = colors[this.state] || '#aaa';
    ctx.fill();
    if (this.state === 'alert') {
      ctx.strokeStyle = '#ff7777';
      ctx.lineWidth = 2;
      ctx.stroke();
    }

    // Etiqueta de concentración si está en alerta
    if (this.state === 'alert' || this.state === 'pheromone') {
      ctx.fillStyle = '#ffffff';
      ctx.font = '8px monospace';
      ctx.textAlign = 'center';
      ctx.fillText(this.reading.toFixed(0) + 'µ', this.x, this.y - 11);
    }

    ctx.restore();
  }

  drawLinks(nodes) {
    for (const nb of nodes) {
      if (nb.id <= this.id) continue;
      const d = Math.hypot(nb.x - this.x, nb.y - this.y);
      if (d >= COMM_RADIUS) continue;
      const phAvg = (this.pheromone + nb.pheromone) / 2;
      ctx.beginPath();
      ctx.moveTo(this.x, this.y);
      ctx.lineTo(nb.x, nb.y);
      if (phAvg > 0.25) {
        ctx.strokeStyle = `rgba(232,140,32,${phAvg * 0.7})`;
        ctx.lineWidth = 1.5 + phAvg * 2;
      } else {
        ctx.strokeStyle = 'rgba(60,120,180,0.18)';
        ctx.lineWidth = 0.5;
      }
      ctx.stroke();
    }
  }
}

// ─── Inicialización ───────────────────────────────────────────────────────────
function init() {
  nodes          = Array.from({ length: NUM_NODES }, (_, i) => new SensorNode(i));
  source.x       = 60;
  source.y       = 230;
  totalPackets   = 0;
  detectionTime  = null;
  startTime      = Date.now();
  packets.length = 0;
  chartHistory   = [];
}

// ─── Dibujo del río ───────────────────────────────────────────────────────────
function drawRiver() {
  // Orillas
  ctx.save();
  ctx.beginPath();
  ctx.moveTo(0, SPINE[0].y - RIVER_WIDTH / 2 - 15);
  for (let t = 0; t <= 1; t += 0.01) {
    const p   = riverPoint(t);
    const seg = Math.min(Math.floor(t * (SPINE.length-1)), SPINE.length-2);
    const dx  = SPINE[seg+1].x - SPINE[seg].x;
    const dy  = SPINE[seg+1].y - SPINE[seg].y;
    const len = Math.hypot(dx, dy) || 1;
    ctx.lineTo(p.x + (-dy / len) * (RIVER_WIDTH / 2 + 18),
               p.y + ( dx / len) * (RIVER_WIDTH / 2 + 18));
  }
  ctx.lineTo(W, 0); ctx.lineTo(0, 0); ctx.closePath();
  ctx.fillStyle = '#1a2e1a';
  ctx.fill();

  ctx.beginPath();
  ctx.moveTo(0, SPINE[0].y + RIVER_WIDTH / 2 + 15);
  for (let t = 0; t <= 1; t += 0.01) {
    const p   = riverPoint(t);
    const seg = Math.min(Math.floor(t * (SPINE.length-1)), SPINE.length-2);
    const dx  = SPINE[seg+1].x - SPINE[seg].x;
    const dy  = SPINE[seg+1].y - SPINE[seg].y;
    const len = Math.hypot(dx, dy) || 1;
    ctx.lineTo(p.x + ( dy / len) * (RIVER_WIDTH / 2 + 18),
               p.y + (-dx / len) * (RIVER_WIDTH / 2 + 18));
  }
  ctx.lineTo(W, H); ctx.lineTo(0, H); ctx.closePath();
  ctx.fillStyle = '#1a2e1a';
  ctx.fill();

  // Agua
  ctx.beginPath();
  for (let t = 0; t <= 1; t += 0.01) {
    const p   = riverPoint(t);
    const seg = Math.min(Math.floor(t * (SPINE.length-1)), SPINE.length-2);
    const dx  = SPINE[seg+1].x - SPINE[seg].x;
    const dy  = SPINE[seg+1].y - SPINE[seg].y;
    const len = Math.hypot(dx, dy) || 1;
    if (t === 0) ctx.moveTo(p.x + (-dy / len) * RIVER_WIDTH / 2, p.y + (dx / len) * RIVER_WIDTH / 2);
    else ctx.lineTo(p.x + (-dy / len) * RIVER_WIDTH / 2, p.y + (dx / len) * RIVER_WIDTH / 2);
  }
  for (let t = 1; t >= 0; t -= 0.01) {
    const p   = riverPoint(t);
    const seg = Math.min(Math.floor(t * (SPINE.length-1)), SPINE.length-2);
    const dx  = SPINE[seg+1].x - SPINE[seg].x;
    const dy  = SPINE[seg+1].y - SPINE[seg].y;
    const len = Math.hypot(dx, dy) || 1;
    ctx.lineTo(p.x + (dy / len) * RIVER_WIDTH / 2, p.y + (-dx / len) * RIVER_WIDTH / 2);
  }
  ctx.closePath();
  ctx.fillStyle = '#0d2a3a';
  ctx.fill();
  ctx.restore();
}

// ─── Gráfico histórico ────────────────────────────────────────────────────────
let chartHistory = [];
const CHART_MAX_POINTS = CW;

function updateChart(maxConc) {
  chartHistory.push(maxConc);
  if (chartHistory.length > CHART_MAX_POINTS) chartHistory.shift();
}

function drawChart() {
  cctx.clearRect(0, 0, CW, CH);
  cctx.fillStyle = '#070d18';
  cctx.fillRect(0, 0, CW, CH);

  if (chartHistory.length < 2) return;
  const maxVal = Math.max(HG_INTENSITY, ALERT_THRESH, 10);

  // Línea de umbral
  const ty = CH - (ALERT_THRESH / maxVal) * (CH - 6) - 2;
  cctx.setLineDash([4, 3]);
  cctx.strokeStyle = 'rgba(255,136,68,0.6)';
  cctx.lineWidth = 1;
  cctx.beginPath(); cctx.moveTo(0, ty); cctx.lineTo(CW, ty); cctx.stroke();
  cctx.setLineDash([]);

  // Curva
  cctx.beginPath();
  chartHistory.forEach((v, i) => {
    const x = (i / (CHART_MAX_POINTS - 1)) * CW;
    const y = CH - (v / maxVal) * (CH - 6) - 2;
    i === 0 ? cctx.moveTo(x, y) : cctx.lineTo(x, y);
  });
  cctx.strokeStyle = '#3a9fd5';
  cctx.lineWidth = 1.5;
  cctx.stroke();
}

// ─── Estadísticas UI ──────────────────────────────────────────────────────────
function updateStats() {
  const alertCount = nodes.filter(n => n.state === 'alert').length;
  document.getElementById('s-active').textContent = nodes.length;
  document.getElementById('s-alert').textContent  = alertCount;
  document.getElementById('s-pkts').textContent   = totalPackets;
  const td = document.getElementById('s-tdet');
  if (detectionTime !== null) {
    td.textContent = (detectionTime / 1000).toFixed(1) + 's';
  }
}

// ─── Loop principal ───────────────────────────────────────────────────────────
let lastFrame = 0;
function loop(ts) {
  requestAnimationFrame(loop);
  if (paused) return;

  const dt = Math.min(ts - lastFrame, 50);
  lastFrame = ts;

  // Mover fuente contaminante
  source.x += source.vx;
  if (source.x > W + 60) { source.x = -20; source.y = 200 + Math.random() * 80; }

  // Actualizar nodos
  for (const n of nodes) n.update(dt, nodes);

  // Heatmap
  updateHeatmap();

  // Estadísticas del gráfico
  const maxConc = Math.max(...nodes.map(n => n.reading), 0);
  updateChart(maxConc);

  // ── Dibujar ──────────────────────────────────────────────────────────────
  ctx.clearRect(0, 0, W, H);

  // Fondo
  ctx.fillStyle = '#0e1f0e';
  ctx.fillRect(0, 0, W, H);

  drawRiver();
  drawHeatmap();

  // Links entre nodos
  for (const n of nodes) n.drawLinks(nodes);

  // Paquetes
  updatePackets();
  drawPackets();

  // Nodos
  for (const n of nodes) n.draw();

  // Gateway
  drawGateway();

  // Indicador de fuente
  ctx.beginPath();
  ctx.arc(source.x, source.y, 7, 0, Math.PI * 2);
  ctx.fillStyle = 'rgba(255,60,60,0.7)';
  ctx.fill();
  ctx.fillStyle = '#ffaaaa';
  ctx.font = '8px monospace';
  ctx.textAlign = 'center';
  ctx.fillText('Hg²⁺', source.x, source.y - 10);

  // Gráfico histórico
  drawChart();

  updateStats();
}

// ─── Arranque ─────────────────────────────────────────────────────────────────
init();
requestAnimationFrame(loop);
