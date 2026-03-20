import { create } from "zustand";
import { devtools, persist } from "zustand/middleware";
import type { QueryResponse } from "./query-response";
import type { DBTable } from "./objects";

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

export interface GalleryInfo {
  columnName: string;
}

export interface TabState {
  id: string;
  title: string;
  type: "query" | "gallery" | "object" | "objects-overview";
  query: string;
  message: string;
  response?: QueryResponse;
  tableInfo?: DBTable;
  isLoading: boolean;
  bottomPanelTab: "messages" | "results" | "gallery";
  galleryInfo?: GalleryInfo;
  isDirty: boolean;
  filePath?: string;
  fileHandle?: FileSystemFileHandle;
}

export type MultiTabQueryStore = {
  tabs: TabState[];
  activeTabId: string | null;
  addTab: (title: string) => string;
  addGalleryTab: (title: string, response: QueryResponse, columnName: string, query: string) => string;
  addObjectTab: (table: DBTable) => string;
  addObjectsOverviewTab: () => string;
  openTab: (title: string, query: string, fileHandle: FileSystemFileHandle) => string;
  closeTab: (id: string) => void;
  setActiveTab: (id: string | null) => void;
  updateActiveTabQuery: (query: string) => void;
  updateTabResults: (id: string, message: string, response?: QueryResponse, isLoading?: boolean) => void;
  updateTabBottomPanel: (id: string, tab: "messages" | "results" | "gallery", galleryInfo?: GalleryInfo) => void;
  updateTabSaveStatus: (id: string, isDirty: boolean, filePath?: string, fileHandle?: FileSystemFileHandle) => void;
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
            type: "query",
            query: "-- start writing your code here",
            message: "Output messages will be shown here",
            isLoading: false,
            bottomPanelTab: "messages",
            isDirty: false,
          };
          set((state) => ({
            tabs: [...state.tabs, newTab],
            activeTabId: id,
          }));
          return id;
        },
        addGalleryTab: (title, response, columnName, query) => {
          const id = `gallery-${Date.now()}`;
          const newTab: TabState = {
            id,
            title,
            type: "gallery",
            query: query,
            response: response,
            message: "",
            isLoading: false,
            bottomPanelTab: "gallery",
            galleryInfo: { columnName },
            isDirty: false,
          };
          set((state) => ({
            tabs: [...state.tabs, newTab],
            activeTabId: id,
          }));
          return id;
        },
        addObjectTab: (table) => {
          const id = `object-${table.name}`;
          // Check if tab for this table already exists
          const existingTab = get().tabs.find(t => t.type === 'object' && t.tableInfo?.name === table.name);
          if (existingTab) {
            set({ activeTabId: existingTab.id });
            return existingTab.id;
          }

          const newTab: TabState = {
            id,
            title: table.name,
            type: "object",
            query: "",
            message: "",
            tableInfo: table,
            isLoading: false,
            bottomPanelTab: "messages",
            isDirty: false,
          };
          set((state) => ({
            tabs: [...state.tabs, newTab],
            activeTabId: id,
          }));
          return id;
        },
        addObjectsOverviewTab: () => {
          const existing = get().tabs.find(t => t.type === 'objects-overview');
          if (existing) {
            set({ activeTabId: existing.id });
            return existing.id;
          }
          const id = 'objects-overview';
          const newTab: TabState = {
            id,
            title: "Object View",
            type: "objects-overview",
            query: "",
            message: "",
            isLoading: false,
            bottomPanelTab: "messages",
            isDirty: false,
          };
          set((state) => ({
            tabs: [...state.tabs, newTab],
            activeTabId: id,
          }));
          return id;
        },
        openTab: (title, query, fileHandle) => {
          const id = `query-${Date.now()}`;
          const newTab: TabState = {
            id,
            title,
            type: "query",
            query,
            message: "Output messages will be shown here",
            isLoading: false,
            bottomPanelTab: "messages",
            isDirty: false,
            filePath: title,
            fileHandle: fileHandle,
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
              t.id === state.activeTabId ? { ...t, query, isDirty: true } : t
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
        updateTabBottomPanel: (id, tab, galleryInfo) => {
          set((state) => ({
            tabs: state.tabs.map((t) =>
              t.id === id ? { ...t, bottomPanelTab: tab, galleryInfo: galleryInfo || t.galleryInfo } : t
            ),
          }));
        },
        updateTabSaveStatus: (id, isDirty, filePath, fileHandle) => {
          set((state) => ({
            tabs: state.tabs.map((t) =>
              t.id === id ? { ...t, isDirty, filePath, fileHandle } : t
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
