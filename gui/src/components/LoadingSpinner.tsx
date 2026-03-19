import "../styles/loading-spinner.css";

function LoadingSpinner() {
  return (
    <div className="loading-container">
      <img src="src/assets/loading.svg" alt="loading..." className="spinner"/>
      <p>Loading...</p>
    </div>
  );
}

export default LoadingSpinner;
