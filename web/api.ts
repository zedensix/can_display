// The server's log store (server.ts), from the page.
import type { SessionPage, SessionQuery, StoredSession } from "./datalog";

async function ok(response: Response): Promise<Response> {
  if (!response.ok) throw new Error((await response.text()) || `HTTP ${response.status}`);
  return response;
}

/** A page of the stored sessions matching query. */
export async function listSessions(query: SessionQuery): Promise<SessionPage> {
  const params = new URLSearchParams({ page: String(query.page), size: String(query.size) });
  if (query.q) params.set("q", query.q);
  if (query.from !== undefined) params.set("from", String(query.from));
  if (query.to !== undefined) params.set("to", String(query.to));
  return (await ok(await fetch(`/api/sessions?${params}`))).json();
}

export const getSession = async (id: number): Promise<StoredSession> =>
  (await ok(await fetch(`/api/sessions/${id}`))).json();

/** Names a session; null or empty takes the name away. Resolves with the name kept. */
export async function renameSession(id: number, name: string | null): Promise<string | null> {
  const response = await fetch(`/api/sessions/${id}`, {
    method: "PATCH",
    headers: { "content-type": "application/json" },
    body: JSON.stringify({ name }),
  });
  return (await (await ok(response)).json()).name;
}

/** Sends the dash's raw log; resolves with how many samples were new. */
export async function uploadLog(bytes: Uint8Array<ArrayBuffer>): Promise<{ samples: number; added: number }> {
  const response = await fetch("/api/sessions", {
    method: "POST",
    headers: { "content-type": "application/octet-stream" },
    body: bytes,
  });
  return (await ok(response)).json();
}

export async function deleteSession(id: number): Promise<void> {
  await ok(await fetch(`/api/sessions/${id}`, { method: "DELETE" }));
}
