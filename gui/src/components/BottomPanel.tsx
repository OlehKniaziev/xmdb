import { useState } from "react";
import { sampleData } from "../data/query-response";
import QueryResponse from "./QueryResponse";
import { useOutputMessagesStore, useQueryResponceStore } from "../data/global-states";

function BottomPanel() {
  const [activeTab, setActiveTab] = useState<"messages" | "results">(
    "messages"
  );

  const { message } = useOutputMessagesStore();
  const { response } = useQueryResponceStore();

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
          <div className="messages-style">{message}</div>
        ) : (
          <QueryResponse response={response} />
        )}
      </div>
    </div>
  );
}

export default BottomPanel;
