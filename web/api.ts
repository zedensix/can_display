// The server's log store (server.ts), from the page.
import type { Sample, SessionSummary } from "./datalog";

async function ok(response: Response): Promise<Response> {
  if (!response.ok) throw new Error((await response.text()) || `HTTP ${response.status}`);
  return response;
}

export const listSessions = async (): Promise<SessionSummary[]> => (await ok(await fetch("/api/sessions"))).json();

export const sessionSamples = async (id: number): Promise<Sample[]> =>
  (await ok(await fetch(`/api/sessions/${id}`))).json();

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
