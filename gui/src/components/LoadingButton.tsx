import type { ButtonHTMLAttributes, ReactNode } from "react";

interface LoadingButtonProps extends ButtonHTMLAttributes<HTMLButtonElement> {
  loading: boolean;
  children: ReactNode;
  loadingAlt?: string;
}

export default function LoadingButton({
  loading,
  children,
  loadingAlt = "loading...",
  className = "",
  ...buttonProps
}: LoadingButtonProps) {
  return (
    <button className={`connect-button ${className}`.trim()} {...buttonProps}>
      {loading ? (
        <img src="src/assets/loading.svg" alt={loadingAlt} />
      ) : (
        <span>{children}</span>
      )}
    </button>
  );
}
