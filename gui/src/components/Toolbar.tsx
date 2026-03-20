import { use, useEffect, useRef } from "react";
import { useLocation, useNavigate } from "react-router-dom";
import ConnectionEdit, { type ConnectionEditHandle } from "./ConnectionEdit";
import {
  useConnectionStore,
  useMultiTabQueryStore,
} from "../data/global-states";
import type { QueryResponse } from "../data/query-response";
import { useMonaco } from "@monaco-editor/react";
import { fileToHexDataString } from "../data/util";

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
  const { ConnectionId, Hostname, Database, Username } = useConnectionStore();

  const activeTab = tabs.find((t) => t.id === activeTabId);

  const dialogRef = useRef<ConnectionEditHandle>(null);

  async function runQuery(tabId: string, queryToExecute: string) {
    if (ConnectionId && Hostname && Database && Username) {
      try {
        const tab = tabs.find((t) => t.id === tabId);
        if (!tab) return;

        updateTabResults(tabId, tab.message, tab.response, true);
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
          if (q.ok === true) {
            updateTabResults(tabId, "Query executed successfully.", q, false);
          } else {
            updateTabResults(tabId, `Error: ${q.error_message}`, q, false);
          }
        } else {
          updateTabResults(
            tabId,
            "Could not execute query. Network error.",
            undefined,
            false,
          );
        }
      } catch (e: any) {
        console.error(e);
        updateTabResults(tabId, `${e.name}: ${e.message}`, undefined, false);
      }
    }
  }

  async function loadFile({
    description,
    accept,
    multiple,
  }: {
    description: string;
    accept: { [mimeType: string]: string[] };
    multiple: boolean;
  }): Promise<
    | {
        content: string;
        fileName: string;
        fileHandle: FileSystemFileHandle;
      }
    | undefined
  > {
    try {
      // @ts-expect-error - File System Access API
      const [handle] = await window.showOpenFilePicker({
        types: [
          {
            description,
            accept,
          },
        ],
        multiple,
      });

      if (handle) {
        const file = await handle.getFile();
        const content = await file.text();
        return {
          content,
          fileName: file.name,
          fileHandle: handle,
        };
      }
    } catch (err: unknown) {
      console.error("Failed to open file: %s", err);
    }
  }

  async function openQueryOnClick() {
    const result = await loadFile({
      description: "SQL Files",
      accept: { "text/plain": [".sql"] },
      multiple: false,
    });

    if (!result) return;

    const { content, fileName, fileHandle } = result;

    openTab(fileName, content, fileHandle);
    navigate("/query");
  }

  async function saveQueryOnClick() {
    if (!activeTab) return;

    try {
      let handle = activeTab.fileHandle;

      if (!handle) {
        // @ts-expect-error - File System Access API
        handle = await window.showSaveFilePicker({
          suggestedName: activeTab.title.endsWith(".sql")
            ? activeTab.title.replace(/\*$/, "")
            : `${activeTab.title.replace(/\*$/, "")}.sql`,
          types: [
            {
              description: "SQL Files",
              accept: { "text/plain": [".sql"] },
            },
          ],
        });
      }

      if (handle) {
        const writable = await handle.createWritable();
        await writable.write(activeTab.query);
        await writable.close();

        const file = await handle.getFile();
        updateTabSaveStatus(activeTab.id, false, file.name, handle);
      }
    } catch (err: unknown) {
      if (err instanceof Error && err.name !== "AbortError") {
        console.error("Failed to save file:", err);
        alert("Failed to save file: " + err.message);
      }
    }
  }

  const monaco = useMonaco();

  async function insertMedia() {
    if (!monaco) return;

    const result = await loadFile({
      description: "Image Files",
      accept: { "image/*": [".png", ".jpg", ".jpeg", ".gif", ".bmp"] },
      multiple: false,
    });

    if (!result) return;

    const { fileHandle } = result;

    const file = await fileHandle.getFile();
    const hexData = await fileToHexDataString(file);

    const editor = monaco.editor.getEditors()[0];
    editor.executeEdits("insert-media", [
      {
        range: editor.getSelection()!,
        text: `RGB(${hexData.width}, ${hexData.height}, "${hexData.data}")`,
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
    window.location.reload();
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
