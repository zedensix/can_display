// Downloads the dash's data log over Web Bluetooth and shows it.
//
// The dash's GATT service is src/firmware/dash.gatt: write 'S' to the
// control characteristic, then the data characteristic notifies a 4-byte
// little-endian size and the log; 'D' once it's all in tells the dash,
// which goes back to its gauges. The log is src/app/datalog.h's 32-byte
// little-endian records.

const SERVICE = "c0ff8600-4b1e-4c7a-9e2d-5a3f86d0a001";
const CONTROL = "c0ff8601-4b1e-4c7a-9e2d-5a3f86d0a001";
const DATA = "c0ff8602-4b1e-4c7a-9e2d-5a3f86d0a001";
const RECORD = 32;
const SAMPLE = 1;
const LAP = 2;
const NONE16 = -32768;
const NO_FIX = 0x7fffffff;

// Values as the CSV prints them: "" for no reading.
type Sample = {
  session: number;
  t: number;
  laps: number;
  rpm: string;
  coolant: string;
  oil: string;
  intake: string;
  afr: string;
  battery: string;
  ethanol: string;
  lat: string;
  lon: string;
  speed: string;
};

type Lap = { session: number; t: number; lap: number; ms: number };

type Session = { from: number; to: number; laps: Lap[] };

type Log = { samples: Sample[]; laps: Lap[] };

function el<T extends HTMLElement = HTMLElement>(id: string): T {
  const found = document.getElementById(id);
  if (!found) throw new Error(`#${id} is missing from the page`);
  return found as T;
}

let log: Log = { samples: [], laps: [] };

const status = (text: string) => {
  el("status").textContent = text;
};

const lapTime = (ms: number) => {
  const minutes = Math.floor(ms / 60000);
  const seconds = ((ms % 60000) / 1000).toFixed(2).padStart(5, "0");
  return `${minutes}:${seconds}`;
};

// parse turns the raw log into samples and laps, oldest first.
export function parse(bytes: Uint8Array): Log {
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const out: Log = { samples: [], laps: [] };
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
    const column = (i: number, scale: number) => {
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
  const order = (a: { session: number; t: number }, b: { session: number; t: number }) =>
    a.session - b.session || a.t - b.t;
  out.samples.sort(order);
  out.laps.sort(order);
  return out;
}

function sessionCard(id: number, session: Session): string {
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
  const sessions = new Map<number, Session>();
  const entry = (id: number, t: number) => {
    const e = sessions.get(id) ?? { from: t, to: t, laps: [] };
    e.from = Math.min(e.from, t);
    e.to = Math.max(e.to, t);
    sessions.set(id, e);
    return e;
  };
  for (const s of log.samples) entry(s.session, s.t);
  for (const l of log.laps) entry(l.session, l.t).laps.push(l);

  const minutes = [...sessions.values()].reduce((sum, e) => sum + (e.to - e.from) / 60000, 0);
  el("stat-sessions").textContent = String(sessions.size);
  el("stat-laps").textContent = String(log.laps.length);
  el("stat-best").textContent = log.laps.length ? lapTime(Math.min(...log.laps.map((l) => l.ms))) : "--";
  el("stat-time").textContent = `${Math.round(minutes)} min`;
  el("sessions").innerHTML = [...sessions]
    .reverse()
    .map(([id, e]) => sessionCard(id, e))
    .join("");
  el<HTMLButtonElement>("csv").disabled = log.samples.length === 0;
  el<HTMLButtonElement>("laps").disabled = log.laps.length === 0;
  el("results").classList.remove("hidden");
}

function save(name: string, header: string, rows: string[]) {
  const blob = new Blob([`${header}\n${rows.join("\n")}\n`], { type: "text/csv" });
  const link = document.createElement("a");
  link.href = URL.createObjectURL(blob);
  link.download = name;
  link.click();
  URL.revokeObjectURL(link.href);
}

el("csv").onclick = () =>
  save(
    "gt86-log.csv",
    "session,time_s,laps,rpm,coolant_c,oil_c,intake_c,afr,battery_v,ethanol_pct,lat,lon,speed_kmh",
    log.samples.map((s) =>
      [s.session, (s.t / 1000).toFixed(1), s.laps, s.rpm, s.coolant, s.oil, s.intake, s.afr, s.battery,
        s.ethanol, s.lat, s.lon, s.speed].join(","),
    ),
  );

el("laps").onclick = () =>
  save(
    "gt86-laps.csv",
    "session,lap,time_ms,time",
    log.laps.map((l) => [l.session, l.lap, l.ms, lapTime(l.ms)].join(",")),
  );

// download connects, asks for the log and resolves with its bytes.
async function download(): Promise<Uint8Array> {
  const device = await navigator.bluetooth.requestDevice({ filters: [{ services: [SERVICE] }] });
  if (!device.gatt) throw new Error("the dash has no GATT server");
  status("Connecting...");
  const server = await device.gatt.connect();
  const service = await server.getPrimaryService(SERVICE);
  const data = await service.getCharacteristic(DATA);
  const control = await service.getCharacteristic(CONTROL);
  let buffer: Uint8Array | null = null;
  let got = 0;
  const started = performance.now();
  const bytes = new Promise<Uint8Array>((resolve, reject) => {
    device.addEventListener("gattserverdisconnected", () => reject(new Error("the dash disconnected")));
    data.addEventListener("characteristicvaluechanged", (event) => {
      const value = (event.target as BluetoothRemoteGATTCharacteristic).value;
      if (!value) return;
      const chunk = new Uint8Array(value.buffer, value.byteOffset, value.byteLength);
      if (!buffer) {
        // The first notification is the size.
        buffer = new Uint8Array(value.getUint32(0, true));
        el("transfer").classList.remove("hidden");
        if (buffer.length === 0) resolve(buffer);
        return;
      }
      buffer.set(chunk.subarray(0, buffer.length - got), got);
      got += chunk.length;
      const seconds = (performance.now() - started) / 1000;
      el("progress").style.width = `${Math.min(100, (100 * got) / buffer.length)}%`;
      el("received").textContent = `${Math.round(got / 1024)} / ${Math.round(buffer.length / 1024)} KB`;
      el("speed").textContent = `${(got / 1024 / seconds).toFixed(1)} KB/s`;
      if (got >= buffer.length) resolve(buffer);
    });
  });
  await data.startNotifications();
  status("Downloading...");
  await control.writeValueWithoutResponse(new Uint8Array([0x53])); // 'S': send
  const received = await bytes;
  await control.writeValueWithoutResponse(new Uint8Array([0x44])); // 'D': downloaded
  device.gatt.disconnect();
  return received;
}

const connect = el<HTMLButtonElement>("connect");
connect.onclick = async () => {
  if (!navigator.bluetooth) {
    status("No Web Bluetooth here: open this page in Chrome on Android.");
    return;
  }
  connect.disabled = true;
  try {
    log = parse(await download());
    el("progress").classList.remove("sheen");
    const counts = log.samples.length || log.laps.length;
    status(
      `${counts ? `${log.samples.length} samples, ${log.laps.length} laps.` : "The log is empty."} ` +
        "The dash is back on its gauges.",
    );
    show();
  } catch (e) {
    status(`Failed: ${e instanceof Error ? e.message : String(e)}`);
  }
  connect.disabled = false;
};
