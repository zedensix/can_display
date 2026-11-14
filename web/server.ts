// Serves the page: Bun bundles index.html, its script and Tailwind (see
// bunfig.toml) on the first request. TLS is the reverse proxy's job; Web
// Bluetooth only works over https.
import index from "./index.html";

const server = Bun.serve({
  port: Number(process.env.PORT ?? 3000),
  development: process.env.NODE_ENV !== "production",
  routes: { "/": index },
});

console.log(`GT86 dash log on ${server.url}`);
