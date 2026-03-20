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

  useEffect(() => {
    const currentPath = location.pathname;

    if (currentPath === '/') {
      setActiveTab(null);
    } else if (currentPath === '/query' || currentPath === '/gallery') {
      if (tabs.length === 0 && currentPath === '/query') {
        addTab('SQL Query');
      } else if (!activeTabId) {
        const lastTab = tabs[tabs.length - 1];
        if (lastTab) setActiveTab(lastTab.id);
      }
    }
  }, [location.pathname, activeTabId, tabs.length]);

  const addTabHandler = (path: string) => {
    if (path === '/query') {
      const sqlTabs = tabs.filter(tab => tab.title.startsWith('SQL Query'));
      const newTitle = sqlTabs.length === 0 ? 'SQL Query' : `SQL Query ${sqlTabs.length}`;
      addTab(newTitle);
      navigate('/query');
    }
  };

  const handleTabClick = (tabId: string) => {
    const tab = tabs.find(t => t.id === tabId);
    if (!tab) return;
    
    setActiveTab(tabId);
    if (tab.type === 'gallery') {
      navigate('/gallery');
    } else {
      navigate('/query');
    }
  };

  const handleTabClose = (tabId: string) => {
    closeTab(tabId);
    
    const tabIndex = tabs.findIndex(t => t.id === tabId);
    let nextTab = null;
    
    if (tabs.length > 1) {
      if (activeTabId === tabId) {
        nextTab = tabs[tabs.length - 1].id === tabId ? tabs[tabs.length - 2] : tabs[tabs.length - 1];
      } else {
        return;
      }
    }

    if (!nextTab) {
      navigate('/');
    } else {
      setActiveTab(nextTab.id);
      if (nextTab.type === 'gallery') {
        navigate('/gallery');
      } else {
        navigate('/query');
      }
    }
  };

  const tabBarTabs = tabs.map(t => ({
    id: t.id,
    title: t.isDirty ? `${t.title}*` : t.title,
    path: t.type === 'gallery' ? '/gallery' : '/query'
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
