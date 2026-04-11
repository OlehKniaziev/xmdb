import { useState, useEffect } from "react";
import { ChevronDown, ChevronRight } from "lucide-react";
import {
  useConnectionStore,
  useMultiTabQueryStore,
} from "../data/global-states";
import { useNavigate } from "react-router-dom";
import type { DBTable, DBObjectsResponse } from "../data/objects";

export default function Sidebar() {
  const [isOpen, setIsOpen] = useState(true);
  const [tables, setTables] = useState<DBTable[]>([]);
  const [searchQuery, setSearchQuery] = useState("");
  const [showSuggestions, setShowSuggestions] = useState(false);
  const { ConnectionId, Hostname, dbObjectsVersion } = useConnectionStore();
  const { addObjectTab, updateObjectTabs } = useMultiTabQueryStore();
  const navigate = useNavigate();

  const isConnected = ConnectionId !== undefined;

  const trimmedQuery = searchQuery.trim().toLowerCase();
  const suggestions = trimmedQuery
    ? tables.filter((t) => t.name.toLowerCase().includes(trimmedQuery))
    : [];

  function openTable(table: DBTable) {
    addObjectTab(table);
    navigate("/object");
    setSearchQuery("");
    setShowSuggestions(false);
  }

  useEffect(() => {
    async function fetchTables() {
      if (isConnected && Hostname) {
        try {
          const resp = await fetch(`${Hostname}/get-db-objects`, {
            method: "POST",
            body: JSON.stringify({ connection_id: ConnectionId }),
          });

          if (resp.ok) {
            const data = (await resp.json()) as DBObjectsResponse;
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
      <div
        className="connection-status"
        style={{
          padding: "12px 20px",
          fontSize: "14px",
          fontWeight: "bold",
          fontFamily: "var(--font-main)",
          backgroundColor: "var(--color-darker)",
          color: isConnected
            ? "var(--color-accent-green)"
            : "var(--color-accent-dark)",
        }}
      >
        {isConnected
          ? "Connected to the server"
          : "Not connected to the server"}
      </div>

      <div style={{ position: "relative", flex: 1, minHeight: 0 }}>
        <div className="sidebar">
          <div
            style={{
              display: "inline-block",
              minWidth: "100%",
              boxSizing: "border-box",
              paddingRight: "16px",
            }}
          >
            <button
              onClick={() => setIsOpen(!isOpen)}
              style={{ fontWeight: "bold", cursor: "pointer" }}
            >
              {isOpen ? (
                <ChevronDown size={16} style={{ marginRight: "6px" }} />
              ) : (
                <ChevronRight size={16} style={{ marginRight: "6px" }} />
              )}
              <span className="ml-2">Tables</span>
              <span style={{ color: "var(--color-grey)", marginLeft: "6px" }}>
                ({tables.length})
              </span>
            </button>

            {isOpen && (
              <ul
                style={{
                  listStyle: "none",
                  paddingLeft: "40px",
                  marginTop: "8px",
                }}
              >
                {tables.map((table) => (
                  <li
                    key={table.name}
                    style={{
                      padding: "2px 0",
                      fontFamily: "var(--font-main)",
                      cursor: "pointer",
                      whiteSpace: "nowrap",
                    }}
                    onClick={() => openTable(table)}
                  >
                    {table.name}
                  </li>
                ))}
              </ul>
            )}
          </div>
        </div>
      </div>
      <div style={{ position: "relative" }}>
        <input
          type="text"
          className="sidebar-search-button"
          placeholder="Search..."
          value={searchQuery}
          onChange={(e) => {
            setSearchQuery(e.target.value);
            setShowSuggestions(true);
          }}
          onFocus={() => setShowSuggestions(true)}
          onBlur={() => setTimeout(() => setShowSuggestions(false), 150)}
        />
        {showSuggestions && trimmedQuery && (
          <ul
            style={{
              position: "absolute",
              bottom: "100%",
              left: 0,
              right: 0,
              margin: "0 12px 4px 12px",
              padding: 0,
              listStyle: "none",
              background: "var(--color-darker)",
              border: "1px solid var(--color-brown)",
              borderRadius: "4px",
              maxHeight: "200px",
              overflowY: "auto",
              zIndex: 10,
              fontFamily: "var(--font-main)",
              fontSize: "13px",
              boxShadow: "0 -2px 8px rgba(0,0,0,0.15)",
            }}
          >
            {suggestions.length === 0 ? (
              <li
                style={{
                  padding: "8px 12px",
                  color: "var(--color-grey)",
                  fontStyle: "italic",
                }}
              >
                No matches
              </li>
            ) : (
              suggestions.map((table) => (
                <li
                  key={table.name}
                  onMouseDown={(e) => {
                    e.preventDefault();
                    openTable(table);
                  }}
                  style={{
                    padding: "8px 12px",
                    cursor: "pointer",
                    color: "var(--color-black)",
                    borderBottom: "1px solid var(--color-brown)",
                  }}
                  onMouseEnter={(e) =>
                    (e.currentTarget.style.background =
                      "var(--color-dark-beige)")
                  }
                  onMouseLeave={(e) =>
                    (e.currentTarget.style.background = "transparent")
                  }
                >
                  {table.name}
                </li>
              ))
            )}
          </ul>
        )}
      </div>
    </div>
  );
}
