import { useEffect, useRef, useState, type FormEvent } from "react";
import "../styles/bars-style.css";
import "../styles/forms-style.css";
import { useConnectionStore } from "../data/global-states";
import { sha256HexDigest } from "../data/util";

function ConnectionDialog() {
  // const [isConnected, setIsConnected] = useState(false);
  const [errorMessage, setErrorMessage] = useState("No error");
  const [isLoading, setIsLoading] = useState(false);
  const { ConnectionId, setId, addInfo } = useConnectionStore();

  const dialogRef = useRef<HTMLDialogElement>(null);

  useEffect(() => {
    if (ConnectionId === undefined) {
      dialogRef.current?.showModal();
    }
  }, [ConnectionId]);

  async function handleSubmit(event: FormEvent) {
    event.preventDefault();

    const formData = new FormData(event.currentTarget as HTMLFormElement);
    const hostname = formData.get("hostname");
    const database = formData.get("database");
    const user = formData.get("user");
    const password = formData.get("password");

    setIsLoading(true);

    if (!hostname || !database || !user || !password) {
      await new Promise((resolve) => setTimeout(resolve, 100));
      setIsLoading(false);
      setErrorMessage("Please fill out all the fields!");
      return;
    }

    try {
      setIsLoading(true);
      const resp = await fetch(`${hostname!.toString()}/connect`, {
        method: "POST",
        body: JSON.stringify({
          db_name: database.toString(),
          username: user.toString(),
          // FIXME(liza): Replace by hex encoding of SHA256 hash of the password.
          password_hash: await sha256HexDigest(password.toString()),
        }),
      });

      await new Promise((resolve) => setTimeout(resolve, 100));

      if (resp.status == 200) {
        const body = await resp.text();
        const connectionId = parseInt(body);
        setErrorMessage("No error");
        addInfo(hostname.toString(), database.toString(), user.toString());
        setId(connectionId);
        dialogRef.current?.close();
      } else {
        const errorReason = await resp.text();
        console.error("Status: %s, reason: %s", resp.statusText, errorReason);
        setErrorMessage("Wrong credentials! Try again!");
      }
    } catch {
      setErrorMessage("Could not connect to the server. Try again!");
    } finally {
      setIsLoading(false);
    }
  }

  return (
    <dialog ref={dialogRef}>
      <h1>Connect to the Server</h1>
      <form autoComplete="off" onSubmit={handleSubmit}>
        <div className="vertical-container">
          <div className="horizontal-container">
            <label htmlFor="hostname">Server URL:</label>
            <input
              name="hostname"
              type="text"
              placeholder="Server URL..."
            ></input>
            <div className="input-hint-container">
              <img
                src="src/assets/icons/question.png"
                alt="hint"
                className="input-hint-icon"
              />
              <div className="input-hint-tooltip">
                Right format: http&#91;s&#93;://&lt;hostname or IP&gt;:&lt;port&gt; 
              </div>
            </div>
          </div>
          <div className="horizontal-container">
            <label htmlFor="database">Database name:</label>
            <input
              name="database"
              type="text"
              placeholder="Database name..."
            ></input>
          </div>
          <div className="horizontal-container">
            <label htmlFor="user">Username:</label>
            <input name="user" type="text" placeholder="Username..."></input>
          </div>
          <div className="horizontal-container">
            <label htmlFor="password">Password:</label>
            <input
              name="password"
              type="password"
              placeholder="Password..."
            ></input>
          </div>
          <p
            className={
              errorMessage === "No error"
                ? "error-message-hidden"
                : "error-message-dialog"
            }
          >
            {errorMessage}
          </p>
          <button className="connect-button">
            {isLoading ? (
              <img src="src/assets/loading.svg" alt="loading..."></img>
            ) : (
              <span>Connect</span>
            )}
          </button>
        </div>
      </form>
    </dialog>
  );
}

export default ConnectionDialog;
