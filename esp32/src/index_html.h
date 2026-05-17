#pragma once
#include <pgmspace.h>

// HTML stránka pro / — embedded v PROGMEM, ať nezabírá RAM.
static const char INDEX_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="cs">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Holub</title>
<style>
  body { font-family: system-ui, sans-serif; margin: 0; padding: 1rem;
         background: #111; color: #eee; text-align: center; }
  h1   { font-size: 1.2rem; margin: .5rem 0; }
  img  { max-width: 100%; height: auto; border: 1px solid #333; border-radius: 8px; }
  .row { margin: .75rem 0; display: flex; gap: .5rem;
         justify-content: center; flex-wrap: wrap; align-items: center; }
  button { font-size: 1rem; padding: .6rem 1.4rem; border: 0; border-radius: 6px;
           background: #c33; color: #fff; cursor: pointer; }
  button:disabled { background: #555; cursor: not-allowed; }
  input[type=number] { width: 5rem; padding: .4rem; border-radius: 4px;
                       border: 1px solid #444; background: #222; color: #eee; }
  #log { font-family: monospace; font-size: .85rem; color: #888;
         margin-top: .5rem; min-height: 1.2em; }
</style>
</head>
<body>
  <h1>Holub</h1>
  <div id="cam-wrap">
    <img id="cam" src="/stream" alt="stream">
    <div id="cam-err" style="display:none;padding:2rem;color:#f66;border:1px solid #333;border-radius:8px">Kamera nedostupna</div>
  </div>
  <div class="row">
    <label>ms: <input type="number" id="ms" value="500" min="50" max="3000" step="50"></label>
    <button id="btn">Spray</button>
  </div>
  <div id="log"></div>
<script>
const btn = document.getElementById('btn');
const ms  = document.getElementById('ms');
const log = document.getElementById('log');
const cam = document.getElementById('cam');
const camErr = document.getElementById('cam-err');
cam.onerror = () => { cam.style.display='none'; camErr.style.display='block'; };
btn.addEventListener('click', async () => {
  btn.disabled = true;
  const v = Math.max(50, Math.min(3000, parseInt(ms.value) || 500));
  try {
    const r = await fetch('/spray', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: 'ms=' + v
    });
    log.textContent = r.status + ' ' + (await r.text());
  } catch (e) {
    log.textContent = 'chyba: ' + e;
  }
  // re-enable po cooldownu (2s) + rezerva
  setTimeout(() => { btn.disabled = false; }, 2200);
});
</script>
</body>
</html>
)HTML";
