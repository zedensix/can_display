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
const NONE16 = -32768;

// A reading per column, null where the dash had none.
type Sample = {
  session: number;
  t: number;
  rpm: number | null;
  coolant: number | null;
  oil: number | null;
  intake: number | null;
  afr: number | null;
  battery: number | null;
  ethanol: number | null;
};

type Session = { id: number; samples: Sample[] };

function el<T extends HTMLElement = HTMLElement>(id: string): T {
  const found = document.getElementById(id);
  if (!found) throw new Error(`#${id} is missing from the page`);
  return found as T;
}

let samples: Sample[] = [];

const status = (text: string) => {
  el("status").textContent = text;
};

// parse turns the raw log into samples, oldest first.
export function parse(bytes: Uint8Array): Sample[] {
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const out: Sample[] = [];
  for (let at = 0; at + RECORD <= bytes.length; at += RECORD) {
    const session = view.getUint16(at, true);
    // Erased flash, or a lap record from a firmware that had a lap timer.
    if (session === 0xffff || view.getUint8(at + 2) !== SAMPLE) continue;
    const column = (i: number, scale: number) => {
      const v = view.getInt16(at + 8 + 2 * i, true);
      return v === NONE16 ? null : v / scale;
    };
    out.push({
      session,
      t: view.getUint32(at + 4, true),
      rpm: column(0, 1),
      coolant: column(1, 10),
      oil: column(2, 10),
      intake: column(3, 10),
      afr: column(4, 100),
      battery: column(5, 100),
      ethanol: column(6, 10),
    });
  }
  return out.sort((a, b) => a.session - b.session || a.t - b.t);
}

type Column = "rpm" | "coolant" | "oil" | "intake" | "afr" | "battery" | "ethanol";

// extremes is the lowest and highest reading of a column, null without any.
function extremes(of: Sample[], column: Column): [number, number] | null {
  const values = of.map((s) => s[column]).filter((v): v is number => v !== null);
  return values.length ? [Math.min(...values), Math.max(...values)] : null;
}

const minutes = (of: Sample[]) =>
  of.length ? Math.max(1, Math.round((of[of.length - 1].t - of[0].t) / 60000)) : 0;

function peak(label: string, value: string, hot = false) {
  return `
    <div class="flex items-baseline justify-between py-1.5">
      <span class="text-sm text-dim">${label}</span>
      <span class="font-display text-lg font-semibold tabular-nums ${hot ? "text-accent" : ""}">${value}</span>
    </div>`;
}

function sessionCard(session: Session): string {
  const top = (column: Column, unit: string, digits = 0) => {
    const range = extremes(session.samples, column);
    return range ? `${range[1].toFixed(digits)}${unit}` : "--";
  };
  const afr = extremes(session.samples, "afr");
  return `
    <article class="rounded-2xl border border-frame bg-panel p-4">
      <header class="flex items-baseline justify-between">
        <h2 class="font-display text-2xl font-bold tracking-wider text-accent">SESSION ${session.id}</h2>
        <span class="text-sm text-dim">${minutes(session.samples)} min</span>
      </header>
      <div class="mt-3 divide-y divide-frame">
        ${peak("Top RPM", top("rpm", ""))}
        ${peak("Hottest oil", top("oil", "°C"), (extremes(session.samples, "oil")?.[1] ?? 0) >= 120)}
        ${peak("Hottest coolant", top("coolant", "°C"))}
        ${peak("Hottest intake", top("intake", "°C"))}
        ${peak("AFR range", afr ? `${afr[0].toFixed(1)} to ${afr[1].toFixed(1)}` : "--")}
      </div>
    </article>`;
}

function show() {
  const sessions = new Map<number, Sample[]>();
  for (const s of samples) {
    const list = sessions.get(s.session) ?? [];
    list.push(s);
    sessions.set(s.session, list);
  }
  const oil = extremes(samples, "oil");
  const rpm = extremes(samples, "rpm");
  el("stat-sessions").textContent = String(sessions.size);
  el("stat-time").textContent = `${[...sessions.values()].reduce((sum, list) => sum + minutes(list), 0)} min`;
  el("stat-oil").textContent = oil ? `${oil[1].toFixed(0)}°C` : "--";
  el("stat-rpm").textContent = rpm ? rpm[1].toFixed(0) : "--";
  el("sessions").innerHTML = [...sessions]
    .reverse()
    .map(([id, list]) => sessionCard({ id, samples: list }))
    .join("");
  el<HTMLButtonElement>("csv").disabled = samples.length === 0;
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

const cell = (v: number | null) => (v === null ? "" : String(v));

el("csv").onclick = () =>
  save(
    "gt86-log.csv",
    "session,time_s,rpm,coolant_c,oil_c,intake_c,afr,battery_v,ethanol_pct",
    samples.map((s) =>
      [s.session, (s.t / 1000).toFixed(1), cell(s.rpm), cell(s.coolant), cell(s.oil), cell(s.intake), cell(s.afr),
        cell(s.battery), cell(s.ethanol)].join(","),
    ),
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
    samples = parse(await download());
    el("progress").classList.remove("sheen");
    status(`${samples.length ? `${samples.length} samples.` : "The log is empty."} The dash is back on its gauges.`);
    show();
  } catch (e) {
    status(`Failed: ${e instanceof Error ? e.message : String(e)}`);
  }
  connect.disabled = false;
};
