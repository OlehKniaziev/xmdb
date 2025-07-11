import { NavLink } from "react-router-dom";
import "../styles/bars-style.css";

export default function Topbar() {
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
      </nav>
    </div>
  );
}
