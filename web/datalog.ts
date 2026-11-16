// The dash's data log: src/app/store/datalog.h's 32-byte little-endian records,
// parsed into samples, and what the page shows of them. This parser must
// follow datalog.h's record layout.

export const RECORD = 32;
const SAMPLE = 1;
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
};

export type Column = "rpm" | "coolant" | "oil" | "intake" | "afr" | "battery" | "ethanol";
export const COLUMNS: Column[] = ["rpm", "coolant", "oil", "intake", "afr", "battery", "ethanol"];

export type Session = { id: number; samples: Sample[] };

/** The raw log as samples, oldest first. */
export function parseDataLog(bytes: Uint8Array): Sample[] {
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const out: Sample[] = [];
  for (let at = 0; at + RECORD <= bytes.length; at += RECORD) {
    const session = view.getUint16(at, true);
    // Erased flash, or a lap record from a firmware that had a lap timer.
    if (session === 0xffff || view.getUint8(at + 2) !== SAMPLE) continue;
    const readColumn = (i: number, scale: number) => {
      const v = view.getInt16(at + 8 + 2 * i, true);
      return v === NONE16 ? null : v / scale;
    };
    out.push({
      session,
      t: view.getUint32(at + 4, true),
      rpm: readColumn(0, 1),
      coolant: readColumn(1, 10),
      oil: readColumn(2, 10),
      intake: readColumn(3, 10),
      afr: readColumn(4, 100),
      battery: readColumn(5, 100),
      ethanol: readColumn(6, 10),
    });
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

const csvCell = (v: number | null) => (v === null ? "" : String(v));

/** The samples as a CSV file's text. */
export function toCsv(samples: Sample[]): string {
  const rows = samples.map((s) =>
    [
      s.session,
      (s.t / 1000).toFixed(1),
      csvCell(s.rpm),
      csvCell(s.coolant),
      csvCell(s.oil),
      csvCell(s.intake),
      csvCell(s.afr),
      csvCell(s.battery),
      csvCell(s.ethanol),
    ].join(","),
  );
  return `session,time_s,rpm,coolant_c,oil_c,intake_c,afr,battery_v,ethanol_pct\n${rows.join("\n")}\n`;
}

// A session's peaks, what the list of sessions shows. receivedAt: when
// the server first got it (the dash has no clock), null for a log just
// downloaded.
export type SessionSummary = {
  id: number;
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
