// Downloads the dash's data log over Web Bluetooth and shows it.
//
// The dash's GATT service is src/firmware/dash.gatt: write 'S' to the
// control characteristic, then the data characteristic notifies a 4-byte
// little-endian size and the log. The log is src/app/datalog.h's 32-byte
// little-endian records.

const SERVICE = "c0ff8600-4b1e-4c7a-9e2d-5a3f86d0a001";
const CONTROL = "c0ff8601-4b1e-4c7a-9e2d-5a3f86d0a001";
const DATA = "c0ff8602-4b1e-4c7a-9e2d-5a3f86d0a001";
const RECORD = 32;
const SAMPLE = 1;
const LAP = 2;
const NONE16 = -32768;
const NO_FIX = 0x7fffffff;

const $ = (id) => document.getElementById(id);
let samples = [];
let laps = [];

const status = (text) => {
  $("status").textContent = text;
};

const lapTime = (ms) => {
  const minutes = Math.floor(ms / 60000);
  const seconds = ((ms % 60000) / 1000).toFixed(2).padStart(5, "0");
  return `${minutes}:${seconds}`;
};

// parse turns the raw log into samples and laps, oldest first.
export function parse(bytes) {
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const out = { samples: [], laps: [] };
  for (let at = 0; at + RECORD <= bytes.length; at += RECORD) {
    const session = view.getUint16(at, true);
    if (session === 0xffff) continue;
    const kind = view.getUint8(at + 2);
    const lapCount = view.getUint8(at + 3);
    const t = view.getUint32(at + 4, true);
    if (kind === LAP) {
      out.laps.push({ session, t, lap: lapCount, ms: view.getUint32(at + 22, true) });
      continue;
    }
    if (kind !== SAMPLE) continue;
    const column = (i, scale) => {
      const v = view.getInt16(at + 8 + 2 * i, true);
      return v === NONE16 ? "" : String(v / scale);
    };
    const lat = view.getInt32(at + 22, true);
    const lon = view.getInt32(at + 26, true);
    const speed = view.getUint16(at + 30, true);
    out.samples.push({
      session,
      t,
      laps: lapCount,
      rpm: column(0, 1),
      coolant: column(1, 10),
      oil: column(2, 10),
      intake: column(3, 10),
      afr: column(4, 100),
      battery: column(5, 100),
      ethanol: column(6, 10),
      lat: lat === NO_FIX ? "" : (lat / 1e7).toFixed(7),
      lon: lat === NO_FIX ? "" : (lon / 1e7).toFixed(7),
      speed: speed === 0xffff ? "" : String(speed / 10),
    });
  }
  const order = (a, b) => a.session - b.session || a.t - b.t;
  out.samples.sort(order);
  out.laps.sort(order);
  return out;
}

function sessionCard(id, session) {
  const best = Math.min(...session.laps.map((l) => l.ms));
  const worst = Math.max(...session.laps.map((l) => l.ms));
  const rows = session.laps
    .map((l) => {
      const isBest = l.ms === best;
      const share = worst > best ? (l.ms - best) / (worst - best) : 0;
      const delta = isBest ? "BEST" : `+${((l.ms - best) / 1000).toFixed(2)}`;
      return `
        <div class="grid grid-cols-[2.5rem_1fr_7.5rem] items-center gap-3 py-1.5 ${isBest ? "text-good" : ""}">
          <span class="font-display text-dim">L${l.lap}</span>
          <div class="h-1.5 overflow-hidden rounded-full bg-frame">
            <div class="h-full rounded-full ${isBest ? "bg-good" : "bg-accent"}" style="width:${8 + 92 * share}%"></div>
          </div>
          <span class="text-right font-display text-lg font-semibold tabular-nums">${lapTime(l.ms)}
            <span class="ml-1 text-xs ${isBest ? "text-good" : "text-dim"}">${delta}</span></span>
        </div>`;
    })
    .join("");
  const minutes = Math.max(1, Math.round((session.to - session.from) / 60000));
  return `
    <article class="rounded-2xl border border-frame bg-panel p-4">
      <header class="flex items-baseline justify-between">
        <h2 class="font-display text-2xl font-bold tracking-wider text-accent">SESSION ${id}</h2>
        <span class="text-sm text-dim">${minutes} min, ${session.laps.length} laps</span>
      </header>
      ${rows ? `<div class="mt-3 divide-y divide-frame">${rows}</div>` : `<p class="mt-2 text-sm text-dim">No laps: no start line set, or no GPS.</p>`}
    </article>`;
}

