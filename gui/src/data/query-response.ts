export type ImageChunk = {
  x: number;
  y: number;
  width: number;
  height: number;
  data: string; // Hex-encoded RGB data
}

export type ColumnType =
  | "INTEGER"
  | "FLOAT"
  | "TEXT"
  | "BOOLEAN"
  | "PNG";

export type QueryResponse =
  | {
      ok: true;
      column_names?: string[];
      column_types?: ColumnType[];
      rows?: { [columnName: string]: unknown }[];
    }
  | {
      ok: false;
      error_message: string;
    };

export const sampleData: QueryResponse = {
  ok: true,
  column_names: ["column1", "column2", "column3"],
  rows: [
    {
      column1: 1,
      column2: "value1",
      column3: 22.4,
    },
    {
      column1: 2,
      column2: "value2",
      column3: 11.6,
    },
    {
      column1: 3,
      column2: "value3",
      column3: 66.1,
    },
    {
      column1: 4,
      column2: "value4",
      column3: 42.4,
    },
    {
      column1: 1,
      column2: "value1",
      column3: 22.4,
    },
    {
      column1: 2,
      column2: "value2",
      column3: 11.6,
    },
    {
      column1: 3,
      column2: "value3",
      column3: 66.1,
    },
    {
      column1: 4,
      column2: "value4",
      column3: 42.4,
    },
    {
      column1: 1,
      column2: "value1",
      column3: 22.4,
    },
    {
      column1: 2,
      column2: "value2",
      column3: 11.6,
    },
    {
      column1: 3,
      column2: "value3",
      column3: 66.1,
    },
    {
      column1: 4,
      column2: "value4",
      column3: 42.4,
    },
    {
      column1: 1,
      column2: "value1",
      column3: 22.4,
    },
    {
      column1: 2,
      column2: "value2",
      column3: 11.6,
    },
    {
      column1: 3,
      column2: "value3",
      column3: 66.1,
    },
    {
      column1: 4,
      column2: "value4",
      column3: 42.4,
    },
  ],
};
