export default function FileBar({ activeView }) {
  return (
      <div className="tab-bar-line">
        {activeView ? `Opened file: ${activeView}` : "No file opened"}
      </div>
  );
}
