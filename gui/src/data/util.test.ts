import { afterEach, describe, expect, it, vi } from "vitest";
import {
  imageHexToDataUrl,
  pickFile,
  saveFile,
  sha256HexDigest,
  toHexString,
} from "./util";

describe("toHexString", () => {
  it("converts every character to its hexadecimal char code", () => {
    expect(toHexString("XMDB")).toBe("584d4442");
    expect(toHexString("A z!")).toBe("41207a21");
  });

  it("returns an empty string for empty input", () => {
    expect(toHexString("")).toBe("");
  });
});

describe("sha256HexDigest", () => {
  it("returns a lowercase SHA-256 digest", async () => {
    await expect(sha256HexDigest("xmdb")).resolves.toBe(
      "7a3ae67e5ef3cf65137cb3639bc0b18dcc57813018554d81425b93b2598ef1fb",
    );
  });
});

describe("pickFile", () => {
  afterEach(() => {
    vi.unstubAllGlobals();
  });

  it("opens a file input with the requested accept filter and resolves the selected file", async () => {
    const file = new File(["select * from users"], "query.sql", {
      type: "text/sql",
    });
    const input = {
      type: "",
      accept: "",
      files: [file],
      onchange: undefined as (() => void) | undefined,
      oncancel: undefined as (() => void) | undefined,
      click: vi.fn(() => input.onchange?.()),
    };

    const createElement = vi.fn(() => input);
    vi.stubGlobal("document", { createElement });

    await expect(pickFile(".sql")).resolves.toBe(file);
    expect(createElement).toHaveBeenCalledWith("input");
    expect(input.type).toBe("file");
    expect(input.accept).toBe(".sql");
    expect(input.click).toHaveBeenCalledOnce();
  });

  it("resolves null when the file picker is canceled", async () => {
    const input = {
      type: "",
      accept: "",
      files: [],
      onchange: undefined as (() => void) | undefined,
      oncancel: undefined as (() => void) | undefined,
      click: vi.fn(() => input.oncancel?.()),
    };

    vi.stubGlobal("document", { createElement: vi.fn(() => input) });

    await expect(pickFile("*/*")).resolves.toBeNull();
  });
});

describe("saveFile", () => {
  afterEach(() => {
    vi.unstubAllGlobals();
  });

  it("writes through the File System Access API when it is available", async () => {
    const write = vi.fn();
    const close = vi.fn();
    const createWritable = vi.fn().mockResolvedValue({ write, close });
    const showSaveFilePicker = vi.fn().mockResolvedValue({ createWritable });

    vi.stubGlobal("window", { showSaveFilePicker });

    await saveFile("select 1", "query.sql", "text/sql");

    expect(showSaveFilePicker).toHaveBeenCalledWith({
      suggestedName: "query.sql",
    });
    expect(write).toHaveBeenCalledWith(expect.any(Blob));
    expect(close).toHaveBeenCalledOnce();
  });

  it("falls back to a temporary download link when native saving is unavailable", async () => {
    const link = {
      href: "",
      download: "",
      click: vi.fn(),
    };
    const createObjectURL = vi.fn(() => "blob:download-url");
    const revokeObjectURL = vi.fn();

    vi.stubGlobal("window", {});
    vi.stubGlobal("document", { createElement: vi.fn(() => link) });
    vi.stubGlobal("URL", { createObjectURL, revokeObjectURL });

    await saveFile("select 1", "query.sql");

    expect(createObjectURL).toHaveBeenCalledWith(expect.any(Blob));
    expect(link.href).toBe("blob:download-url");
    expect(link.download).toBe("query.sql");
    expect(link.click).toHaveBeenCalledOnce();
    expect(revokeObjectURL).toHaveBeenCalledWith("blob:download-url");
  });
});

describe("imageHexToDataUrl", () => {
  afterEach(() => {
    vi.unstubAllGlobals();
  });

  it("copies RGB hex pixels into an opaque canvas image", () => {
    const putImageData = vi.fn();
    const pixels = new Uint8ClampedArray(8);
    const canvas = {
      width: 0,
      height: 0,
      getContext: vi.fn(() => ({
        createImageData: vi.fn(() => ({ data: pixels })),
        putImageData,
      })),
      toDataURL: vi.fn(() => "data:image/png;base64,test"),
    };

    vi.stubGlobal("document", { createElement: vi.fn(() => canvas) });

    const result = imageHexToDataUrl("ff000000ff00", 2, 1);

    expect(result).toBe("data:image/png;base64,test");
    expect(canvas.width).toBe(2);
    expect(canvas.height).toBe(1);
    expect([...pixels]).toEqual([255, 0, 0, 255, 0, 255, 0, 255]);
    expect(putImageData).toHaveBeenCalledWith({ data: pixels }, 0, 0);
  });
});
