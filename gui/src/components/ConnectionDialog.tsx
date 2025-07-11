import { useEffect, useRef, useState } from "react";
import "../styles/bars-style.css";
import "../styles/forms-style.css";

function ConnectionDialog() {
  const [isConnected, setIsConnected] = useState(false);

  const dialogRef = useRef<HTMLDialogElement>(null);
  useEffect(() => {
    dialogRef.current?.showModal();
  }, []);

  return (
    <dialog ref={dialogRef}>
      <h1>Connect to the Server</h1>
      <form autoComplete="off" onSubmit={event => {event.preventDefault()}}> 
        <div className="vertical-container">
          <div className="horizontal-container">
            <label htmlFor="hostname">Hostname / IP:</label>
            <input name="hostname" type="text"></input>
          </div>
          <div className="horizontal-container">
            <label htmlFor="user">User:</label>
            <input name="user" type="text"></input>
          </div>
          <div className="horizontal-container">
            <label htmlFor="password">Password:</label>
            <input name="password" type="password"></input>
          </div>
          <button onClick={() => dialogRef.current?.close()}>Connect</button>
        </div>
      </form>
    </dialog>
  );
}

export default ConnectionDialog;
