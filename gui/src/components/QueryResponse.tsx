import type { QueryResponse } from "../data/query-responce";

function QueryResponse({ response }: { response: QueryResponse }) {
  return (
    <>
      <table className="query-response-table">
        <thead>
          <tr>
            {response.columnNames.map((name) => {
              return <th>{name}</th>;
            })}
          </tr>
        </thead>
        <tbody>
          {response.rows.map((row) => {
            return (
              <tr>
                {response.columnNames.map((columnName) => (
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

export default QueryResponse;
