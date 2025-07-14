import { create } from "zustand";
import { devtools, persist } from "zustand/middleware";

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
        setId: (id) => set(() => ({ ConnectionId: id })),
        addInfo: (hostname, database, username) =>
          set(() => ({
            Hostname: hostname,
            Database: database,
            Username: username,
          })),
        disconnect: () =>
          set(() => ({
            ConnectionId: undefined,
            Hostname: undefined,
            Database: undefined,
            Username: undefined,
          })),
      }),
      {
        name: "connection-info-storage",
      }
    )
  )
);
