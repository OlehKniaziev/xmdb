import { useEffect, useRef, useState, type FormEvent } from "react";
import "../styles/bars-style.css";
import "../styles/forms-style.css";
import { useConnectionStore } from "../data/connection-data";

function ConnectionDialog() {
  // const [isConnected, setIsConnected] = useState(false);
  const [errorMessage, setErrorMessage] = useState("No error");
  const { setId, addInfo } = useConnectionStore();

  const dialogRef = useRef<HTMLDialogElement>(null);
  useEffect(() => {
    dialogRef.current?.showModal();
  }, []);

  async function handleSubmit(event: FormEvent) {
    event.preventDefault();

    const formData = new FormData(event.currentTarget as HTMLFormElement);
    const hostname = formData.get("hostname");
    const database = formData.get("database");
    const user = formData.get("user");
    const password = formData.get("password");

    if (!hostname || !database || !user || !password) {
      setErrorMessage("Please fill out all the fields!");
      return;
    }

    try {
      const resp = await fetch(hostname!.toString(), {
        method: "POST",
        body: JSON.stringify({
          db_name: database.toString(),
          username: user.toString(),
          // FIXME(liza): Replace by base64 encoding of SHA256 hash of the password.
          password_hash: password.toString(),
        }),
      });

      if (resp.status == 200) {
        const body = await resp.text();
        const connectionId = parseInt(body);
        setErrorMessage("No error");
        addInfo(hostname.toString(), database.toString(), user.toString());
        setId(connectionId);
        dialogRef.current?.close();
      } else {
        setErrorMessage("Wrong credentials! Try again!");
      }
    } catch (e: any) {
      setErrorMessage("Could not connect to the server. Try again!");
    }
  }

  return (
    <dialog ref={dialogRef}>
      <h1>Connect to the Server</h1>
      <form autoComplete="off" onSubmit={handleSubmit}>
        <div className="vertical-container">
          <div className="horizontal-container">
            <label htmlFor="hostname">Hostname \ IP:</label>
            <input
              name="hostname"
              type="text"
              placeholder="Hostname \ IP..."
            ></input>
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
              errorMessage === "No error" ? "error-message-hidden" : "error-message-dialog"
            }
          >
            {errorMessage}
          </p>
          <button>Connect</button>
        </div>
      </form>
    </dialog>
  );
}

export default ConnectionDialog;
