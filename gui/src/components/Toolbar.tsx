import { useLocation } from "react-router-dom";

export default function Toolbar() {
  const location = useLocation();

  let buttons;
  if (location.pathname === "/query") {
    buttons = (
      <>
        <div className="btn-label-container">
          <button name="newQueryBtn" className="toolbar-btn">
            <img
              alt="New query icon"
              src="src/assets/icons/sql-server.png"
            ></img>
          </button>
          <label htmlFor="newQueryBtn">New Query</label>
        </div>
        <div className="btn-label-container">
          <button name="openQueryBtn" className="toolbar-btn">
            <img
              alt="Open query icon"
              src="src/assets/icons/open-file.png"
            ></img>
          </button>
          <label htmlFor="openQueryBtn">Open Query</label>
        </div>

        <div className="btn-label-container">
          <button name="saveQueryBtn" className="toolbar-btn">
            <img
              alt="Save query icon"
              src="src/assets/icons/diskette.png"
            ></img>
          </button>
          <label htmlFor="saveQueryBtn">Save</label>
        </div>
        <div className="btn-label-container">
          <button name="executeQueryBtn" className="toolbar-btn">
            <img
              alt="Execute query icon"
              src="src/assets/icons/play-button.png"
            ></img>
          </button>
          <label htmlFor="executeQueryBtn">Execute</label>
        </div>
        <div className="btn-label-container">
          <button name="insertMediaBtn" className="toolbar-btn">
            <img alt="Insert media icon" src="src/assets/icons/tag.png"></img>
          </button>
          <label htmlFor="insertMediaBtn">Insert media</label>
        </div>
      </>
    );
  } else if (location.pathname === "/settings") {
    buttons = <button className="toolbar-btn">Save Settings</button>;
  } else {
    buttons = (
      <>
        <div className="btn-label-container">
          <button name="connectionBtn" className="toolbar-btn">
            <img alt="Connection icon" src="src/assets/icons/connect.png"></img>
          </button>
          <label htmlFor="connectionBtn">Connection</label>
        </div>
        <div className="btn-label-container">
          <button name="newQueryBtnHomePage" className="toolbar-btn">
            <img alt="New query icon" src="src/assets/icons/sql-server.png"></img>
          </button>
          <label htmlFor="newQueryBtnHomePage">New Query</label>
        </div>
      </>
    );
  }

  return (
    <div className="toolbar">{buttons}</div>
  );
}
