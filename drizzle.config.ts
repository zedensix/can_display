// drizzle-kit: `bun run db:generate` turns web/schema.ts changes into a
// migration in web/drizzle/.
import { defineConfig } from "drizzle-kit";

export default defineConfig({
  dialect: "sqlite",
  schema: "./web/schema.ts",
  out: "./web/drizzle",
});
