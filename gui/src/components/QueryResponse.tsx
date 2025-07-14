import type { QueryResponse } from "../data/query-response";

function QueryResponse({ response }: { response: QueryResponse }) {
  if(response === undefined) return <>Error</>;
  if (response.ok) {
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
  } else {
    return <>Error</>;
  }
}

export default QueryResponse;
