// Serves the page (Bun bundles index.html, its Svelte components and
// Tailwind: bunfig.toml) and the logs it stores:
//
//   GET    /api/sessions       every stored session's summary, the latest first
//   POST   /api/sessions       the dash's raw log (application/octet-stream), as downloaded
//   GET    /api/sessions/:id   a session's samples
//   DELETE /api/sessions/:id   forgets a session
//
// No authentication here: the https reverse proxy in front asks who's
// there (and Web Bluetooth only works over https). The logs go to
// DATA_DIR (./data by default; the Docker image's volume).
import { parseDataLog, RECORD } from "./datalog";
import index from "./index.html";
import { openStore } from "./store";

// The dash's whole log ring is under 1MB.
const MAX_LOG_BYTES = 4 * 1024 * 1024;

const store = openStore(process.env.DATA_DIR ?? "data");

/** The :id of a request, a dash session number, or null. */
function sessionId(request: Request & { params: { id: string } }): number | null {
  const id = Number(request.params.id);
  return Number.isInteger(id) && id >= 0 && id <= 0xffff ? id : null;
}

async function upload(request: Request): Promise<Response> {
  const bytes = new Uint8Array(await request.arrayBuffer());
  if (bytes.length === 0 || bytes.length % RECORD !== 0) {
    return new Response(`a dash log is whole ${RECORD}-byte records`, { status: 400 });
  }
  const samples = parseDataLog(bytes);
  return Response.json({ samples: samples.length, added: store.add(samples, Date.now()) });
}

const server = Bun.serve({
  port: Number(process.env.PORT ?? 3000),
  development: process.env.NODE_ENV !== "production",
  maxRequestBodySize: MAX_LOG_BYTES,
  routes: {
    "/": index,
    "/api/sessions": { GET: () => Response.json(store.sessions()), POST: upload },
    "/api/sessions/:id": {
      GET: (request) => {
        const id = sessionId(request);
        return id === null ? new Response("bad session", { status: 400 }) : Response.json(store.samples(id));
      },
      DELETE: (request) => {
        const id = sessionId(request);
        if (id === null) return new Response("bad session", { status: 400 });
        return store.remove(id)
          ? new Response(null, { status: 204 })
          : new Response("no such session", { status: 404 });
      },
    },
  },
});

console.log(`GT86 dash log on ${server.url}`);
