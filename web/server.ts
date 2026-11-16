// Serves the page (Bun bundles index.html, its Svelte components and
// Tailwind: bunfig.toml) and the logs it stores:
//
//   GET    /api/sessions       a page of the stored sessions' summaries, the latest first:
//                              ?q= (a name, or a session's number), ?from= ?to= (ms since
//                              the epoch), ?page= (from 1), ?size= (up to 60)
//   POST   /api/sessions       the dash's raw log (application/octet-stream), as downloaded
//   GET    /api/sessions/:id   a session's name, dates and samples
//   PATCH  /api/sessions/:id   names it: {"name": "..."} (null or empty: no name)
//   DELETE /api/sessions/:id   forgets a session
//
// No authentication here: the https reverse proxy in front asks who's
// there (and Web Bluetooth only works over https). The logs go to
// DATA_DIR (./data by default; the Docker image's volume).
import { MAX_NAME, parseDataLog, RECORD, type SessionQuery } from "./datalog";
import index from "./index.html";
import { openStore, type Store } from "./store";

// The dash's whole log ring is under 1MB.
const MAX_LOG_BYTES = 4 * 1024 * 1024;

/** The :id of a request, a dash session number, or null. */
function sessionId(request: Request & { params: { id: string } }): number | null {
  const id = Number(request.params.id);
  return Number.isInteger(id) && id >= 0 && id <= 0xffff ? id : null;
}

/** The list's filters from a request's URL, bad numbers left out. */
function sessionQuery(request: Request): SessionQuery {
  const params = new URL(request.url).searchParams;
  const number = (name: string) => {
    const value = Number(params.get(name) ?? Number.NaN);
    return Number.isFinite(value) ? value : undefined;
  };
  return {
    q: params.get("q")?.slice(0, MAX_NAME) || undefined,
    from: number("from"),
    to: number("to"),
    page: Math.max(1, Math.floor(number("page") ?? 1)),
    size: Math.min(60, Math.max(1, Math.floor(number("size") ?? 12))),
  };
}

/** Names a session: a string up to MAX_NAME, trimmed; null or empty takes the name away. */
async function rename(store: Store, id: number, request: Request): Promise<Response> {
  let body: unknown;
  try {
    body = await request.json();
  } catch {
    return new Response("expected JSON", { status: 400 });
  }
  const name = (body as { name?: unknown } | null)?.name;
  if (name !== null && typeof name !== "string") return new Response("name: a string or null", { status: 400 });
  const trimmed = name?.trim() || null;
  if (trimmed && trimmed.length > MAX_NAME)
    return new Response(`name: ${MAX_NAME} characters at most`, { status: 400 });
  return store.rename(id, trimmed)
    ? Response.json({ name: trimmed })
    : new Response("no such session", { status: 404 });
}

async function upload(store: Store, request: Request): Promise<Response> {
  const bytes = new Uint8Array(await request.arrayBuffer());
  if (bytes.length === 0 || bytes.length % RECORD !== 0) {
    return new Response(`a dash log is whole ${RECORD}-byte records`, { status: 400 });
  }
  const samples = parseDataLog(bytes);
  return Response.json({ samples: samples.length, added: store.add(samples, Date.now()) });
}

/** Serves the page and the store in dataDir on port (0: any free one). */
export function serveLog(port: number, dataDir: string) {
  const store = openStore(dataDir);
  return Bun.serve({
    port,
    development: process.env.NODE_ENV !== "production",
    maxRequestBodySize: MAX_LOG_BYTES,
    routes: {
      "/": index,
      "/api/sessions": {
        GET: (request) => Response.json(store.sessions(sessionQuery(request))),
        POST: (request) => upload(store, request),
      },
      "/api/sessions/:id": {
        GET: (request) => {
          const id = sessionId(request);
          if (id === null) return new Response("bad session", { status: 400 });
          const session = store.session(id);
          return session ? Response.json(session) : new Response("no such session", { status: 404 });
        },
        PATCH: (request) => {
          const id = sessionId(request);
          return id === null ? new Response("bad session", { status: 400 }) : rename(store, id, request);
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
}

if (import.meta.main) {
  const server = serveLog(Number(process.env.PORT ?? 3000), process.env.DATA_DIR ?? "data");
  console.log(`GT86 dash log on ${server.url}`);
}
