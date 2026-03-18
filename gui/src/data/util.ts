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