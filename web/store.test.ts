// The log store against a log built as the dash writes it (datalog.h).
import { afterEach, expect, test } from "bun:test";
import { mkdtempSync, rmSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { parseDataLog, RECORD } from "./datalog";
import { openStore, type Store } from "./store";

/** A log of n samples per session, 200ms apart: RPM 1000 + 10 per sample, oil 90.5°C, the rest none. */
function dashLog(sessions: number[], n: number): Uint8Array {
  const bytes = new Uint8Array(sessions.length * n * RECORD).fill(0xff);
  const view = new DataView(bytes.buffer);
  let at = 0;
  for (const session of sessions) {
    for (let i = 0; i < n; i++, at += RECORD) {
      view.setUint16(at, session, true);
      view.setUint8(at + 2, 1);
      view.setUint32(at + 4, i * 200, true);
      for (let column = 0; column < 7; column++) view.setInt16(at + 8 + 2 * column, -32768, true);
      view.setInt16(at + 8, 1000 + 10 * i, true);
      view.setInt16(at + 8 + 2 * 2, 905, true);
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

test("stores each sample once, summarizes and forgets sessions", () => {
  dir = mkdtempSync(join(tmpdir(), "dash-log-"));
  store = openStore(dir);
  const samples = parseDataLog(dashLog([3, 4], 600));
  expect(samples).toHaveLength(1200);
  expect(store.add(samples, 1000)).toBe(1200);
  // The same log again, as the dash sends it whole: nothing new.
  expect(store.add(samples, 2000)).toBe(0);

  const [latest, first] = store.sessions();
  expect(latest).toMatchObject({ id: 4, receivedAt: 1000, samples: 600, minutes: 2, topRpm: 6990, peakOil: 90.5 });
  expect(latest.afrMin).toBeNull();
  expect(first.id).toBe(3);
  expect(store.samples(3)[1]).toMatchObject({ session: 3, t: 200, rpm: 1010, oil: 90.5, afr: null });

  expect(store.remove(3)).toBe(true);
  expect(store.remove(3)).toBe(false);
  expect(store.sessions().map((s) => s.id)).toEqual([4]);
});
