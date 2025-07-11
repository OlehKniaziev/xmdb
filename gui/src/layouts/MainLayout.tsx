import Sidebar from "../components/Sidebar";
import Topbar from "../components/Topbar";
import Toolbar from "../components/Toolbar";
import { Outlet } from "react-router-dom";
import { useState } from "react";
import TabBar from "../components/TabBar";

export default function MainLayout() {
  const [activeView, setActiveView] = useState(null);
  return (
    <div id="main" className="flex">
      <div className="topbar">
        <Topbar />
        <Toolbar />
      </div>
      <div className="main-content">
        <Sidebar />
        <div className="content-container">
          <TabBar activeView={activeView} />
          <div className="content-style">
            <Outlet />
          </div>
        </div>
      </div>
    </div>
  );
}
