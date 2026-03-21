import { create } from "zustand";
import { devtools, persist, createJSONStorage } from "zustand/middleware";
import type { QueryResponse } from "./query-response";
import type { DBTable } from "./objects";

const indexedDBStorage = (() => {
  const dbName = "xmdb-zustand";
  const storeName = "keyval";

  function openDB(): Promise<IDBDatabase> {
    return new Promise((resolve, reject) => {
      const req = indexedDB.open(dbName, 1);
      req.onupgradeneeded = () => req.result.createObjectStore(storeName);
      req.onsuccess = () => resolve(req.result);
      req.onerror = () => reject(req.error);
    });
  }

  return {
    getItem: async (name: string): Promise<string | null> => {
      const db = await openDB();
      return new Promise((resolve) => {
        const tx = db.transaction(storeName, "readonly");
        const req = tx.objectStore(storeName).get(name);
        req.onsuccess = () => resolve(req.result ?? null);
        req.onerror = () => resolve(null);
      });
    },
    setItem: async (name: string, value: string): Promise<void> => {
      const db = await openDB();
      return new Promise((resolve) => {
        const tx = db.transaction(storeName, "readwrite");
        tx.objectStore(storeName).put(value, name);
        tx.oncomplete = () => resolve();
      });
    },
    removeItem: async (name: string): Promise<void> => {
      const db = await openDB();
      return new Promise((resolve) => {
        const tx = db.transaction(storeName, "readwrite");
        tx.objectStore(storeName).delete(name);
        tx.oncomplete = () => resolve();
      });
    },
  };
})();

export type ConnectionStore = {
  ConnectionId?: number;
  Hostname?: string;
  Database?: string;
  Username?: string;
  dbObjectsVersion: number;
  setId: (id: number) => void;
  addInfo: (
    hostname: string,
    database: string,
    username: string,
  ) => void;
  disconnect: () => void;
  bumpDbObjectsVersion: () => void;
};

export const useConnectionStore = create<ConnectionStore>()(
  devtools(
    persist(
      (set) => ({
        ConnectionId: undefined,
        Hostname: undefined,
        Database: undefined,
        Username: undefined,
        dbObjectsVersion: 0,
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
        bumpDbObjectsVersion: () =>
          set((state) => ({ dbObjectsVersion: state.dbObjectsVersion + 1 })),
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
}

export type MultiTabQueryStore = {
  tabs: TabState[];
  activeTabId: string | null;
  addTab: (title: string) => string;
  addGalleryTab: (title: string, response: QueryResponse, columnName: string, query: string) => string;
  addObjectTab: (table: DBTable) => string;
  addObjectsOverviewTab: () => string;
  openTab: (title: string, query: string) => string;
  closeTab: (id: string) => void;
  setActiveTab: (id: string | null) => void;
  updateActiveTabQuery: (query: string) => void;
  updateTabResults: (id: string, message: string, response?: QueryResponse, isLoading?: boolean) => void;
  updateTabBottomPanel: (id: string, tab: "messages" | "results" | "gallery", galleryInfo?: GalleryInfo) => void;
  updateTabSaveStatus: (id: string, isDirty: boolean, filePath?: string) => void;
  setTabLoading: (id: string, isLoading: boolean) => void;
  updateObjectTabs: (tables: DBTable[]) => void;
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
        openTab: (title, query) => {
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
        updateTabSaveStatus: (id, isDirty, filePath) => {
          set((state) => ({
            tabs: state.tabs.map((t) =>
              t.id === id ? { ...t, isDirty, filePath } : t
            ),
          }));
        },
        setTabLoading: (id, isLoading) => {
          set((state) => ({
            tabs: state.tabs.map((t) =>
              t.id === id ? { ...t, isLoading } : t
            ),
          }));
        },
        updateObjectTabs: (tables) => {
          set((state) => ({
            tabs: state.tabs.map((t) => {
              if (t.type !== "object" || !t.tableInfo) return t;
              const updated = tables.find((tbl) => tbl.name === t.tableInfo!.name);
              if (updated) return { ...t, tableInfo: updated };
              return t;
            }),
          }));
        }
      }),
      {
        name: "multi-tab-query-storage",
        storage: createJSONStorage(() => indexedDBStorage),
        partialize: (state) => ({
          ...state,
          tabs: state.tabs.map((t) => ({
            ...t,
            response: undefined,
            isLoading: false,
          })),
        }),
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
