import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
// import './index.css'
import App from './App.tsx'

let isCtrlDown = false;

window.addEventListener("keydown", (e) => {
  if (e.key === "Control") {
    isCtrlDown = true;
  } else if (isCtrlDown && e.key === "=" || e.key === "-") {
    e.preventDefault();
  }
});

window.addEventListener("keyup", (e) => {
  if (e.key === "Control") {
    isCtrlDown = false;
  }
});

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <App />
  </StrictMode>,
)
