import { useMultiTabQueryStore } from "../data/global-states";

export default function ObjectView() {
  const { tabs, activeTabId } = useMultiTabQueryStore();
  const activeTab = tabs.find((t) => t.id === activeTabId);

  if (!activeTab || activeTab.type !== "object" || !activeTab.tableInfo) {
    return <div className="messages-style">No active object tab.</div>;
  }

  const { name, column_names, column_types } = activeTab.tableInfo;

  return (
    <div style={{ padding: "24px 32px", height: "100%", boxSizing: "border-box", overflowY: "auto" }}>
      <h2 style={{ marginBottom: "24px", fontFamily: "var(--font-header)" }}>Table: {name}</h2>
      <table className="query-response-table">
        <thead>
          <tr style={{border: "1px solid var(--color-black)"}}>
            <th style={{ textAlign: "left", border: "1px solid var(--color-black)" }}>Column</th>
            <th style={{ textAlign: "left", border: "1px solid var(--color-black)" }}>Type</th>
          </tr>
        </thead>
        <tbody>
          {column_names.map((col, i) => (
            <tr key={col} style={{border: "1px solid var(--color-accent-middle)"}}>
              <td style={{border: "1px solid var(--color-accent-middle)"}}>{col}</td>
              <td style={{ color: "var(--color-accent-middle)", border: "1px solid var(--color-accent-middle)" }}>{column_types[i]}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}
