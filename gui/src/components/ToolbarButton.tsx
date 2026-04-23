import {
  type ButtonHTMLAttributes,
  type ReactNode,
  useState,
} from "react";

interface ToolbarButtonProps {
  name: string;
  label: string;
  iconSrc?: string;
  hoverIconSrc?: string;
  iconAlt?: string;
  onClick?: ButtonHTMLAttributes<HTMLButtonElement>["onClick"];
  children?: ReactNode;
}

export default function ToolbarButton({
  name,
  label,
  iconSrc,
  hoverIconSrc,
  iconAlt,
  onClick,
  children,
}: ToolbarButtonProps) {
  const [currentIconSrc, setCurrentIconSrc] = useState(iconSrc);

  function handleMouseEnter() {
    if (hoverIconSrc) setCurrentIconSrc(hoverIconSrc);
  }

  function handleMouseLeave() {
    if (iconSrc) setCurrentIconSrc(iconSrc);
  }

  return (
    <div className="btn-label-container">
      <button
        id={name}
        name={name}
        className="toolbar-btn"
        onClick={onClick}
        onMouseEnter={handleMouseEnter}
        onMouseLeave={handleMouseLeave}
      >
        {currentIconSrc ? (
          <img alt={iconAlt ?? label} src={currentIconSrc} />
        ) : (
          children
        )}
      </button>
      <label htmlFor={name}>{label}</label>
    </div>
  );
}
