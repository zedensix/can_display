// The bar's bottom, hollowed into an arc to sit on the steering column's
// shroud.

/**
 * Points along an arc through (0, 0), every mm from -half to half, sag
 * below it at both ends: the bar's bottom.
 */
export function arc(sag: number, half: number): [number, number][] {
  const r = (half * half + sag * sag) / (2 * sag);
  const n = Math.ceil(half);
  return Array.from({ length: 2 * n + 1 }, (_, i): [number, number] => {
    const x = Math.max(-half, Math.min(half, i - n));
    return [x, -(r - Math.sqrt(r * r - x * x))];
  });
}
