import { useState } from "react";
import { ChevronDown, ChevronRight } from "lucide-react";

export default function Sidebar() {
  const [isOpen, setIsOpen] = useState(true);

  // Уявний список файлів з бази даних
  const tables = ["users", "orders", "products", "invoices"];

  return (
      <div className="sidebar-container">
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
