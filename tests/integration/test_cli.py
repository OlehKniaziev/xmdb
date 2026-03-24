import os
import shutil
import subprocess
import unittest

from .helpers import find_free_port, start_server
from .scenarios import SCENARIOS


class CliIntegrationTest(unittest.TestCase):
    """Runs each scenario from scenarios.py by spawning the CLI as a subprocess."""

    @classmethod
    def setUpClass(cls):
        cls.port = find_free_port()
        cls.server_proc, cls.tmpdir = start_server(cls.port)
        cls.cli_bin = os.environ.get("CLI_BIN")
        if not cls.cli_bin:
            raise RuntimeError("CLI_BIN environment variable not set")

    @classmethod
    def tearDownClass(cls):
        cls.server_proc.kill()
        cls.server_proc.wait()
        shutil.rmtree(cls.tmpdir, ignore_errors=True)

    def _run_cli(self, queries, db_name="default", username="admin", password="admin"):
        """Run the CLI with the given queries piped to stdin.

        Returns (returncode, stdout_str, stderr_str).
        """
        stdin_data = "\n".join(queries) + "\n" if queries else "\n"

        proc = subprocess.run(
            [
                self.cli_bin,
                "-hostname", "localhost",
                "-port", str(self.port),
                "-db", db_name,
                "-user", username,
                "-password", password,
            ],
            input=stdin_data,
            capture_output=True,
            text=True,
            timeout=30,
        )
        return proc.returncode, proc.stdout, proc.stderr

    @staticmethod
    def _parse_rows(stdout):
        """Parse CLI pipe-delimited rows from stdout.

        The CLI prints rows like: |val1|val2|
        Returns a list of lists of raw string values.
        """
        rows = []
        for line in stdout.splitlines():
            line = line.strip()
            pipe_idx = line.find("|")
            if pipe_idx == -1:
                continue

            line = line[pipe_idx:]
            cells = line.split("|")[1:-1]
            rows.append(cells)
        return rows

    def test_cli_invalid_credentials(self):
        """Wrong password should cause CLI to exit with an error."""
        error_code = self._run_cli([], password="wrong_password")[0]
        self.assertNotEqual(error_code, 0, "CLI should fail with wrong password")


def _make_cli_test(scenario):
    """Generate a test method for a given scenario."""

    def test_method(self):
        db_name = scenario.get("db_name", "default")
        expect_connect = scenario.get("expect_connect", True)
        queries = [q["sql"] for q in scenario.get("queries", [])]

        returncode, stdout, stderr = self._run_cli(queries, db_name=db_name)

        if not expect_connect:
            self.assertNotEqual(returncode, 0,
                                f"Expected CLI to fail connecting to '{db_name}'")
            return

        self.assertIn("Successfully connected", stdout,
                      f"CLI did not report successful connection.\nstdout: {stdout}\nstderr: {stderr}")

        query_specs = scenario.get("queries", [])
        parsed_rows = self._parse_rows(stdout)

        all_expected_rows = []
        for query in query_specs:
            expect_rows = query.get("expect_rows")
            if expect_rows is not None:
                all_expected_rows.extend(expect_rows)

        if all_expected_rows:
            self.assertEqual(
                len(parsed_rows), len(all_expected_rows),
                f"Expected {len(all_expected_rows)} output rows, got {len(parsed_rows)}.\n"
                f"stdout: {stdout}",
            )

            for row_idx, (actual_cells, expected_row) in enumerate(zip(parsed_rows, all_expected_rows)):
                expected_values = list(expected_row.values())
                self.assertEqual(
                    len(actual_cells), len(expected_values),
                    f"Row {row_idx}: expected {len(expected_values)} columns, got {len(actual_cells)}",
                )
                for col_idx, (actual, expected) in enumerate(zip(actual_cells, expected_values)):
                    if isinstance(expected, str):
                        actual_stripped = actual.strip('"')
                        self.assertEqual(actual_stripped, expected,
                                         f"Row {row_idx}, col {col_idx}: expected {expected!r}, got {actual!r}")
                    elif isinstance(expected, int):
                        actual_int = int(actual)
                        self.assertEqual(actual_int, expected,
                                         f"Row {row_idx}, col {col_idx}: expected {expected}, got {actual}")
                    else:
                        raise RuntimeError(f"Unsupported type of 'expected' value '{type(expected)}'")

    test_method.__doc__ = scenario.get("description", scenario["name"])
    return test_method


# Dynamically add a test method for each scenario.
for _scenario in SCENARIOS:
    _test_name = f"test_{_scenario['name']}"
    setattr(CliIntegrationTest, _test_name, _make_cli_test(_scenario))


if __name__ == "__main__":
    unittest.main()
