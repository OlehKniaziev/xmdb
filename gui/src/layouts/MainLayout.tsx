import Sidebar from "../components/Sidebar";
import Topbar from "../components/Topbar";
import Toolbar from "../components/Toolbar";
import { Outlet } from "react-router-dom";
import { useState } from "react";
import TabBar from "../components/TabBar";
import { Panel, PanelGroup, PanelResizeHandle } from "react-resizable-panels";

export default function MainLayout() {
  const [activeView, setActiveView] = useState(null);
  return (
    <div id="main" className="flex">
      <div className="topbar">
        <Topbar />
        <Toolbar />
      </div>
      <div className="main-content">
        <PanelGroup direction="horizontal">
          <Panel minSize={1} maxSize={20} defaultSize={16}>
            <Sidebar />
          </Panel>
          <PanelResizeHandle />
          <Panel>
            <div className="content-container">
              <TabBar activeView={activeView} />
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
