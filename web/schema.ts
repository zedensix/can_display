// The log store's tables (store.ts). A change here takes a migration:
// `bun run db:generate` writes it to web/drizzle/, the server applies it
// when it starts.
import { integer, primaryKey, real, sqliteTable } from "drizzle-orm/sqlite-core";

/** Each dash session the server got, and when it first did (the dash has no clock). */
export const sessions = sqliteTable("sessions", {
  id: integer("id").primaryKey(),
  receivedAt: integer("received_at").notNull(),
});

/** Every sample, keyed by its session and its time since that boot. */
export const samples = sqliteTable(
  "samples",
  {
    session: integer("session").notNull(),
    t: integer("t").notNull(),
    rpm: real("rpm"),
    coolant: real("coolant"),
    oil: real("oil"),
    intake: real("intake"),
    afr: real("afr"),
    battery: real("battery"),
    ethanol: real("ethanol"),
  },
  (table) => [primaryKey({ columns: [table.session, table.t] })],
);
