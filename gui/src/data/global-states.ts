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

export interface TabState {
  id: string;
  title: string;
  query: string;
  message: string;
  response?: QueryResponse;
  isLoading: boolean;
  bottomPanelTab: "messages" | "results";
}

export type MultiTabQueryStore = {
  tabs: TabState[];
  activeTabId: string | null;
  addTab: (title: string) => string;
  closeTab: (id: string) => void;
  setActiveTab: (id: string | null) => void;
  updateActiveTabQuery: (query: string) => void;
  updateTabResults: (id: string, message: string, response?: QueryResponse, isLoading?: boolean) => void;
  updateTabBottomPanel: (id: string, tab: "messages" | "results") => void;
  setTabLoading: (id: string, isLoading: boolean) => void;
}

export const useMultiTabQueryStore = create<MultiTabQueryStore>()(
  devtools(
    persist(
      (set, get) => ({
        tabs: [],
        activeTabId: null,
        addTab: (title) => {
          const id = `query-${Date.now()}`;
          const newTab: TabState = {
            id,
            title,
            query: "-- start writing your code here",
            message: "Output messages will be shown here",
            isLoading: false,
            bottomPanelTab: "messages",
          };
          set((state) => ({
            tabs: [...state.tabs, newTab],
            activeTabId: id,
          }));
          return id;
        },
        closeTab: (id) => {
          set((state) => {
            const newTabs = state.tabs.filter((t) => t.id !== id);
            let newActiveTabId = state.activeTabId;
            if (state.activeTabId === id) {
              newActiveTabId = newTabs.length > 0 ? newTabs[newTabs.length - 1].id : null;
            }
            return {
              tabs: newTabs,
              activeTabId: newActiveTabId,
            };
          });
        },
        setActiveTab: (id) => set({ activeTabId: id }),
        updateActiveTabQuery: (query) => {
          set((state) => ({
            tabs: state.tabs.map((t) =>
              t.id === state.activeTabId ? { ...t, query } : t
            ),
          }));
        },
        updateTabResults: (id, message, response, isLoading = false) => {
          set((state) => ({
            tabs: state.tabs.map((t) =>
              t.id === id ? { ...t, message, response, isLoading } : t
            ),
          }));
        },
        updateTabBottomPanel: (id, tab) => {
          set((state) => ({
            tabs: state.tabs.map((t) =>
              t.id === id ? { ...t, bottomPanelTab: tab } : t
            ),
          }));
        },
        setTabLoading: (id, isLoading) => {
          set((state) => ({
            tabs: state.tabs.map((t) =>
              t.id === id ? { ...t, isLoading } : t
            ),
          }));
        }
      }),
      {
        name: "multi-tab-query-storage",
      }
    )
  )
);

// Keep these for backward compatibility during transition or if they are still needed elsewhere
// but eventually we should remove them if they are only used for the query editor.

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
      (set) => ({
        message: "Output messages will be shown here",
        setMessage: (message) => set({message}),
      }),
      {
        name: "message-storage"
      }
  )
);

export type QueryResponseStore = {
  response?: QueryResponse;
  setQueryResponse: (response?: QueryResponse) => void;
  isLoading: boolean;
  setIsLoading: (isLoading: boolean) => void;
}

export const useQueryResponseStore = create<QueryResponseStore>()(
   devtools(
      (set) => ({
        response: undefined,
        setQueryResponse: (response) => set({response}),
        isLoading: false,
        setIsLoading: (isLoading) => set({isLoading}),
      }),
      {
        name: "query-response-storage"
      }
  )
);