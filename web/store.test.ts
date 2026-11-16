// The log store against a log built as the dash writes it (datalog.h).
import { afterEach, expect, test } from "bun:test";
import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { parseDataLog } from "./datalog";
import { openStore, type Store } from "./store";

/**
 * A log of n samples per session, 200ms apart: RPM 1000 + 10 per sample,
 * oil 90.5°C, the rest none. v1: the 32-byte records of an older firmware
 * (its throttle 0xFFFF), else the 64-byte ones with a rail pressure of 18.5 MPa.
 */
function dashLog(sessions: number[], n: number, v1 = false): Uint8Array {
  const size = v1 ? 32 : 64;
  const bytes = new Uint8Array(sessions.length * n * size).fill(0xff);
  const view = new DataView(bytes.buffer);
  let at = 0;
  for (const session of sessions) {
    for (let i = 0; i < n; i++, at += size) {
      view.setUint16(at, session, true);
      view.setUint8(at + 2, v1 ? 1 : 2);
      view.setUint32(at + 4, i * 200, true);
      for (let column = 0; column < (v1 ? 7 : 17); column++) view.setInt16(at + 8 + 2 * column, -32768, true);
      view.setInt16(at + 8, 1000 + 10 * i, true);
      view.setInt16(at + 8 + 2 * 2, 905, true);
      if (!v1) view.setInt16(at + 8 + 2 * 14, 1850, true);
    }
  }
  return bytes;
}

let dir = "";
let store: Store;
afterEach(() => {
  store.close();
  rmSync(dir, { recursive: true });
});

const ALL = { page: 1, size: 60 };

test("stores each sample once, summarizes and forgets sessions", () => {
  dir = mkdtempSync(join(tmpdir(), "dash-log-"));
  store = openStore(dir);
  // An older firmware's records, then the current ones: a ring the update ran through.
  const old = dashLog([3], 600, true);
  const log = new Uint8Array([...old, ...dashLog([4], 600)]);
  const samples = parseDataLog(log);
  expect(samples).toHaveLength(1200);
  expect(store.add(samples, 1000)).toBe(1200);
  // The same log again, as the dash sends it whole: nothing new.
  expect(store.add(samples, 2000)).toBe(0);

  const { total, items } = store.sessions(ALL);
  expect(total).toBe(2);
  const [latest, first] = items;
  expect(latest).toMatchObject({ id: 4, receivedAt: 1000, samples: 600, minutes: 2, topRpm: 6990, peakOil: 90.5 });
  expect(latest.afrMin).toBeNull();
  expect(first.id).toBe(3);
  // No throttle in these records: 0xFFFF, as a firmware from before it wrote them.
  expect(store.session(3)?.samples[1]).toMatchObject({
    session: 3,
    t: 200,
    rpm: 1010,
    oil: 90.5,
    afr: null,
    throttle: null,
    rail: null,
  });
  expect(store.session(4)?.samples[1]).toMatchObject({ rpm: 1010, rail: 18.5, pedal: null });

  expect(store.remove(3)).toBe(true);
  expect(store.remove(3)).toBe(false);
  expect(store.sessions(ALL).items.map((s) => s.id)).toEqual([4]);
});

test("pages, filters by date and searches the sessions, names them", () => {
  dir = mkdtempSync(join(tmpdir(), "dash-log-"));
  store = openStore(dir);
  // Sessions 1 to 5, received a day apart.
  for (let session = 1; session <= 5; session++) store.add(parseDataLog(dashLog([session], 10)), session * 86400000);

  const page = store.sessions({ page: 2, size: 2 });
  expect(page.total).toBe(5);
  expect(page.items.map((s) => s.id)).toEqual([3, 2]);

  const days = store.sessions({ ...ALL, from: 2 * 86400000, to: 4 * 86400000 });
  expect(days.items.map((s) => s.id)).toEqual([4, 3, 2]);

  expect(store.rename(2, "Track day, 100%_wet")).toBe(true);
  expect(store.rename(9, "nothing")).toBe(false);
  expect(store.session(2)?.name).toBe("Track day, 100%_wet");
  expect(store.sessions({ ...ALL, q: "track" }).items.map((s) => s.id)).toEqual([2]);
  // LIKE's wildcards in a search are taken literally.
  expect(store.sessions({ ...ALL, q: "100%_w" }).total).toBe(1);
  expect(store.sessions({ ...ALL, q: "0%w" }).total).toBe(0);
  expect(store.sessions({ ...ALL, q: "5" }).items.map((s) => s.id)).toEqual([5]);
  expect(store.rename(2, null)).toBe(true);
  expect(store.session(2)?.name).toBeNull();
});
