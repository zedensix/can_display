// The server's store of the dash's logs: every sample ever received, in
// SQLite through Drizzle (schema.ts; the migrations in drizzle/ apply when
// the store opens). A sample is keyed by its session and its time since
// that boot, so sending the dash's whole log again only adds what's new,
// and the sessions its flash ring overwrites stay here. A dash whose log
// flash was erased starts again at session 0: its sessions would mix
// with the stored ones of the same number.

import { Database } from "bun:sqlite";
import { mkdirSync } from "node:fs";
import { join } from "node:path";
import { and, desc, eq, gte, lte, or, type SQL, sql } from "drizzle-orm";
import { drizzle } from "drizzle-orm/bun-sqlite";
import { migrate } from "drizzle-orm/bun-sqlite/migrator";
import type { Sample, SessionPage, SessionQuery, SessionSummary, StoredSession } from "./datalog";
import { samples, sessions } from "./schema";

// Rows per INSERT: 10 columns each, under SQLite's 32766 variables.
const BATCH = 1000;

export type Store = ReturnType<typeof openStore>;

export function openStore(dir: string) {
  mkdirSync(dir, { recursive: true });
  const client = new Database(join(dir, "logs.sqlite"), { create: true });
  client.run("PRAGMA journal_mode = WAL");
  const db = drizzle(client);
  migrate(db, { migrationsFolder: join(import.meta.dir, "drizzle") });

  /** Adds samples received at now (ms since the epoch); returns how many were new. */
  function add(list: Sample[], now: number): number {
    return db.transaction((tx) => {
      const ids = [...new Set(list.map((s) => s.session))];
      if (ids.length) {
        tx.insert(sessions)
          .values(ids.map((id) => ({ id, receivedAt: now })))
          .onConflictDoNothing()
          .run();
      }
      let added = 0;
      for (let i = 0; i < list.length; i += BATCH) {
        const rows = list.slice(i, i + BATCH);
        // Only the rows inserted come back: the ones already stored are skipped.
        added += tx.insert(samples).values(rows).onConflictDoNothing().returning({ t: samples.t }).all().length;
      }
      return added;
    });
  }

  /** The filters' conditions on sessions. */
  function matching(query: SessionQuery): SQL | undefined {
    const q = query.q?.trim();
    return and(
      query.from === undefined ? undefined : gte(sessions.receivedAt, query.from),
      query.to === undefined ? undefined : lte(sessions.receivedAt, query.to),
      q
        ? or(
            // A name holding q, its LIKE wildcards taken literally; or the session's number.
            sql`${sessions.name} like ${`%${q.replace(/[%_\\]/g, "\\$&")}%`} escape '\\'`,
            sql`cast(${sessions.id} as text) = ${q}`,
          )
        : undefined,
    );
  }

  /** A page of the sessions matching query, the latest first. */
  function summaries(query: SessionQuery): SessionPage {
    const where = matching(query);
    const total = db.select({ n: sql<number>`count(*)` }).from(sessions).where(where).get()?.n ?? 0;
    const items: SessionSummary[] = db
      .select({
        id: sessions.id,
        name: sessions.name,
        receivedAt: sessions.receivedAt,
        samples: sql<number>`count(*)`,
        minutes: sql<number>`max(1, round((max(${samples.t}) - min(${samples.t})) / 60000.0))`,
        topRpm: sql<number | null>`max(${samples.rpm})`,
        peakOil: sql<number | null>`max(${samples.oil})`,
        peakCoolant: sql<number | null>`max(${samples.coolant})`,
        peakIntake: sql<number | null>`max(${samples.intake})`,
        afrMin: sql<number | null>`min(${samples.afr})`,
        afrMax: sql<number | null>`max(${samples.afr})`,
      })
      .from(sessions)
      .innerJoin(samples, eq(samples.session, sessions.id))
      .where(where)
      .groupBy(sessions.id)
      .orderBy(desc(sessions.id))
      .limit(query.size)
      .offset((query.page - 1) * query.size)
      .all();
    return { total, items };
  }

  /** A session's own fields and samples, or null. */
  function session(id: number): StoredSession | null {
    const row = db.select().from(sessions).where(eq(sessions.id, id)).get();
    if (!row) return null;
    const list = db.select().from(samples).where(eq(samples.session, id)).orderBy(samples.t).all();
    return { ...row, samples: list };
  }

  /** Names a session (null: no name); false when there's no such session. */
  function rename(id: number, name: string | null): boolean {
    return db.update(sessions).set({ name }).where(eq(sessions.id, id)).returning({ id: sessions.id }).all().length > 0;
  }

  function remove(id: number): boolean {
    return db.transaction((tx) => {
      tx.delete(samples).where(eq(samples.session, id)).run();
      return tx.delete(sessions).where(eq(sessions.id, id)).returning({ id: sessions.id }).all().length > 0;
    });
  }

  return { add, sessions: summaries, session, rename, remove, close: () => client.close() };
}
