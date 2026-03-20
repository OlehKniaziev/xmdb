export type DBTable = {
    name: string;
    column_names: string[];
    column_types: string[];
}

export type DBObjectsResponse = {
    ok: true;
    tables: DBTable[];
} | {
    ok: false;
    error: string;
}