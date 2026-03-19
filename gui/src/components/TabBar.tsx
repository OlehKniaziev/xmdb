interface Tab {
  id: string;
  title: string;
  path: string;
}

interface TabBarProps {
  tabs: Tab[];
  activeTab: string | null;
  onTabClick: (tabId: string) => void;
  onTabClose: (tabId: string) => void;
}

export default function TabBar({ tabs, activeTab, onTabClick, onTabClose }: TabBarProps) {
  if (tabs.length === 0) {
    return (
      <div className="tab-bar-container">
        <div className="tab-bar-empty">
          No files are currently open
        </div>
      </div>
    );
  }

  return (
    <div className="tab-bar-container">
      <div className="tab-bar-tabs">
        {tabs.map((tab) => (
          <div
            key={tab.id}
            className={`tab-item ${activeTab === tab.id ? 'active' : ''}`}
            onClick={() => onTabClick(tab.id)}
          >
            <span className="tab-title">{tab.title}</span>
            <button
              className="tab-close"
              onClick={(e) => {
                e.stopPropagation();
                onTabClose(tab.id);
              }}
            >
              ×
            </button>
          </div>
        ))}
      </div>
      <div className="tab-bar-spacer"></div>
    </div>
  );
}
