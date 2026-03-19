import { useState } from "react";
import { ChevronDown, ChevronRight } from "lucide-react";
import { useConnectionStore } from "../data/global-states";

export default function Sidebar() {
  const [isOpen, setIsOpen] = useState(true);
  const { ConnectionId } = useConnectionStore();

  const isConnected = ConnectionId !== undefined;

  // Уявний список файлів з бази даних
  const tables = ["users", "orders", "products", "invoices"];

  return (
      <div className="sidebar-container">
        <div className="connection-status" style={{
          padding: "12px 20px",
          fontSize: "14px",
          fontWeight: "bold",
          fontFamily: 'var(--font-main)',
          backgroundColor: 'var(--color-darker)',
          color: isConnected ? "#6B7A3A" : "#690500"
        }}>
          {isConnected ? "Connected to the server" : "Not connected to the server"}
        </div>
        <div className="sidebar">
          <button onClick={() => setIsOpen(!isOpen)}>
            {isOpen ? <ChevronDown size={16} /> : <ChevronRight size={16} />}
            <span className="ml-2">Tables</span>
          </button>

          {isOpen && (
            <ul>
              {tables.map((table) => (
                <li key={table}>{table}</li>
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
