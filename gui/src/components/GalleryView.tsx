import type { ImageChunk, QueryResponse as QueryResponseType } from "../data/query-response";
import { imageHexToDataUrl, saveFile } from "../data/util";
import { useMemo, useState } from "react";

function ImageDisplay({ chunk }: { chunk: ImageChunk }) {
  const [isHovered, setIsHovered] = useState(false);

  const src = useMemo(
    () => imageHexToDataUrl(chunk.data, chunk.width, chunk.height),
    [chunk.data, chunk.width, chunk.height]
  );

  const technicalInfo = useMemo(() => {
    return `Format: PNG
Width: ${chunk.width}px
Height: ${chunk.height}px
Data (first 16 chars): ${chunk.data.substring(0, 16)}...`;
  }, [chunk]);

  const saveInfoAsText = (e: React.MouseEvent) => {
    e.stopPropagation();
    saveFile(chunk.data, `image-data-${Date.now()}.txt`);
  };

  return (
    <div 
      className="gallery-item" 
      style={{ position: "relative" }}
      onMouseEnter={() => setIsHovered(true)}
      onMouseLeave={() => setIsHovered(false)}
    >
      <img
        src={src}
        alt="Gallery data"
        style={{ maxWidth: "100%", maxHeight: "300px", objectFit: "contain", borderRadius: "4px" }}
      />
      {isHovered && (
        <div 
          className="image-tooltip"
          style={{
            position: "absolute",
            top: "24px",
            left: "24px",
            backgroundColor: "var(--color-accent-green)",
            opacity: 0.9,
            color: "white",
            padding: "16px",
            border: "none",
            borderRadius: "10px",
            fontSize: "12px",
            zIndex: 10,
            pointerEvents: "auto",
            width: "180px",
            boxShadow: "0 4px 12px rgba(var(--color-brown), 0.3)"
          }}
        >
          <pre style={{ margin: 0, whiteSpace: "pre-wrap", fontFamily: "var(--font-main)", color: "var(--color-main-light)" }}>
            {technicalInfo}
          </pre>
          <button 
            onClick={saveInfoAsText}
            style={{
              marginTop: "8px",
              width: "100%",
              padding: "4px",
              backgroundColor: "var(--color-accent-dark)",
              border: "none",
              color: "var(--color-main-light)",
              cursor: "pointer",
              borderRadius: "7px",
              fontFamily: "var(--font-main)",
            }}
          >
            <img style={{height: "12px", marginRight: "5px"}}
              src="src/assets/icons/diskette.png"
            ></img>
          Save media data as text
          </button>
        </div>
      )}
    </div>
  );
}

interface GalleryViewProps {
  response: QueryResponseType;
  columnName: string;
  query: string;
}

function GalleryView({ response, columnName, query }: GalleryViewProps) {
  const tableName = useMemo(() => {
    const match = query.match(/from\s+([a-zA-Z0-9_]+)/i);
    return match ? match[1] : "Current Query";
  }, [query]);

  if (!response.ok || !response.rows) return null;

  const images = response.rows
    .map((row) => row[columnName] as ImageChunk)
    .filter((chunk) => chunk && typeof chunk === "object" && "data" in chunk);

  return (
    <div className="gallery-view" style={{marginLeft: "24px"}}>
      <div
        className="gallery-header"
        style={{ padding: "10px", marginBottom: "10px" }}
      >
        <h3>Gallery</h3>
        <p style={{marginTop: "24px"}}>
          <span style={{ marginRight: "64px" }}>Table: {tableName}</span>
          <span style={{ marginRight: "64px" }}>Column: {columnName}</span>
          <span>Total records: {images.length}</span>
        </p>
      </div>
      <div
        className="gallery-grid"
        style={{
          display: "grid",
          gridTemplateColumns: "repeat(auto-fill, minmax(200px, 1fr))",
          gap: "15px",
          padding: "10px",
        }}
      >
        {images.map((chunk, index) => (
          <ImageDisplay key={index} chunk={chunk} />
        ))}
      </div>
    </div>
  );
}

export default GalleryView;
