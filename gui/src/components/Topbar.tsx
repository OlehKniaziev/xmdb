import { NavLink, useLocation } from "react-router-dom";
import "../styles/bars-style.css";

export default function Topbar() {
  const location = useLocation();
  const isGalleryActive = location.pathname === "/gallery";
  const isObjectTabActive = location.pathname === "/object";

  return (
    <div className="topbar-bg">
      <nav className="topbar-container">
        <NavLink
          to="/"
          className={({ isActive }) =>
            isActive ? "nav-link-active" : "nav-link"
          }
        >
          Home
        </NavLink>
        <NavLink
          to="/query"
          className={({ isActive }) =>
            isActive ? "nav-link-active" : "nav-link"
          }
        >
          SQL
        </NavLink>
        <NavLink
          to="/objects"
          className={({ isActive }) =>
            isActive || isObjectTabActive ? "nav-link-active" : "nav-link"
          }
        >
          Objects
        </NavLink>
        {isGalleryActive && (
          <div className="nav-link-active" style={{backgroundColor: "#96504c"}}>
            Gallery
          </div>
        )}
      </nav>
    </div>
  );
}
