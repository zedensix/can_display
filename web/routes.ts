// The page's routes, in the URL's hash: #/ (download from the dash),
// #/logs (every stored session), #/logs/<id> (a session's charts).

export type Route = { page: "download" } | { page: "logs" } | { page: "session"; id: number };

export function routeFromHash(hash: string): Route {
  const match = /^#\/logs(?:\/(\d+))?$/.exec(hash);
  if (!match) return { page: "download" };
  return match[1] === undefined ? { page: "logs" } : { page: "session", id: Number(match[1]) };
}
