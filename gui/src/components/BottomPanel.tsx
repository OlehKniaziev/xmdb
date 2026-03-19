import QueryResponse from "./QueryResponse";
import LoadingSpinner from "./LoadingSpinner";
import { useMultiTabQueryStore } from "../data/global-states";

interface BottomPanelProps {
  tabId: string;
}

function BottomPanel({ tabId }: BottomPanelProps) {
  const { tabs, updateTabBottomPanel } = useMultiTabQueryStore();
  const currentTab = tabs.find(t => t.id === tabId);

  if (!currentTab) return null;

  const activeTab = currentTab.bottomPanelTab || "messages";

  return (
    <div className="bottom-panel">
      <div className="tab-header">
        <button
          className={activeTab === "messages" ? "active-tab-bottom" : ""}
          onClick={() => updateTabBottomPanel(tabId, "messages")}
        >
          Messages
        </button>
        <button
          className={activeTab === "results" ? "active-tab-bottom" : ""}
          onClick={() => updateTabBottomPanel(tabId, "results")}
        >
          Results
        </button>
      </div>

      <div className="tab-content">
        {currentTab.isLoading ? (
          <LoadingSpinner />
        ) : activeTab === "messages" ? (
          <div className="messages-style">{currentTab.message}</div>
        ) : (
          <QueryResponse response={currentTab.response} isLoading={currentTab.isLoading} />
        )}
      </div>
    </div>
  );
}

export default BottomPanel;
