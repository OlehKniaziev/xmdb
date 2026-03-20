import { BrowserRouter as Router, Routes, Route } from "react-router-dom";
import MainLayout from "./layouts/MainLayout";
import QueryEditor from "./components/QueryEditor";
import ConnectionDialog from "./components/ConnectionDialog";
import "./styles/index.css";
import HomePage from "./components/HomePage";
import GalleryView from "./components/GalleryView";
import { useMultiTabQueryStore } from "./data/global-states";

function GalleryViewWrapper() {
  const { tabs, activeTabId } = useMultiTabQueryStore();
  const activeTab = tabs.find((t) => t.id === activeTabId);

  if (!activeTab || activeTab.type !== "gallery" || !activeTab.response || !activeTab.galleryInfo) {
    return <div className="messages-style">No active gallery tab</div>;
  }

  return (
    <div style={{ height: "100%", overflowY: "auto" }}>
      <GalleryView
        response={activeTab.response}
        columnName={activeTab.galleryInfo.columnName}
        query={activeTab.query}
      />
    </div>
  );
}

function App() {
  return (
    <>
      <ConnectionDialog />
      <Router>
        <Routes>
          <Route path="/" element={<MainLayout />}>
            <Route index element={<HomePage />} />
            <Route path="query" element={<QueryEditor />} />
            <Route path="gallery" element={<GalleryViewWrapper />} />
          </Route>
        </Routes>
      </Router>
    </>
  );
}

export default App;
