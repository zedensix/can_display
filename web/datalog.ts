// The dash's data log: pico/src/app/store/datalog.h's little-endian records,
// parsed into samples, and what the page shows of them. This parser must
// follow datalog.h's record layout: 64-byte records (kind 2), and the
// 32-byte ones an older firmware wrote (kind 1), which a log may still
// hold before them. A log is whole 32-byte slots.

export const SLOT = 32;
const RECORD = 64;
const SAMPLE = 2;
const SAMPLE_V1 = 1;
const NONE16 = -32768;

// A reading per column, null where the dash had none.
export type Sample = {
  session: number;
  t: number;
  rpm: number | null;
  coolant: number | null;
  oil: number | null;
  intake: number | null;
  afr: number | null;
  battery: number | null;
  ethanol: number | null;
  throttle: number | null;
  pedal: number | null;
  load: number | null;
  timing: number | null;
  stft: number | null;
  ltft: number | null;
  afrTarget: number | null;
  rail: number | null;
  speed: number | null;
  ambient: number | null;
};

export type Column = Exclude<keyof Sample, "session" | "t">;

// Each column in the record's order, its scale and its CSV header.
const LAYOUT: [Column, number, string][] = [
  ["rpm", 1, "rpm"],
  ["coolant", 10, "coolant_c"],
  ["oil", 10, "oil_c"],
  ["intake", 10, "intake_c"],
  ["afr", 100, "afr"],
  ["battery", 100, "battery_v"],
  ["ethanol", 10, "ethanol_pct"],
  ["throttle", 10, "throttle_pct"],
  ["pedal", 10, "pedal_pct"],
  ["load", 10, "load_pct"],
  ["timing", 10, "timing_deg"],
  ["stft", 10, "stft_pct"],
  ["ltft", 10, "ltft_pct"],
  ["afrTarget", 100, "afr_target"],
  ["rail", 100, "rail_mpa"],
  ["speed", 1, "speed_kmh"],
  ["ambient", 10, "outside_c"],
];
export const COLUMNS: Column[] = LAYOUT.map(([column]) => column);

export type Session = { id: number; samples: Sample[] };

/** The raw log as samples, oldest first. */
export function parseDataLog(bytes: Uint8Array): Sample[] {
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const out: Sample[] = [];
  for (let at = 0; at + SLOT <= bytes.length; ) {
    const session = view.getUint16(at, true);
    const kind = view.getUint8(at + 2);
    const columns = session !== 0xffff && kind === SAMPLE && at + RECORD <= bytes.length ? LAYOUT.length : 0;
    // Erased flash, an old record, or a lap record from a firmware that had a lap timer.
    const v1 = session !== 0xffff && kind === SAMPLE_V1;
    if (columns || v1) {
      const sample = { session, t: view.getUint32(at + 4, true) } as Sample;
      LAYOUT.forEach(([column, scale], i) => {
        const v = i < (columns || 8) ? view.getInt16(at + 8 + 2 * i, true) : NONE16;
        // An old record holds 0xFFFF (-1) for the throttle from before it was logged.
        sample[column] = v === NONE16 || (v1 && column === "throttle" && v === -1) ? null : v / scale;
      });
      out.push(sample);
    }
    at += columns ? RECORD : SLOT;
  }
  return out.sort((a, b) => a.session - b.session || a.t - b.t);
}

/** The samples grouped by session, the latest first. */
export function groupSessions(samples: Sample[]): Session[] {
  const sessions = new Map<number, Sample[]>();
  for (const s of samples) {
    const list = sessions.get(s.session) ?? [];
    list.push(s);
    sessions.set(s.session, list);
  }
  return [...sessions].reverse().map(([id, list]) => ({ id, samples: list }));
}

/** The lowest and highest reading of a column, null without any. */
export function lowestAndHighest(of: Sample[], column: Column): [number, number] | null {
  const values = of.map((s) => s[column]).filter((v): v is number => v !== null);
  return values.length ? [Math.min(...values), Math.max(...values)] : null;
}

export const minutesDriven = (of: Sample[]) =>
  of.length ? Math.max(1, Math.round((of[of.length - 1].t - of[0].t) / 60000)) : 0;

/** The highest reading of a column with its unit, "--" without any. */
export function highest(of: Sample[], column: Column, unit = "", digits = 0): string {
  const range = lowestAndHighest(of, column);
  return range ? `${range[1].toFixed(digits)}${unit}` : "--";
}

/** The samples as a CSV file's text. */
export function toCsv(samples: Sample[]): string {
  const rows = samples.map((s) =>
    [s.session, (s.t / 1000).toFixed(1), ...COLUMNS.map((column) => s[column] ?? "")].join(","),
  );
  const header = ["session", "time_s", ...LAYOUT.map(([, , name]) => name)].join(",");
  return `${header}\n${rows.join("\n")}\n`;
}

// A session's peaks, what the list of sessions shows. receivedAt: when
// the server first got it (the dash has no clock), null for a log just
// downloaded.
export type SessionSummary = {
  id: number;
  name: string | null;
  receivedAt: number | null;
  samples: number;
  minutes: number;
  topRpm: number | null;
  peakOil: number | null;
  peakCoolant: number | null;
  peakIntake: number | null;
  afrMin: number | null;
  afrMax: number | null;
};

/** A session's summary, from its samples. */
export function summarize(session: Session, receivedAt: number | null = null): SessionSummary {
  const peak = (column: Column) => lowestAndHighest(session.samples, column)?.[1] ?? null;
  const afr = lowestAndHighest(session.samples, "afr");
  return {
    id: session.id,
    name: null,
    receivedAt,
    samples: session.samples.length,
    minutes: minutesDriven(session.samples),
    topRpm: peak("rpm"),
    peakOil: peak("oil"),
    peakCoolant: peak("coolant"),
    peakIntake: peak("intake"),
    afrMin: afr?.[0] ?? null,
    afrMax: afr?.[1] ?? null,
  };
}

/** A page of stored sessions, and how many match the filters in all. */
export type SessionPage = { total: number; items: SessionSummary[] };

/** The list's filters: a search (a session's name, or its number), a time range (ms since the epoch), a page from 1. */
export type SessionQuery = { q?: string; from?: number; to?: number; page: number; size: number };

/** A stored session's own fields, and its samples. */
export type StoredSession = { id: number; name: string | null; receivedAt: number; samples: Sample[] };

export const MAX_NAME = 80;
