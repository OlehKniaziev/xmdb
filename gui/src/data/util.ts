export function toHexString(s: string): string {
    let out = "";
    for (const c of s) {
        out += c.charCodeAt(0).toString(16).toString();
    }
    return out;
}

export function sha256HexDigest(s: string): string {
    let encoded = toHexString(s);
    for (let i = encoded.length; i < 64; ++i) {
        encoded += "0";
    }
    return encoded;
}

export function imageHexToDataUrl(data: string, width: number, height: number): string {
  const canvas = document.createElement("canvas");
  canvas.width = width;
  canvas.height = height;
  const ctx = canvas.getContext("2d");
  if (!ctx) return "";

  const imageData = ctx.createImageData(width, height);
  const uint8Data = imageData.data;

  for (let i = 0; i < data.length / 6; i++) {
    const r = parseInt(data.substring(i * 6, i * 6 + 2), 16);
    const g = parseInt(data.substring(i * 6 + 2, i * 6 + 4), 16);
    const b = parseInt(data.substring(i * 6 + 4, i * 6 + 6), 16);

    uint8Data[i * 4] = r;
    uint8Data[i * 4 + 1] = g;
    uint8Data[i * 4 + 2] = b;
    uint8Data[i * 4 + 3] = 255;
  }

  ctx.putImageData(imageData, 0, 0);
  return canvas.toDataURL();
}