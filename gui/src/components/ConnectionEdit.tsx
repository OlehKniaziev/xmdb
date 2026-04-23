import {
  forwardRef,
  useImperativeHandle,
  useRef,
  useState,
  type SyntheticEvent,
} from "react";
import "../styles/bars-style.css";
import "../styles/forms-style.css";
import { useConnectionStore } from "../data/global-states";
import { sha256HexDigest } from "../data/util";
import LoadingButton from "./LoadingButton";

export type ConnectionEditHandle = {
  open: () => void;
};

const ConnectionEdit = forwardRef<ConnectionEditHandle>((_, ref) => {
  // const [isConnected, setIsConnected] = useState(false);
  const {
    Hostname,
    Database,
    Username,
    setId,
    addInfo,
    disconnect,
  } = useConnectionStore();

  const [errorMessage, setErrorMessage] = useState("No error");
  const [isLoadingConnect, setIsLoadingConnect] = useState(false);
  const [isLoadingDisconnect, setIsLoadingDisconnect] = useState(false);

  const dialogRef = useRef<HTMLDialogElement>(null);

  useImperativeHandle(ref, () => ({
    open: () => {
      if (!dialogRef.current?.open) {
        dialogRef.current?.showModal();
      }
    },
  }));

  async function handleSubmit(
    event: SyntheticEvent<HTMLFormElement, SubmitEvent>
  ) {
    event.preventDefault();

    const submitter = event.nativeEvent.submitter;

    if (submitter?.innerText === "Reconnect") {
      const formData = new FormData(event.currentTarget as HTMLFormElement);
      const hostname = formData.get("hostname");
      const database = formData.get("database");
      const user = formData.get("user");
      const password = formData.get("password");

      setIsLoadingConnect(true);

      if (!hostname || !database || !user || !password) {
        await new Promise((resolve) => setTimeout(resolve, 100));
        setIsLoadingConnect(false);
        setErrorMessage("Please fill out all the fields!");
        return;
      }

      try {
        setIsLoadingConnect(true);
        const resp = await fetch(`${hostname!.toString()}/connect`, {
          method: "POST",
          body: JSON.stringify({
            db_name: database.toString(),
            username: user.toString(),
            password_hash: await sha256HexDigest(password.toString()),
          }),
        });

        await new Promise((resolve) => setTimeout(resolve, 100));
        if (resp.status === 200) {
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
        setIsLoadingConnect(false);
      }
    }

    if (submitter?.innerText === "Disconnect") {
      setIsLoadingDisconnect(true);
      await new Promise((resolve) => setTimeout(resolve, 100));
      disconnect();
      setIsLoadingDisconnect(false);
      dialogRef.current?.close();
    }
  }

  return (
    <dialog ref={dialogRef}>
      <button
        className="close-button-dialog"
        onClick={() => dialogRef.current?.close()}
      >
        <img src="src/assets/icons/close-icon.svg"></img>
      </button>
      <h1>Edit connection</h1>
      <form autoComplete="off" onSubmit={handleSubmit}>
        <div className="vertical-container">
          <div className="horizontal-container">
            <label htmlFor="hostname">Hostname:</label>
            <input name="hostname" type="text" defaultValue={Hostname}></input>
            <div className="input-hint-container">
              <img
                src="src/assets/icons/question.png"
                alt="hint"
                className="input-hint-icon"
              />
              <div className="input-hint-tooltip">
                Right format: http&#91;s&#93;://&lt;hostname or IP&gt;:port 
              </div>
            </div>
          </div>
          <div className="horizontal-container">
            <label htmlFor="database">Database name:</label>
            <input name="database" type="text" defaultValue={Database}></input>
          </div>
          <div className="horizontal-container">
            <label htmlFor="user">Username:</label>
            <input name="user" type="text" defaultValue={Username}></input>
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
          <div className="horizontal-container">
            <LoadingButton loading={isLoadingConnect}>Reconnect</LoadingButton>
            <LoadingButton loading={isLoadingDisconnect} className="button-danger">
              Disconnect
            </LoadingButton>
          </div>
        </div>
      </form>
    </dialog>
  );
});

export default ConnectionEdit;
