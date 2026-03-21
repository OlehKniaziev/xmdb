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

export function imageHexToDataUrl(
  data: string,
  width: number,
  height: number,
): string {
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

export async function fileToHexDataString(file: File): Promise<{
    width: number;
    height: number;
    data: string;
}> {
  const img = new Image();
  img.src = URL.createObjectURL(file);

  await img.decode();

  const canvas = document.createElement("canvas");
  canvas.width = img.width;
  canvas.height = img.height;

  const ctx = canvas.getContext("2d")!;
  ctx.drawImage(img, 0, 0);

  const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);

  const rgba = imageData.data; // Uint8ClampedArray (RGBA)

  let hexString = "";

  for (let i = 0; i < rgba.length; i += 4) {
    hexString += rgba[i].toString(16).padStart(2, "0");
    hexString += rgba[i + 1].toString(16).padStart(2, "0");
    hexString += rgba[i + 2].toString(16).padStart(2, "0");
  }

  return {
    width: img.width,
    height: img.height,
    data: hexString
  };
}
