import {
  forwardRef,
  useEffect,
  useImperativeHandle,
  useRef,
  useState,
} from "react";
import "../styles/bars-style.css";
import "../styles/forms-style.css";

export type ConnectionEditHandle = {
  open: () => void;
};

const ConnectionEdit = forwardRef<ConnectionEditHandle>((_, ref) => {
  const [isConnected, setIsConnected] = useState(false);

  const dialogRef = useRef<HTMLDialogElement>(null);

  useImperativeHandle(ref, () => ({
    open: () => {
     if (!dialogRef.current?.open) {
        dialogRef.current?.showModal();
      }
    },
  }));

  return (
    <dialog ref={dialogRef}>
      <h1>Edit connection</h1>
      <form
        autoComplete="off"
        onSubmit={(event) => {
          event.preventDefault();
        }}
      >
        <div className="vertical-container">
          <div className="horizontal-container">
            <label htmlFor="hostname">Hostname:</label>
            <input name="hostname" type="text" defaultValue="Hostname"></input>
          </div>
          <div className="horizontal-container">
            <label htmlFor="database">Database name:</label>
            <input
              name="database"
              type="text"
              defaultValue="Database name"
            ></input>
          </div>
          <div className="horizontal-container">
            <label htmlFor="user">Username:</label>
            <input name="user" type="text" defaultValue="Username"></input>
          </div>
          <div className="horizontal-container">
            <label htmlFor="password">Password:</label>
            <input
              name="password"
              type="password"
              defaultValue="Password"
            ></input>
          </div>
          <button onClick={() => dialogRef.current?.close()}>Reconnect</button>
        </div>
      </form>
    </dialog>
  );
});

export default ConnectionEdit;
