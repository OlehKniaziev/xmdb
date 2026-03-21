import json
import os
import shutil
import unittest
import urllib.request
import urllib.error

from .helpers import find_free_port, sha256_hex, start_server
from .scenarios import SCENARIOS


class HttpIntegrationTest(unittest.TestCase):
    """Runs each scenario from scenarios.py via direct HTTP requests."""

    @classmethod
    def setUpClass(cls):
        cls.port = find_free_port()
        cls.server_proc, cls.tmpdir = start_server(cls.port)
        cls.base_url = f"http://127.0.0.1:{cls.port}"

    @classmethod
    def tearDownClass(cls):
        cls.server_proc.kill()
        cls.server_proc.wait()
        shutil.rmtree(cls.tmpdir, ignore_errors=True)

    def _post_json(self, path, payload):
        """POST JSON to the server and return (status_code, body_string)."""
        data = json.dumps(payload).encode("utf-8")
        req = urllib.request.Request(
            f"{self.base_url}{path}",
            data=data,
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        try:
            with urllib.request.urlopen(req) as resp:
                return resp.status, resp.read().decode("utf-8")
        except urllib.error.HTTPError as exc:
            return exc.code, exc.read().decode("utf-8")

    def _connect(self, db_name="default", username="admin", password="admin"):
        """Connect to the server and return (success_bool, connection_id_or_None)."""
        password_hash = sha256_hex(password)
        status, body = self._post_json("/connect", {
            "username": username,
            "password_hash": password_hash,
            "db_name": db_name,
        })
        if status == 200:
            return True, int(body)
        return False, None

    def _run_query(self, connection_id, sql):
        """Run a query and return the parsed JSON response dict."""
        status, body = self._post_json("/run-query", {
            "connection_id": connection_id,
            "query": sql,
        })
        return json.loads(body)


def _make_test(scenario):
    """Generate a test method for a given scenario."""

    def test_method(self):
        db_name = scenario.get("db_name", "default")
        expect_connect = scenario.get("expect_connect", True)

        success, connection_id = self._connect(db_name=db_name)

        if not expect_connect:
            self.assertFalse(success, f"Expected connection to '{db_name}' to fail")
            return

        self.assertTrue(success, f"Expected connection to '{db_name}' to succeed")

        for query_idx, query in enumerate(scenario.get("queries", [])):
            sql = query["sql"]
            expect_ok = query.get("expect_ok", True)
            expect_rows = query.get("expect_rows", [])

            result = self._run_query(connection_id, sql)

            with self.subTest(query_idx=query_idx, sql=sql):
                self.assertEqual(
                    result["ok"], expect_ok,
                    f"Query #{query_idx} ({sql!r}): expected ok={expect_ok}, "
                    f"got ok={result.get('ok')}, error={result.get('error_message', '')}",
                )

                if expect_rows is not None and expect_ok:
                    rows = result.get("rows", [])
                    self.assertEqual(
                        len(rows), len(expect_rows),
                        f"Query #{query_idx} ({sql!r}): expected {len(expect_rows)} rows, got {len(rows)}",
                    )
                    for row_idx, (actual, expected) in enumerate(zip(rows, expect_rows)):
                        for key, value in expected.items():
                            self.assertIn(key, actual, f"Row {row_idx} missing key '{key}'")
                            self.assertEqual(
                                actual[key], value,
                                f"Row {row_idx}, key '{key}': expected {value!r}, got {actual[key]!r}",
                            )

    test_method.__doc__ = scenario.get("description", scenario["name"])
    return test_method


# Dynamically add a test method for each scenario.
for _scenario in SCENARIOS:
    _test_name = f"test_{_scenario['name']}"
    setattr(HttpIntegrationTest, _test_name, _make_test(_scenario))


if __name__ == "__main__":
    unittest.main()
