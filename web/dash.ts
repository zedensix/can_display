// Downloads the dash's data log over Web Bluetooth.
//
// The dash's GATT service is pico/src/firmware/ble/dash.gatt: write 'S' to the
// control characteristic, then the data characteristic notifies a 4-byte
// little-endian size and the log; 'D' once it's all in tells the dash,
// which goes back to its gauges.

const SERVICE = "c0ff8600-4b1e-4c7a-9e2d-5a3f86d0a001";
const CONTROL = "c0ff8601-4b1e-4c7a-9e2d-5a3f86d0a001";
const DATA = "c0ff8602-4b1e-4c7a-9e2d-5a3f86d0a001";

export type Progress = { got: number; total: number; seconds: number };

/**
 * Connects to the dash, asks for the log and resolves with its bytes.
 * onStatus says what it's doing, onProgress how far the transfer is.
 */
export async function downloadLogFromDash(
  onStatus: (text: string) => void,
  onProgress: (progress: Progress) => void,
): Promise<Uint8Array<ArrayBuffer>> {
  const device = await navigator.bluetooth.requestDevice({ filters: [{ services: [SERVICE] }] });
  if (!device.gatt) throw new Error("the dash has no GATT server");
  onStatus("Connecting...");
  const server = await device.gatt.connect();
  const service = await server.getPrimaryService(SERVICE);
  const data = await service.getCharacteristic(DATA);
  const control = await service.getCharacteristic(CONTROL);
  let buffer: Uint8Array<ArrayBuffer> | null = null;
  let got = 0;
  const started = performance.now();
  const bytes = new Promise<Uint8Array<ArrayBuffer>>((resolve, reject) => {
    device.addEventListener("gattserverdisconnected", () => reject(new Error("the dash disconnected")));
    data.addEventListener("characteristicvaluechanged", (event) => {
      const value = (event.target as BluetoothRemoteGATTCharacteristic).value;
      if (!value) return;
      if (!buffer) {
        // The first notification is the size.
        buffer = new Uint8Array(value.getUint32(0, true));
        onProgress({ got: 0, total: buffer.length, seconds: 0 });
        if (buffer.length === 0) resolve(buffer);
        return;
      }
      const chunk = new Uint8Array(value.buffer, value.byteOffset, value.byteLength);
      buffer.set(chunk.subarray(0, buffer.length - got), got);
      got += chunk.length;
      onProgress({ got, total: buffer.length, seconds: (performance.now() - started) / 1000 });
      if (got >= buffer.length) resolve(buffer);
    });
  });
  await data.startNotifications();
  onStatus("Downloading...");
  await control.writeValueWithoutResponse(new Uint8Array([0x53])); // 'S': send
  const received = await bytes;
  await control.writeValueWithoutResponse(new Uint8Array([0x44])); // 'D': downloaded
  device.gatt.disconnect();
  return received;
}
