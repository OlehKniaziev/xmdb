import { use, useEffect, useRef } from "react";
import { useLocation, useNavigate } from "react-router-dom";
import ConnectionEdit, { type ConnectionEditHandle } from "./ConnectionEdit";
import {
  useConnectionStore,
  useMultiTabQueryStore,
} from "../data/global-states";
import type { QueryResponse } from "../data/query-response";
import { useMonaco } from "@monaco-editor/react";
import { fileToHexDataString, pickFile, saveFile } from "../data/util";

interface ToolbarProps {
  onAddTab?: (path: string) => void;
}

export default function Toolbar({ onAddTab }: ToolbarProps) {
  const location = useLocation();
  const navigate = useNavigate();

  const {
    tabs,
    activeTabId,
    addTab,
    openTab,
    updateTabResults,
    updateTabSaveStatus,
  } = useMultiTabQueryStore();
  const { ConnectionId, Hostname, Database, Username, bumpDbObjectsVersion } =
    useConnectionStore();

  const activeTab = tabs.find((t) => t.id === activeTabId);

  const dialogRef = useRef<ConnectionEditHandle>(null);

  async function runQuery(tabId: string, queryToExecute: string) {
    let resp: Response = new Response();

    if (ConnectionId && Hostname && Database && Username) {
      try {
        const tab = tabs.find((t) => t.id === tabId);
        if (!tab) return;

        updateTabResults(tabId, tab.message, tab.response, true);
        resp = await fetch(`${Hostname!.toString()}/run-query`, {
          method: "POST",
          body: JSON.stringify({
            query: queryToExecute,
            connection_id: ConnectionId,
          }),
        });

        if (resp.status == 200) {
          const body = await resp.json();
          const q = body as QueryResponse;
          if (q.ok === true) {
            updateTabResults(tabId, "Query executed successfully.", q, false);
          } else {
            updateTabResults(tabId, `Error: ${q.error_message}`, q, false);
          }
          bumpDbObjectsVersion();
        } else {
          updateTabResults(
            tabId,
            "Could not execute query. Network error.",
            undefined,
            false,
          );
        }
      } catch (e: any) {
        console.log(
          "Failed to execute query. Status code: %s (%s)",
          resp.status,
          resp.statusText,
        );
        updateTabResults(tabId, `${e.name}: ${e.message}`, undefined, false);
      }
    }
  }

  async function openQueryOnClick() {
    const file = await pickFile(".sql");
    if (!file) return;

    const content = await file.text();
    openTab(file.name, content);
    navigate("/query");
  }

  function saveQueryOnClick() {
    if (!activeTab) return;

    const fileName = activeTab.title.endsWith(".sql")
      ? activeTab.title.replace(/\*$/, "")
      : `${activeTab.title.replace(/\*$/, "")}.sql`;

    saveFile(activeTab.query, fileName);
    updateTabSaveStatus(activeTab.id, false, fileName);
  }

  const monaco = useMonaco();

  async function insertMedia() {
    if (!monaco) return;

    const file = await pickFile("image/png,image/jpeg,image/gif,image/bmp");
    if (!file) return;

    const hexData = await fileToHexDataString(file);

    const editor = monaco.editor.getEditors()[0];
    editor.executeEdits("insert-media", [
      {
        range: editor.getSelection()!,
        text: `RGB(${hexData.width}, ${hexData.height},\n"${hexData.data}"\n)`,
      },
    ]);
    if (!editor) return;
  }

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
    if (!activeTab) return;
    await runQuery(activeTab.id, activeTab.query);
  }

  function newQueryOnClick() {
    const sqlTabs = tabs.filter((tab) => tab.title.startsWith("SQL Query"));
    const newTitle =
      sqlTabs.length === 0 ? "SQL Query" : `SQL Query ${sqlTabs.length}`;
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

  if (location.pathname === "/objects" || location.pathname === "/object") {
    return <div className="toolbar"></div>;
  }

  if (location.pathname === "/gallery") {
    return (
      <div className="toolbar">
        <div className="btn-label-container">
          <button
            name="refreshGalleryBtn"
            className="toolbar-btn"
            data-src="src/assets/icons/refresh.png"
            data-hover="src/assets/icons/refresh-dark-outline.png"
            onClick={async () => {
              if (activeTab && activeTab.type === "gallery") {
                await runQuery(activeTab.id, activeTab.query);
              }
            }}
          >
            <img
              alt="Refresh gallery icon"
              src="src/assets/icons/refresh.png"
            ></img>
          </button>
          <label htmlFor="refreshGalleryBtn">Refresh</label>
        </div>
      </div>
    );
  }

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
            name="openQueryBtn"
            className="toolbar-btn"
            data-src="src/assets/icons/open-file.png"
            data-hover="src/assets/icons/open-file-dark-outline.png"
            onClick={openQueryOnClick}
          >
            <img
              alt="Open query icon"
              src="src/assets/icons/open-file.png"
            ></img>
          </button>
          <label htmlFor="openQueryBtn">Open Query</label>
        </div>

        <div className="btn-label-container">
          <button
            name="saveQueryBtn"
            className="toolbar-btn"
            data-src="src/assets/icons/diskette.png"
            data-hover="src/assets/icons/diskette-dark-outline.png"
            onClick={saveQueryOnClick}
          >
            <img
              alt="Save query icon"
              src="src/assets/icons/diskette.png"
            ></img>
          </button>
          <label htmlFor="saveQueryBtn">Save Query</label>
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

        <div className="btn-label-container">
          <button
            name="insertBtn"
            className="toolbar-btn"
            data-src="src/assets/icons/paperclip.png"
            data-hover="src/assets/icons/paperclip-dark.png"
            onClick={insertMedia}
          >
            <img
              alt="Insert media icon"
              src="src/assets/icons/paperclip.png"
            ></img>
          </button>
          <label htmlFor="insertBtn">Insert Media</label>
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
            name="openQueryBtnHome"
            className="toolbar-btn"
            data-src="src/assets/icons/open-file.png"
            data-hover="src/assets/icons/open-file-dark-outline.png"
            onClick={openQueryOnClick}
          >
            <img
              alt="Open query icon"
              src="src/assets/icons/open-file.png"
            ></img>
          </button>
          <label htmlFor="openQueryBtnHome">Open Query</label>
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
