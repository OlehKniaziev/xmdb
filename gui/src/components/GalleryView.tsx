import type { ImageChunk, QueryResponse as QueryResponseType } from "../data/query-response";
import { imageHexToDataUrl, saveFile } from "../data/util";
import { useMemo, useState } from "react";

function ImageDisplay({ chunk, onClick }: { chunk: ImageChunk; onClick: () => void }) {
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
      style={{ position: "relative", overflow: "visible", justifySelf: "start", alignSelf: "start" }}
      onMouseEnter={() => setIsHovered(true)}
      onMouseLeave={() => setIsHovered(false)}
    >
      <img
        src={src}
        alt="Gallery data"
        style={{ maxWidth: "100%", maxHeight: "300px", objectFit: "contain", borderRadius: "4px", cursor: "pointer" }}
        onClick={onClick}
      />
      {isHovered && (
        <div 
          className="image-tooltip"
          style={{
            position: "absolute",
            top: "calc(100% + 4px)",
            left: "calc(100% + 4px)",
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

function FullScreenOverlay({ images, currentIndex, onClose, onNavigate }: {
  images: ImageChunk[];
  currentIndex: number;
  onClose: () => void;
  onNavigate: (index: number) => void;
}) {
  const chunk = images[currentIndex];
  const src = useMemo(
    () => imageHexToDataUrl(chunk.data, chunk.width, chunk.height),
    [chunk.data, chunk.width, chunk.height]
  );

  const hasPrev = currentIndex > 0;
  const hasNext = currentIndex < images.length - 1;

  const arrowStyle: React.CSSProperties = {
    position: "fixed",
    top: "50%",
    transform: "translateY(-50%)",
    background: "transparent",
    border: "none",
    cursor: "pointer",
    zIndex: 1001,
    padding: 0,
  };

  return (
    <div
      style={{
        position: "fixed",
        top: 0,
        left: 0,
        width: "100vw",
        height: "100vh",
        backgroundColor: "rgba(69, 33, 3, 0.4)",
        display: "flex",
        alignItems: "center",
        justifyContent: "center",
        zIndex: 1000,
      }}
      onClick={onClose}
    >
      <button
        onClick={(e) => { e.stopPropagation(); onClose(); }}
        className="close-button-gallery"
        style={{
          position: "fixed",
          top: "2rem",
          right: "2rem",
          zIndex: 1001,
        }}
      >
        <img src="src/assets/icons/close.png" alt="Close" />
      </button>
      {hasPrev && (
        <button
          onClick={(e) => { e.stopPropagation(); onNavigate(currentIndex - 1); }}
          style={{ ...arrowStyle, left: "2rem" }}
        >
          <img src="src/assets/icons/left-arrow.png" alt="Previous" style={{ width: "56px", height: "56px" }} />
        </button>
      )}
      {hasNext && (
        <button
          onClick={(e) => { e.stopPropagation(); onNavigate(currentIndex + 1); }}
          style={{ ...arrowStyle, right: "2rem" }}
        >
          <img src="src/assets/icons/right-arrow.png" alt="Next" style={{ width: "56px", height: "56px" }} />
        </button>
      )}
      <img
        src={src}
        alt="Full size"
        onClick={(e) => e.stopPropagation()}
        style={{ maxWidth: "95vw", maxHeight: "95vh" }}
        width={chunk.width}
        height={chunk.height}
      />
    </div>
  );
}

function GalleryView({ response, columnName, query }: GalleryViewProps) {
  const [fullScreenIndex, setFullScreenIndex] = useState<number | null>(null);

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
          <ImageDisplay key={index} chunk={chunk} onClick={() => setFullScreenIndex(index)} />
        ))}
      </div>
      {fullScreenIndex !== null && (
        <FullScreenOverlay
          images={images}
          currentIndex={fullScreenIndex}
          onClose={() => setFullScreenIndex(null)}
          onNavigate={setFullScreenIndex}
        />
      )}
    </div>
  );
}

export default GalleryView;
