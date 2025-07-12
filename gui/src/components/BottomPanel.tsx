import { useState } from "react";
import { sampleData } from "../data/query-responce";
import QueryResponse from "./QueryResponse";

function BottomPanel() {
  const [activeTab, setActiveTab] = useState<"messages" | "results">(
    "messages"
  );

  return (
      <div className="bottom-panel">
        <div className="tab-header">
          <button
            className={activeTab === "messages" ? "active-tab-bottom" : ""}
            onClick={() => setActiveTab("messages")}
          >
            Messages
          </button>
          <button
            className={activeTab === "results" ? "active-tab-bottom" : ""}
            onClick={() => setActiveTab("results")}
          >
            Results
          </button>
        </div>

        <div className="tab-content">
          {activeTab === "messages" ? (
            <div className="messages-style">Output messages here</div>
          ) : (
            <QueryResponse response={sampleData} />
          )}
        </div>
      </div>
  );
}

export default BottomPanel;
