import type { QueryResponse } from "../data/query-response";
import LoadingSpinner from "./LoadingSpinner";

function QueryResponse({
  response,
  isLoading,
}: {
  response: QueryResponse;
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
                    {response.column_names.map((columnName) => (
                      <td>{row[columnName]}</td>
                    ))}
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
