import { useEffect, useRef, useState } from "react";
import { useLocation, useNavigate } from "react-router-dom";
import ConnectionEdit, { type ConnectionEditHandle } from "./ConnectionEdit";
import ToolbarButton from "./ToolbarButton";
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
  const helpRef = useRef<HTMLDivElement>(null);
  const [helpOpen, setHelpOpen] = useState(false);

  useEffect(() => {
    function handleClickOutside(e: MouseEvent) {
      if (helpRef.current && !helpRef.current.contains(e.target as Node)) {
        setHelpOpen(false);
      }
    }
    document.addEventListener("mousedown", handleClickOutside);
    return () => document.removeEventListener("mousedown", handleClickOutside);
  }, []);

  const helpButton = (
    <div className="help-btn-container" ref={helpRef}>
      <ToolbarButton
        name="helpBtn"
        label="Help"
        iconSrc="src/assets/icons/question-mark.png"
        hoverIconSrc="src/assets/icons/question-mark-dark.png"
        iconAlt="Help icon"
        onClick={() => setHelpOpen((o) => !o)}
      />
      {helpOpen && (
        <div className="help-dropdown">
          {location.pathname === "/query" ? (
            <>You can find SQL Syntax here: <a href="https://olehkniaziev.github.io/xmdb/sql.html" target="_blank" rel="noopener noreferrer" style={{color: 'var(--color-accent-bright)'}}>SQL Syntax</a></>
          ) : (
            <>You can find user help here: <a href="https://olehkniaziev.github.io/xmdb/gui.html" target="_blank" rel="noopener noreferrer" style={{color: 'var(--color-accent-bright)'}}>User Help</a></>
          )}
        </div>
      )}
    </div>
  );

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
      } catch (e: unknown) {
        const message = e instanceof Error ? `${e.name}: ${e.message}` : String(e);
        console.log(
          "Failed to execute query. Status code: %s (%s)",
          resp.status,
          resp.statusText,
        );
        updateTabResults(tabId, message, undefined, false);
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

  if (location.pathname === "/objects" || location.pathname === "/object") {
    return <div className="toolbar">{helpButton}</div>;
  }

  if (location.pathname === "/gallery") {
    return (
      <div className="toolbar">
        <ToolbarButton
          name="refreshGalleryBtn"
          label="Refresh"
          iconSrc="src/assets/icons/refresh.png"
          hoverIconSrc="src/assets/icons/refresh-dark-outline.png"
          iconAlt="Refresh gallery icon"
          onClick={async () => {
            if (activeTab && activeTab.type === "gallery") {
              await runQuery(activeTab.id, activeTab.query);
            }
          }}
        />
        {helpButton}
      </div>
    );
  }

  let buttons;
  if (location.pathname === "/query") {
    buttons = (
      <>
        <ToolbarButton
          name="newQueryBtn"
          label="New Query"
          iconSrc="src/assets/icons/sql-server.png"
          hoverIconSrc="src/assets/icons/sql-server-dark-outline.png"
          iconAlt="New query icon"
          onClick={newQueryOnClick}
        />

        <ToolbarButton
          name="openQueryBtn"
          label="Open Query"
          iconSrc="src/assets/icons/open-file.png"
          hoverIconSrc="src/assets/icons/open-file-dark-outline.png"
          iconAlt="Open query icon"
          onClick={openQueryOnClick}
        />

        <ToolbarButton
          name="saveQueryBtn"
          label="Save Query"
          iconSrc="src/assets/icons/diskette.png"
          hoverIconSrc="src/assets/icons/diskette-dark-outline.png"
          iconAlt="Save query icon"
          onClick={saveQueryOnClick}
        />

        <ToolbarButton
          name="executeQueryBtn"
          label="Execute"
          iconSrc="src/assets/icons/play-button.png"
          hoverIconSrc="src/assets/icons/play-button-dark-outline.png"
          iconAlt="Execute query icon"
          onClick={executeQuery}
        />

        <ToolbarButton
          name="insertBtn"
          label="Insert Media"
          iconSrc="src/assets/icons/paperclip.png"
          hoverIconSrc="src/assets/icons/paperclip-dark.png"
          iconAlt="Insert media icon"
          onClick={insertMedia}
        />
      </>
    );
  } else if (location.pathname === "/settings") {
    buttons = (
      <ToolbarButton name="saveSettingsBtn" label="Save Settings">
        Save Settings
      </ToolbarButton>
    );
  } else {
    buttons = (
      <>
        <ToolbarButton
          name="connectionBtn"
          label="Connection"
          iconSrc="src/assets/icons/connect.png"
          hoverIconSrc="src/assets/icons/connect-dark-outline.png"
          iconAlt="Connection icon"
          onClick={editConnectionOnClick}
        />

        <ToolbarButton
          name="openQueryBtnHome"
          label="Open Query"
          iconSrc="src/assets/icons/open-file.png"
          hoverIconSrc="src/assets/icons/open-file-dark-outline.png"
          iconAlt="Open query icon"
          onClick={openQueryOnClick}
        />

        <ToolbarButton
          name="newQueryBtnHomePage"
          label="New Query"
          iconSrc="src/assets/icons/sql-server.png"
          hoverIconSrc="src/assets/icons/sql-server-dark-outline.png"
          iconAlt="New query icon"
          onClick={newQueryHomeOnClick}
        />
      </>
    );
  }

  return (
    <>
      <div className="toolbar">{buttons}{helpButton}</div>
      <ConnectionEdit ref={dialogRef} />
    </>
  );
}
