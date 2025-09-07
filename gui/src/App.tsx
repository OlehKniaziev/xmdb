import { BrowserRouter as Router, Routes, Route } from "react-router-dom";
import MainLayout from "./layouts/MainLayout";
import QueryEditor from "./components/QueryEditor";
import ConnectionDialog from "./components/ConnectionDialog";
import "./styles/index.css";
import HomePage from "./components/HomePage";
import { loadWasmModule } from "./sql/sql";
import { useEffect } from "react";

function App() {
  useEffect(
    () => {
      (async () => await loadWasmModule())()
    }
  );

  return (
    <>
      <ConnectionDialog />
      <Router>
        <Routes>
          <Route path="/" element={<MainLayout />}>
            <Route index element={<HomePage />} />
            <Route path="query" element={<QueryEditor />} />
          </Route>
        </Routes>
      </Router>
    </>
  );
}

export default App;
