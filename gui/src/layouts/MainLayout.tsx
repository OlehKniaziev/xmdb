import Sidebar from "../components/Sidebar";
import Topbar from "../components/Topbar";
import Toolbar from "../components/Toolbar";
import { Outlet, useLocation, useNavigate } from "react-router-dom";
import { useEffect } from "react";
import TabBar from "../components/TabBar";
import { Panel, PanelGroup, PanelResizeHandle } from "react-resizable-panels";
import { useMultiTabQueryStore } from "../data/global-states";

export default function MainLayout() {
  const { tabs, activeTabId, addTab, closeTab, setActiveTab } = useMultiTabQueryStore();
  const location = useLocation();
  const navigate = useNavigate();

  // Handle navigation changes - don't automatically create tabs
  useEffect(() => {
    const currentPath = location.pathname;

    if (currentPath === '/') {
      // On home page, no tabs should be active
      setActiveTab(null);
    } else if (currentPath === '/query') {
      // On SQL page, ensure a tab exists and set it as active
      if (tabs.length === 0) {
        // Create a new SQL query tab if none exists
        addTab('SQL Query');
      } else if (!activeTabId) {
        // If there are tabs but none active, activate the last one
        setActiveTab(tabs[tabs.length - 1].id);
      }
    } else {
       // For other paths, we don't have tab support yet, but we could add it
    }
  }, [location.pathname]);

  const addTabHandler = (path: string) => {
    if (path === '/query') {
      const sqlTabs = tabs.filter(tab => tab.title.startsWith('SQL Query'));
      const newTitle = sqlTabs.length === 0 ? 'SQL Query' : `SQL Query ${sqlTabs.length}`;
      addTab(newTitle);
      navigate('/query');
    }
  };

  const handleTabClick = (tabId: string) => {
    setActiveTab(tabId);
    // Since all query tabs share the same '/query' path, we just navigate there
    navigate('/query');
  };

  const handleTabClose = (tabId: string) => {
    closeTab(tabId);
    
    // If we closed the last tab, go home
    if (tabs.length === 1 && tabs[0].id === tabId) {
      navigate('/');
    }
  };

  // Convert TabState to the format TabBar expects
  const tabBarTabs = tabs.map(t => ({
    id: t.id,
    title: t.isDirty ? `${t.title}*` : t.title,
    path: '/query'
  }));

  return (
    <div id="main" className="flex">
      <div className="topbar">
        <Topbar />
        <Toolbar onAddTab={addTabHandler} />
      </div>
      <div className="main-content">
        <PanelGroup direction="horizontal">
          <Panel minSize={1} maxSize={20} defaultSize={16}>
            <Sidebar />
          </Panel>
          <PanelResizeHandle />
          <Panel>
            <div className="content-container">
              <TabBar
                tabs={tabBarTabs}
                activeTab={activeTabId}
                onTabClick={handleTabClick}
                onTabClose={handleTabClose}
              />
              <div className="content-style">
                <Outlet />
              </div>
            </div>
          </Panel>
        </PanelGroup>
      </div>
    </div>
  );
}
