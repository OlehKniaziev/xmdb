export type QueryResponse = {
  columnNames: string[];
  rows: { [columnName: string]: any }[];
};

export const sampleData: QueryResponse = {
  columnNames: ["column1", "column2", "column3"],
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
