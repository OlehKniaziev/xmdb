import { useState, useEffect } from "react";
import { useNavigate } from "react-router-dom";
import { useConnectionStore, useMultiTabQueryStore } from "../data/global-states";
import type { DBTable, DBObjectsResponse } from "../data/objects";

export default function ObjectsOverview() {
  const [tables, setTables] = useState<DBTable[]>([]);
  const { ConnectionId, Hostname, dbObjectsVersion } = useConnectionStore();
  const { addObjectTab } = useMultiTabQueryStore();
  const navigate = useNavigate();

  useEffect(() => {
    async function fetchTables() {
      if (!ConnectionId || !Hostname) {
        setTables([]);
        return;
      }
      try {
        const resp = await fetch(`${Hostname}/get-db-objects`, {
          method: "POST",
          body: JSON.stringify({ connection_id: ConnectionId }),
        });
        if (resp.ok) {
          const data = await resp.json() as DBObjectsResponse;
          if (data.ok) setTables(data.tables);
        }
      } catch (e) {
        console.error("Error fetching tables:", e);
      }
    }
    fetchTables();
  }, [ConnectionId, Hostname, dbObjectsVersion]);

  function handleTableClick(table: DBTable) {
    addObjectTab(table);
    navigate("/object");
  }

  if (!ConnectionId) {
    return (
      <div className="messages-style">
        Not connected. Connect to a database to view objects.
      </div>
    );
  }

  return (
    <div style={{
      padding: "24px 32px",
      height: "100%",
      boxSizing: "border-box",
      overflowY: "auto",
    }}>
      <h2 style={{ marginBottom: "24px", fontFamily: "var(--font-header)" }}>
        Database Objects
      </h2>
      {tables.length === 0 ? (
        <p style={{ color: "var(--color-grey)" }}>No tables found.</p>
      ) : (
        <div style={{
          display: "flex",
          flexWrap: "wrap",
          gap: "16px",
          alignContent: "flex-start",
        }}>
          {tables.map((table) => (
            <div
              key={table.name}
              onClick={() => handleTableClick(table)}
              style={{
                background: "var(--color-darker)",
                border: "1px solid var(--color-brown)",
                borderRadius: "6px",
                padding: "16px 20px",
                minWidth: "200px",
                maxWidth: "300px",
                cursor: "pointer",
                transition: "background 0.15s ease",
              }}
              onMouseEnter={e => (e.currentTarget.style.background = "var(--color-dark-beige)")}
              onMouseLeave={e => (e.currentTarget.style.background = "var(--color-darker)")}
            >
              <div style={{
                fontFamily: "var(--font-header)",
                fontWeight: "bold",
                fontSize: "15px",
                marginBottom: "10px",
                color: "var(--color-accent-dark)",
                borderBottom: "1px solid var(--color-brown)",
                paddingBottom: "8px",
              }}>
                {table.name}
              </div>
              <ul style={{ listStyle: "none", padding: 0, margin: 0 }}>
                {table.column_names.map((col) => (
                  <li key={col} style={{
                    fontFamily: "var(--font-main)",
                    fontSize: "13px",
                    padding: "2px 0",
                    color: "var(--color-black)",
                  }}>
                    {col}
                  </li>
                ))}
              </ul>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
