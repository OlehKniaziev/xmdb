import { useState, useEffect } from "react";
import { ChevronDown, ChevronRight } from "lucide-react";
import { useConnectionStore, useMultiTabQueryStore } from "../data/global-states";
import { useNavigate } from "react-router-dom";
import type { DBTable, DBObjectsResponse } from "../data/objects";

export default function Sidebar() {
  const [isOpen, setIsOpen] = useState(true);
  const [tables, setTables] = useState<DBTable[]>([]);
  const { ConnectionId, Hostname, dbObjectsVersion } = useConnectionStore();
  const { addObjectTab, updateObjectTabs } = useMultiTabQueryStore();
  const navigate = useNavigate();

  const isConnected = ConnectionId !== undefined;

  useEffect(() => {
    async function fetchTables() {
      if (isConnected && Hostname) {
        try {
          const resp = await fetch(`${Hostname}/get-db-objects`, {
            method: "POST",
            body: JSON.stringify({ connection_id: ConnectionId }),
          });

          if (resp.ok) {
            const data = await resp.json() as DBObjectsResponse;
            if (data.ok) {
              setTables(data.tables);
              updateObjectTabs(data.tables);
            }
          } else {
            console.error("Failed to fetch database objects");
            setTables([]);
          }
        } catch (error) {
          console.error("Error fetching database objects:", error);
          setTables([]);
        }
      } else {
        setTables([]);
      }
    }

    fetchTables();
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [isConnected, ConnectionId, Hostname, dbObjectsVersion]);

  return (
      <div className="sidebar-container">
        <div className="connection-status" style={{
          padding: "12px 20px",
          fontSize: "14px",
          fontWeight: "bold",
          fontFamily: 'var(--font-main)',
          backgroundColor: 'var(--color-darker)',
          color: isConnected ? "var(--color-accent-green)" : "var(--color-accent-dark)",
        }}>
          {isConnected ? "Connected to the server" : "Not connected to the server"}
        </div>

        <div className="sidebar">
          <button onClick={() => setIsOpen(!isOpen)} style={{fontWeight: "bold", cursor: "pointer"}}>
            {isOpen ? <ChevronDown size={16} style={{marginRight: "6px"}}/> : <ChevronRight size={16} style={{marginRight: "6px"}} />}
            <span className="ml-2">Tables</span>
            <span style={{color: "var(--color-grey)", marginLeft: "6px"}}>({tables.length})</span>
          </button>

          {isOpen && (
            <ul style={{ listStyle: 'none', paddingLeft: '40px', marginTop: '8px' }}>
              {tables.map((table) => (
                <li
                  key={table.name}
                  style={{ padding: '2px 0', fontFamily: 'var(--font-main)', cursor: 'pointer' }}
                  onClick={() => { addObjectTab(table); navigate('/object'); }}
                >{table.name}</li>
              ))}
            </ul>
          )}
        </div>
        <input
          type="text"
          className="sidebar-search-button"
          placeholder="Search..."
        />
      </div>
  );
}
