let wasmMem: ArrayBuffer;
let wasmInstance: WebAssembly.Instance;

type Ptr = number;

function strlen(ptr: Ptr): number {
    const buf = new Uint8Array(wasmMem, ptr);
    let c = 0;
    for (; buf[c] != 0; ++c);
    return c;
}

function stringFromPtr(ptr: Ptr): string {
    const decoder = new TextDecoder();
    const len = strlen(ptr);
    const buf = new Uint8Array(wasmMem, ptr, len);
    return decoder.decode(buf);
}

function allocAligned(sz: number): Ptr {
    sz = sz + ((4 - (sz & 3)) & 3);
    const moduleAlloc = wasmInstance.exports.module_alloc as (sz: number) => Ptr;
    return moduleAlloc(sz);
}

function stringToPtr(string: string): Ptr {
    const ptr = allocAligned(string.length);
    const buf = new Uint8Array(wasmMem, ptr);
    const encoder = new TextEncoder();
    encoder.encodeInto(string, buf);
    return ptr;
}

function ptrToInt(ptr: Ptr): number {
    const buf = new Uint8Array(wasmMem, ptr);
    return buf[0] | buf[1] << 8 | buf[2] << 16 | buf[3] << 24;
}

export type SQLParsingResult = {
    ok: true;
} | {
    ok: false;
    line: number;
    col: number;
    error: string;
};

export function parseSQLSource(source: string): SQLParsingResult {
    const sourceWasm = stringToPtr(source);

    const linePtr = allocAligned(4);
    const colPtr = allocAligned(4);

    const errorBufCount = 512;
    const errorBuf = allocAligned(errorBufCount);

    const parseSource = wasmInstance.exports.sql_parse_source as (
        source: Ptr,
        sourceCount: number,
        line: Ptr,
        col: Ptr,
        error: Ptr,
        errorCount: number
    ) => number;
    const succ = parseSource(sourceWasm, source.length, linePtr, colPtr, errorBuf, errorBufCount);

    if (succ) {
        return { ok: true };
    }

    return {
        ok: false,
        line: ptrToInt(linePtr),
        col: ptrToInt(colPtr),
        error: stringFromPtr(errorBuf),
    };
}

let tempSprintfBuf: Ptr | undefined;

function sprintf(fmt: Ptr, args: Ptr): string {
    if (tempSprintfBuf === undefined) {
        tempSprintfBuf = allocAligned(1024);
    }

    const stbSprintf = wasmInstance.exports.stbsp_vsnprintf as (buf: Ptr, fmt: Ptr, args: Ptr) => number;
    stbSprintf(tempSprintfBuf, fmt, args);
    return stringFromPtr(tempSprintfBuf);
}

const wasmModulePath = "./src/sql/module.wasm";

export async function loadWasmModule() {
    if (wasmInstance) return;

    const wasmImports = {
        env: {
            console_log: (fmt: Ptr, args: Ptr) => {
                const string = sprintf(fmt, args);
                console.log(string);
            },
            console_error: (fmt: Ptr, args: Ptr) => {
                const string = sprintf(fmt, args);
                console.error(string);
            },
        },
    };

    const { instance } = await WebAssembly.instantiateStreaming(fetch(wasmModulePath), wasmImports);

    // @ts-expect-error
    wasmMem = instance.exports.memory.buffer;
    wasmInstance = instance;

    // @ts-expect-error
    instance.exports.__wasm_call_ctors();
}
