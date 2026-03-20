import QueryResponse from "./QueryResponse";
import LoadingSpinner from "./LoadingSpinner";
import { useMultiTabQueryStore } from "../data/global-states";
import { useNavigate } from "react-router-dom";

interface BottomPanelProps {
  tabId: string;
}

export default function BottomPanel({ tabId }: BottomPanelProps) {
  const { tabs, updateTabBottomPanel, addGalleryTab } = useMultiTabQueryStore();
  const currentTab = tabs.find(t => t.id === tabId);
  const navigate = useNavigate();

  if (!currentTab) return null;

  const activeTab = currentTab.bottomPanelTab || "messages";

  const handleGalleryView = (columnName: string) => {
    if (currentTab.response) {
      const title = `Gallery - ${columnName}`;
      addGalleryTab(title, currentTab.response, columnName, currentTab.query);
      navigate("/gallery");
    }
  };

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
          <QueryResponse
            response={currentTab.response}
            isLoading={currentTab.isLoading}
            onGalleryView={handleGalleryView}
          />
        )}
      </div>
    </div>
  );
}
