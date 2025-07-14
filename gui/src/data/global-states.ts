import { create } from "zustand";
import { devtools, persist } from "zustand/middleware";
import type { QueryResponse } from "./query-response";

export type ConnectionStore = {
  ConnectionId?: number;
  Hostname?: string;
  Database?: string;
  Username?: string;
  setId: (id: number) => void;
  addInfo: (
    hostname: string,
    database: string,
    username: string,
  ) => void;
  disconnect: () => void;
};

export const useConnectionStore = create<ConnectionStore>()(
  devtools(
    persist(
      (set) => ({
        ConnectionId: undefined,
        Hostname: undefined,
        Database: undefined,
        Username: undefined,
        setId: (id) => set({ ConnectionId: id }),
        addInfo: (hostname, database, username) =>
          set({
            Hostname: hostname,
            Database: database,
            Username: username,
          }),
        disconnect: () =>
          set({
            ConnectionId: undefined,
            Hostname: undefined,
            Database: undefined,
            Username: undefined,
          }),
      }),
      {
        name: "connection-info-storage",
      }
    )
  )
);

export type QueryStore = {
  query: string;
  setQuery: (query: string) => void;
}

export const useQueryStore = create<QueryStore>()(
   devtools(
    persist(
      (set) => ({
        query: "-- start writing your code here",
        setQuery: (query) => set({query}),
      }),
      {
        name: "query-storage"
      }
    )
  )
);


export type OutputMessagesStore = {
  message: string;
  setMessage: (message: string) => void;
}

export const useOutputMessagesStore = create<OutputMessagesStore>()(
   devtools(
    persist(
      (set) => ({
        message: "Output messages will be shown here",
        setMessage: (message) => set({message}),
      }),
      {
        name: "message-storage"
      }
    )
  )
);

export type QueryResponseStore = {
  response?: QueryResponse;
  setQueryResponce: (response?: QueryResponse) => void;
}

export const useQueryResponceStore = create<QueryResponseStore>()(
   devtools(
    persist(
      (set) => ({
        response: undefined,
        setQueryResponce: (response) => set({response}),
      }),
      {
        name: "query-response-storage"
      }
    )
  )
);