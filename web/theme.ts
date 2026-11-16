// Light, dark, or the device's: the page's theme, kept in this browser.
// index.html applies it before the first paint; the page keeps it in sync.

export type ThemeMode = "light" | "dark" | "system";
export const THEME_MODES: ThemeMode[] = ["light", "dark", "system"];
const KEY = "theme";

export function loadTheme(): ThemeMode {
  try {
    const saved = localStorage.getItem(KEY);
    if (saved === "light" || saved === "dark") return saved;
  } catch {}
  return "system";
}

const systemDark = () => matchMedia("(prefers-color-scheme: dark)").matches;

/** Applies mode to the page and keeps it in this browser. */
export function applyTheme(mode: ThemeMode): void {
  const dark = mode === "dark" || (mode === "system" && systemDark());
  document.documentElement.classList.toggle("dark", dark);
  document.documentElement.style.colorScheme = dark ? "dark" : "light";
  try {
    if (mode === "system") localStorage.removeItem(KEY);
    else localStorage.setItem(KEY, mode);
  } catch {}
}
