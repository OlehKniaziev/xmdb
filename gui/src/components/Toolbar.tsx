import { useEffect, useRef } from "react";
import { useLocation, useNavigate } from "react-router-dom";
import ConnectionEdit, { type ConnectionEditHandle } from "./ConnectionEdit";
import {
  useConnectionStore,
  useOutputMessagesStore,
  useQueryResponceStore,
  useQueryStore,
} from "../data/global-states";
import type { QueryResponse } from "../data/query-response";

export default function Toolbar() {
  const location = useLocation();
  const navigate = useNavigate();

  const { query, setQuery } = useQueryStore();
  const { setMessage } = useOutputMessagesStore();
  const { ConnectionId, Hostname, Database, Username } = useConnectionStore();
  const { setQueryResponce } = useQueryResponceStore();

  const dialogRef = useRef<ConnectionEditHandle>(null);

  function newQueryHomeOnClick() {
    navigate("/query");
    setQuery("-- start writing your code here");
    setMessage("Output messages will be shown here");
  }

  function editConnectionOnClick() {
    dialogRef.current?.open();
  }

  async function executeQuery() {
    // console.log(query);
    if (ConnectionId && Hostname && Database && Username) {
      try {
        const resp = await fetch(`${Hostname!.toString()}/run-query`, {
          method: "POST",
          body: JSON.stringify({
            query,
            connection_id: ConnectionId,
          }),
        });

        if (resp.status == 200) {
          const body = await resp.json();
          const q = body as QueryResponse;
          if(q.ok === true) {
            setMessage("Query executed successfully.");
            console.log(q);
            setQueryResponce(q);
          } else {
            setQueryResponce(q);
            setMessage(`Error: ${q.error_message}`);
          }
        } else {
          setQueryResponce();
          setMessage("Could not execute query. Network error.");
        }
      } catch (e: any) {
        console.error(e);
        setQueryResponce();
        setMessage(`${e.name}: ${e.message}`);
      }
    }
  }

  function newQueryOnClick() {
    setQuery("-- start writing your code here");
    setMessage("Output messages will be shown here");
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

        {/* <div className="btn-label-container">
          <button
            name="openQueryBtn"
            className="toolbar-btn"
            data-src="src/assets/icons/open-file.png"
            data-hover="src/assets/icons/open-file-dark-outline.png"
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
          >
            <img
              alt="Save query icon"
              src="src/assets/icons/diskette.png"
            ></img>
          </button>
          <label htmlFor="saveQueryBtn">Save</label>
        </div> */}
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
        {/* <div className="btn-label-container">
          <button
            name="insertMediaBtn"
            className="toolbar-btn"
            data-src="src/assets/icons/paperclip.png"
            data-hover="src/assets/icons/paperclip-dark.png"
          >
            <img
              alt="Insert media icon"
              src="src/assets/icons/paperclip.png"
            ></img>
          </button>
          <label htmlFor="insertMediaBtn">Insert media</label>
        </div> */}
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