function show() {
  const sessions = new Map();
  const entry = (id, t) => {
    const e = sessions.get(id) ?? { from: t, to: t, laps: [] };
    e.from = Math.min(e.from, t);
    e.to = Math.max(e.to, t);
    sessions.set(id, e);
    return e;
  };
  for (const s of samples) entry(s.session, s.t);
  for (const l of laps) entry(l.session, l.t).laps.push(l);

  const minutes = [...sessions.values()].reduce((sum, e) => sum + (e.to - e.from) / 60000, 0);
  $("stat-sessions").textContent = sessions.size;
  $("stat-laps").textContent = laps.length;
  $("stat-best").textContent = laps.length ? lapTime(Math.min(...laps.map((l) => l.ms))) : "--";
  $("stat-time").textContent = `${Math.round(minutes)} min`;
  $("sessions").innerHTML = [...sessions].reverse().map(([id, e]) => sessionCard(id, e)).join("");
  $("csv").disabled = samples.length === 0;
  $("laps").disabled = laps.length === 0;
  $("results").classList.remove("hidden");
}

function save(name, header, rows) {
  const blob = new Blob([`${header}\n${rows.join("\n")}\n`], { type: "text/csv" });
  const link = document.createElement("a");
  link.href = URL.createObjectURL(blob);
  link.download = name;
  link.click();
  URL.revokeObjectURL(link.href);
}

$("csv").onclick = () =>
  save(
    "gt86-log.csv",
    "session,time_s,laps,rpm,coolant_c,oil_c,intake_c,afr,battery_v,ethanol_pct,lat,lon,speed_kmh",
    samples.map((s) =>
      [s.session, (s.t / 1000).toFixed(1), s.laps, s.rpm, s.coolant, s.oil, s.intake, s.afr, s.battery,
        s.ethanol, s.lat, s.lon, s.speed].join(","),
    ),
  );

$("laps").onclick = () =>
  save("gt86-laps.csv", "session,lap,time_ms,time", laps.map((l) => [l.session, l.lap, l.ms, lapTime(l.ms)].join(",")));

// download connects, asks for the log and resolves with its bytes.
async function download() {
  const device = await navigator.bluetooth.requestDevice({ filters: [{ services: [SERVICE] }] });
  status("Connecting...");
  const server = await device.gatt.connect();
  const service = await server.getPrimaryService(SERVICE);
  const data = await service.getCharacteristic(DATA);
  const control = await service.getCharacteristic(CONTROL);
  let buffer = null;
  let got = 0;
  const started = performance.now();
  const bytes = new Promise((resolve, reject) => {
    device.addEventListener("gattserverdisconnected", () => reject(new Error("the dash disconnected")));
    data.addEventListener("characteristicvaluechanged", (event) => {
      const chunk = new Uint8Array(event.target.value.buffer);
      if (!buffer) {
        // The first notification is the size.
        buffer = new Uint8Array(new DataView(chunk.buffer).getUint32(0, true));
        $("transfer").classList.remove("hidden");
        if (buffer.length === 0) resolve(buffer);
        return;
      }
      buffer.set(chunk.subarray(0, buffer.length - got), got);
      got += chunk.length;
      const seconds = (performance.now() - started) / 1000;
      $("progress").style.width = `${Math.min(100, (100 * got) / buffer.length)}%`;
      $("received").textContent = `${Math.round(got / 1024)} / ${Math.round(buffer.length / 1024)} KB`;
      $("speed").textContent = `${(got / 1024 / seconds).toFixed(1)} KB/s`;
      if (got >= buffer.length) resolve(buffer);
    });
  });
  await data.startNotifications();
  status("Downloading...");
  await control.writeValueWithoutResponse(new Uint8Array([0x53])); // 'S': send
  const log = await bytes;
  device.gatt.disconnect();
  return log;
}

$("connect").onclick = async () => {
  if (!navigator.bluetooth) {
    status("No Web Bluetooth here: open this page in Chrome on Android.");
    return;
  }
  $("connect").disabled = true;
  try {
    ({ samples, laps } = parse(await download()));
    $("progress").classList.remove("sheen");
    status(samples.length || laps.length ? `${samples.length} samples, ${laps.length} laps.` : "The log is empty.");
    show();
  } catch (e) {
    status(`Failed: ${e.message}`);
  }
  $("connect").disabled = false;
};
