import type { ImageChunk, QueryResponse as QueryResponseType } from "../data/query-response";
import LoadingSpinner from "./LoadingSpinner";
import { useMemo } from "react";
import { imageHexToDataUrl } from "../data/util";

function ImageDisplay({ chunk }: { chunk: ImageChunk }) {
  const src = useMemo(
    () => imageHexToDataUrl(chunk.data, chunk.width, chunk.height),
    [chunk.data, chunk.width, chunk.height]
  );

  return (
    <img
      src={src}
      alt="PNG data"
      width={chunk.width}
      height={chunk.height}
      style={{ maxWidth: "200px", maxHeight: "200px" }}
    />
  );
}

function QueryResponse({
  response,
  isLoading,
}: {
  response: QueryResponseType | undefined;
  isLoading: boolean;
}) {
  if (isLoading) {
    return <LoadingSpinner />;
  }

  if (response === undefined) return <>Error</>;
  if (response.ok) {
    if (!response.column_names || !response.rows) {
      return <>Query executed successfully. No data returned.</>;
    } else {
      return (
        <>
          <table className="query-response-table">
            <thead>
              <tr>
                {response.column_names.map((name) => {
                  return <th>{name}</th>;
                })}
              </tr>
            </thead>
            <tbody>
              {response.rows.map((row) => {
                return (
                  <tr>
                    {response.column_names?.map((columnName, columnIndex) => {
                      if (typeof row[columnName] === "object") {
                        if (response.column_types?.[columnIndex] === "PNG") {
                          const chunk = row[columnName] as ImageChunk;

                          return (
                            <td>
                              <ImageDisplay chunk={chunk} />
                            </td>
                          );
                        } else {
                          return <td>{JSON.stringify(row[columnName])}</td>;
                        }
                      } else {
                        return (
                          <td>{row[columnName] as string | number | boolean}</td>
                        );
                      }
                    })}
                  </tr>
                );
              })}
            </tbody>
          </table>
        </>
      );
    }
  } else {
    return <>Error</>;
  }
}

export default QueryResponse;
