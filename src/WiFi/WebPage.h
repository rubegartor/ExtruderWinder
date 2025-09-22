#pragma once

#include <Arduino.h>

#ifndef PROGMEM
#define PROGMEM
#endif

static const char WEB_PAGE[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>ExtruderWinder</title>
  <style>
    body { font:14px Arial; margin:0; padding:10px; background:#f5f5f5; }
    .container { max-width:1400px; margin:0 auto; padding:0 10px; }
    .header { text-align:center; margin-bottom:20px; }
    .info { background:#e3f2fd; padding:10px; margin:10px 0; border-radius:4px; text-align:center; }
    .grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(320px,1fr)); gap:15px; margin:10px 0; justify-content:center; }
    .card { background:#fff; border-radius:6px; padding:15px; box-shadow:0 2px 4px rgba(0,0,0,0.1); }
    .card h3 { margin:0 0 15px; color:#333; border-bottom:2px solid #2196f3; padding-bottom:8px; }
    .row { display:flex; justify-content:space-between; align-items:center; margin:8px 0; padding:8px; background:#f9f9f9; border-radius:3px; }
    .row.indent { margin-left:25px; }
    .label { font-weight:bold; color:#666; flex-shrink:0; }
    .value { color:#2e7d32; font-family:monospace; }
    .progress-container { flex:1; height:12px; background:#e0e0e0; border-radius:6px; margin:0 0 0 10px; }
    .progress-bar { height:100%; background:linear-gradient(90deg,#ff9800,#f57c00); border-radius:6px; transition:width 0.3s ease; }
    @media (max-width:768px) { .grid { grid-template-columns:1fr; } }
    @media (min-width:768px) and (max-width:1200px) { .grid { grid-template-columns:repeat(2,1fr); } }
    @media (min-width:1200px) { .grid { grid-template-columns:repeat(3,1fr); max-width:1080px; margin:10px auto; } }
    @media (min-width:1600px) { .grid { grid-template-columns:repeat(4,1fr); max-width:1400px; } }
  </style>
</head>
<body>
  <div class="container">
    <div class="header"><h1>🏭 ExtruderWinder</h1></div>
    <div class="info">🕒 Última actualización: <span id="time">--</span></div>
  </div>

  <div class="grid">
    <div class="card">
      <h3>📏 Medición</h3>
      <div class="row"><span class="label">📐 Actual:</span><span class="value" id="curr">--</span></div>
      <div class="row"><span class="label">📉 Min:</span><span class="value" id="min">--</span></div>
      <div class="row"><span class="label">📈 Max:</span><span class="value" id="max">--</span></div>
    </div>

    <div class="card">
      <h3>🧵 Filamento</h3>
      <div class="row"><span class="label">📊 Diámetro:</span><span class="value" id="diam">--</span></div>
      <div class="row"><span class="label">📏 Longitud:</span><span class="value" id="len">--</span></div>
      <div class="row"><span class="label">⚖️ Peso:</span><span class="value" id="weight">--</span></div>
      <div class="row"><span class="label">🏷️ Tipo:</span><span class="value" id="type">--</span></div>
    </div>

    <div class="card">
      <h3>⚙️ Motores</h3>
      <div class="row"><span class="label">🔧 Puller</span><span class="value"></span></div>
      <div class="row indent"><span class="label">⚡ Velocidad:</span><span class="value"><span id="pSpeed">--</span></span></div>
      <div class="row indent"><span class="label">🔄 Revoluciones:</span><span class="value" id="pRev">--</span></div>
      <div class="row"><span class="label">🎡 Spooler</span><span class="value"></span></div>
      <div class="row indent"><span class="label">⚡ Velocidad:</span><span class="value"><span id="sSpeed">--</span></span></div>
      <div class="row indent"><span class="label">🔄 Revoluciones:</span><span class="value" id="sRev">--</span></div>
    </div>

    <div class="card">
      <h3>Aligner</h3>
      <div class="row"><span class="label">📍 Posición:</span><div class="progress-container"><div class="progress-bar" id="alignProgress"></div></div></div>
      <div class="row"><span class="label">⚙️ Estado:</span><span class="value" id="alignState">--</span></div>
      <div class="row"><span class="label">🧵 Bobina:</span><span class="value" id="alignCal">--</span></div>
    </div>

    <div class="card">
      <h3>🔧 Tensioner</h3>
      <div class="row"><span class="label">📏 Distancia:</span><span class="value" id="tens">-- mm</span></div>
    </div>
  </div>

  <script>
    let sse = null;
    let reconnectAttempts = 0;
    let maxReconnectAttempts = 10;
    let plasticTypes = ['PLA', 'ABS', 'PETG', 'TPU', 'PVA'];
    let alignerStates = ['HOMING', 'MOVING_TO_HOME', 'SPOOL_CALIBRATION', 'AUTO_MOVE', 'IDLE', 'WAITING'];

    function updateFromSSE(data) {
      try {
        document.getElementById('time').textContent = new Date().toLocaleTimeString();

        if (data.measurement) {
          document.getElementById('curr').textContent = (data.measurement.current || 0).toFixed(2) + ' mm';
          document.getElementById('min').textContent = (data.measurement.min || 0).toFixed(2) + ' mm';
          document.getElementById('max').textContent = (data.measurement.max || 0).toFixed(2) + ' mm';
        }

        if (data.configuration) {
          document.getElementById('diam').textContent = (data.configuration.diameter || 0.00).toFixed(2) + ' mm';
          let plasticType = data.configuration.plasticType;
          if (plasticType >= 0 && plasticType < plasticTypes.length) {
            document.getElementById('type').textContent = plasticTypes[plasticType];
          } else {
            document.getElementById('type').textContent = 'PLA';
          }
        }

        if (data.production) {
          document.getElementById('len').textContent = (data.production.extrudedLength || 0).toFixed(1) + ' m';
          document.getElementById('weight').textContent = (data.production.extrudedWeight || 0).toFixed(1) + ' g';
        }

        if (data.motors) {
          if (data.motors.puller) {
            document.getElementById('pSpeed').textContent = Math.round(data.motors.puller.speed || 0);
            document.getElementById('pRev').textContent = Math.round(data.motors.puller.revolutions || 0);
          }
          if (data.motors.spooler) {
            document.getElementById('sSpeed').textContent = Math.round(data.motors.spooler.speed || 0);
            document.getElementById('sRev').textContent = Math.round(data.motors.spooler.revolutions || 0);
          }
          if (data.motors.aligner) {
            let position = data.motors.aligner.position || 0;
            let percentage = 0;
            if (data.system && data.system.calibrated && data.motors.aligner.startPos !== undefined && data.motors.aligner.endPos !== undefined) {
              let effectiveStart = data.motors.aligner.startPos - (data.motors.aligner.startExtension || 0);
              let effectiveEnd = data.motors.aligner.endPos + (data.motors.aligner.endExtension || 0);
              let range = effectiveEnd - effectiveStart;
              if (range > 0) {
                percentage = Math.max(0, Math.min(100, ((position - effectiveStart) / range) * 100));
              }
            } else {
              let maxPosition = data.motors.aligner.maxPosition || 0;
              percentage = Math.max(0, Math.min(100, (position / maxPosition) * 100));
            }
            document.getElementById('alignProgress').style.width = percentage.toFixed(1) + '%';
            let state = data.motors.aligner.state || 0;
            if (state >= 0 && state < alignerStates.length) {
              document.getElementById('alignState').textContent = alignerStates[state];
            } else {
              document.getElementById('alignState').textContent = 'Desconocido';
            }
          }
        }

        if (data.tensioner && data.tensioner.distance !== undefined) {
          document.getElementById('tens').textContent = Math.round(data.tensioner.distance) + ' mm';
        }

        if (data.system) {
          document.getElementById('alignCal').textContent = data.system.calibrated ? '✅ Calibrado' : '❌ No calibrado';
        }
      } catch (error) {
        console.error('Error actualizando UI:', error, data);
      }
    }

    function startSSE() {
      if (sse) { sse.close(); }
      console.log('Conectando a SSE... (intento ' + (reconnectAttempts + 1) + ')');
      sse = new EventSource('/events');
      sse.onopen = function() { console.log('✅ SSE conectado a 5Hz'); reconnectAttempts = 0; };
      sse.onmessage = function(event) {
        try { updateFromSSE(JSON.parse(event.data)); } catch (e) { console.error('Error parsing SSE data:', e, event.data); }
      };
      sse.onerror = function(error) {
        console.warn('SSE connection error:', error); sse.close();
        if (reconnectAttempts < maxReconnectAttempts) {
          reconnectAttempts++; let delay = Math.min(1000 * Math.pow(2, reconnectAttempts), 30000);
          console.log('Reconectando en ' + (delay/1000) + 's...'); setTimeout(startSSE, delay);
        } else { console.error('Máximo de intentos de reconexión alcanzado'); }
      };
    }

    window.onload = function() { console.log('🚀 Iniciando aplicación SSE 5Hz'); startSSE(); };
    window.onbeforeunload = function() { if (sse) sse.close(); };
  </script>
</body>
</html>)HTML";
