import { useEffect, useRef } from "react";
import { useLocation, useNavigate } from "react-router-dom";
import ConnectionEdit, { type ConnectionEditHandle } from "./ConnectionEdit";
import {
  useConnectionStore,
  useMultiTabQueryStore,
} from "../data/global-states";
import type { QueryResponse } from "../data/query-response";

interface ToolbarProps {
  onAddTab?: (path: string) => void;
}

export default function Toolbar({ onAddTab }: ToolbarProps) {
  const location = useLocation();
  const navigate = useNavigate();

  const { tabs, activeTabId, addTab, updateTabResults } = useMultiTabQueryStore();
  const { ConnectionId, Hostname, Database, Username } = useConnectionStore();

  const activeTab = tabs.find(t => t.id === activeTabId);

  const dialogRef = useRef<ConnectionEditHandle>(null);

  function newQueryHomeOnClick() {
    if (onAddTab) {
      onAddTab("/query");
    } else {
      addTab("SQL Query");
      navigate("/query");
    }
  }

  function editConnectionOnClick() {
    dialogRef.current?.open();
  }

  async function executeQuery() {
    console.log(activeTab);
    if (!activeTab) return;

    const tabId = activeTab.id;
    const queryToExecute = activeTab.query;

    console.log({ConnectionId, Hostname, Database, Username, queryToExecute});

      try {
        updateTabResults(tabId, activeTab.message, activeTab.response, true);
        const resp = await fetch(`${Hostname!.toString()}/run-query`, {
          method: "POST",
          body: JSON.stringify({
            query: queryToExecute,
            connection_id: ConnectionId,
          }),
        });

        if (resp.status == 200) {
          const body = await resp.json();
          const q = body as QueryResponse;
          if(q.ok === true) {
            updateTabResults(tabId, "Query executed successfully.", q, false);
          } else {
            updateTabResults(tabId, `Error: ${q.error_message}`, q, false);
          }
        } else {
          updateTabResults(tabId, "Could not execute query. Network error.", undefined, false);
        }
      } catch (e: unknown) {
        console.error(e);
        updateTabResults(tabId, `${(e as Error).name}: ${(e as Error).message}`, undefined, false);
      }
  }

  function newQueryOnClick() {
    const sqlTabs = tabs.filter(tab => tab.title.startsWith('SQL Query'));
    const newTitle = sqlTabs.length === 0 ? 'SQL Query' : `SQL Query ${sqlTabs.length}`;
    addTab(newTitle);
  }

  useEffect(() => {
    const buttons = document.querySelectorAll<HTMLElement>(".toolbar-btn");

    buttons.forEach((button) => {
      const img = button.querySelector("img");
      const defaultSrc = button.dataset.src;
      const hoverSrc = button.dataset.hover;

      if (!img || !defaultSrc || !hoverSrc) return;

      button.addEventListener("mouseenter", () => {
        img.src = hoverSrc!;
      });

      button.addEventListener("mouseleave", () => {
        img.src = defaultSrc!;
      });
    });
  }, [location]);

  let buttons;
  if (location.pathname === "/query") {
    buttons = (
      <>
        <div className="btn-label-container">
          <button
            name="newQueryBtn"
            className="toolbar-btn"
            data-src="src/assets/icons/sql-server.png"
            data-hover="src/assets/icons/sql-server-dark-outline.png"
            onClick={newQueryOnClick}
          >
            <img
              alt="New query icon"
              src="src/assets/icons/sql-server.png"
            ></img>
          </button>
          <label htmlFor="newQueryBtn">New Query</label>
        </div>

        <div className="btn-label-container">
          <button
            name="executeQueryBtn"
            className="toolbar-btn"
            data-src="src/assets/icons/play-button.png"
            data-hover="src/assets/icons/play-button-dark-outline.png"
            onClick={executeQuery}
          >
            <img
              alt="Execute query icon"
              src="src/assets/icons/play-button.png"
            ></img>
          </button>
          <label htmlFor="executeQueryBtn">Execute</label>
        </div>
      </>
    );
  } else if (location.pathname === "/settings") {
    buttons = <button className="toolbar-btn">Save Settings</button>;
  } else {
    buttons = (
      <>
        <div className="btn-label-container">
          <button
            name="connectionBtn"
            className="toolbar-btn"
            onClick={editConnectionOnClick}
            data-src="src/assets/icons/connect.png"
            data-hover="src/assets/icons/connect-dark-outline.png"
          >
            <img alt="Connection icon" src="src/assets/icons/connect.png"></img>
          </button>
          <label htmlFor="connectionBtn">Connection</label>
        </div>
        <div className="btn-label-container">
          <button
            name="newQueryBtnHomePage"
            className="toolbar-btn"
            onClick={newQueryHomeOnClick}
            data-src="src/assets/icons/sql-server.png"
            data-hover="src/assets/icons/sql-server-dark-outline.png"
          >
            <img
              alt="New query icon"
              src="src/assets/icons/sql-server.png"
            ></img>
          </button>
          <label htmlFor="newQueryBtnHomePage">New Query</label>
        </div>
      </>
    );
  }

  return (
    <>
      <div className="toolbar">{buttons}</div>
      <ConnectionEdit ref={dialogRef} />
    </>
  );
}
