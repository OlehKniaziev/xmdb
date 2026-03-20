import Sidebar from "../components/Sidebar";
import Topbar from "../components/Topbar";
import Toolbar from "../components/Toolbar";
import { Outlet, useLocation, useNavigate } from "react-router-dom";
import { useEffect, useRef } from "react";
import TabBar from "../components/TabBar";
import { Panel, PanelGroup, PanelResizeHandle } from "react-resizable-panels";
import { useMultiTabQueryStore } from "../data/global-states";

export default function MainLayout() {
  const { tabs, activeTabId, addTab, addObjectsOverviewTab, closeTab, setActiveTab } = useMultiTabQueryStore();
  const location = useLocation();
  const navigate = useNavigate();

  // Keep refs so the pathname-only effect always reads latest values without
  // re-running when activeTabId or tabs change (which would cause race conditions
  // when setActiveTab and navigate are called together in tab click handlers).
  const tabsRef = useRef(tabs);
  tabsRef.current = tabs;
  const activeTabIdRef = useRef(activeTabId);
  activeTabIdRef.current = activeTabId;

  useEffect(() => {
    const currentPath = location.pathname;
    const currentTabs = tabsRef.current;
    const currentActiveTabId = activeTabIdRef.current;
    const activeTab = currentTabs.find((tab) => tab.id === currentActiveTabId);

    if (currentPath === "/query") {
      if (!activeTab || activeTab.type !== "query") {
        const lastQueryTab = [...currentTabs].reverse().find((t) => t.type === "query");
        if (lastQueryTab) {
          setActiveTab(lastQueryTab.id);
        } else if (currentTabs.length === 0) {
          addTab("SQL Query");
        }
      }
    } else if (currentPath === "/gallery") {
      if (!activeTab || activeTab.type !== "gallery") {
        const lastGalleryTab = [...currentTabs].reverse().find((t) => t.type === "gallery");
        if (lastGalleryTab) {
          setActiveTab(lastGalleryTab.id);
        } else {
          navigate("/query");
        }
      }
    } else if (currentPath === "/object") {
      if (!activeTab || activeTab.type !== "object") {
        const lastObjectTab = [...currentTabs].reverse().find((t) => t.type === "object");
        if (lastObjectTab) {
          setActiveTab(lastObjectTab.id);
        } else {
          navigate("/query");
        }
      }
    } else if (currentPath === "/objects") {
      if (!activeTab || activeTab.type !== "objects-overview") {
        const overviewTab = currentTabs.find((t) => t.type === "objects-overview");
        if (overviewTab) {
          setActiveTab(overviewTab.id);
        } else {
          addObjectsOverviewTab();
        }
      }
    } else if (currentPath === "/") {
      if (currentActiveTabId !== null) {
        setActiveTab(null);
      }
    }
  // eslint-disable-next-line react-hooks/exhaustive-deps
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
    const tab = tabs.find(t => t.id === tabId);
    if (!tab) return;
    
    setActiveTab(tabId);
    if (tab.type === 'gallery') {
      navigate('/gallery');
    } else if (tab.type === 'object') {
      navigate('/object');
    } else if (tab.type === 'objects-overview') {
      navigate('/objects');
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
      } else if (nextTab.type === 'object') {
        navigate('/object');
      } else if (nextTab.type === 'objects-overview') {
        navigate('/objects');
      } else {
        navigate('/query');
      }
    }
  };

  const tabBarTabs = tabs.map(t => ({
    id: t.id,
    title: t.isDirty ? `${t.title}*` : t.title,
    path: t.type === 'gallery' ? '/gallery' : t.type === 'object' ? '/object' : t.type === 'objects-overview' ? '/objects' : '/query'
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
