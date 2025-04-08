#!/usr/bin/env python3
import subprocess
import argparse
import os
import sys
import json

fail_on_mismatch = False

def main() -> None:
    argparser = argparse.ArgumentParser(prog="stiff", description="Run some snapshot tests")
    argparser.add_argument("executable")
    argparser.add_argument("-d", "--dir")
    argparser.add_argument("-f", "--fail-on-mismatch", action="store_true")
    args = argparser.parse_args()

    if args.dir is None:
        args.dir = "."

    if args.fail_on_mismatch:
        global fail_on_mismatch
        fail_on_mismatch = True

    test_snapshot(args.executable, args.dir)

def test_snapshot(executable: str, snapshot_dir: str) -> None:
    append_dot = True
    executable_dir = os.path.dirname(executable)
    if executable_dir == "":
        append_dot = False
        executable_dir = "."

    executable_name = os.path.basename(executable)
    snapshot_name = f"{executable}.json"
    full_snapshot_path = os.path.join(snapshot_dir, snapshot_name)
    snapshot_dirname = os.path.dirname(full_snapshot_path)

    if not os.path.exists(full_snapshot_path):
        os.makedirs(snapshot_dirname, exist_ok=True)
        with open(full_snapshot_path, "w") as f:
            f.write('{ "code": 0, "stdout": "", "stderr": "" }')

    expected_snapshot = load_snapshot(full_snapshot_path)
    if append_dot:
        executable_name = f"./{executable_name}"

    recorded_snapshot = record_snapshot(executable_name, executable_dir)
    (failed, save_recorded_snapshot) = report_snapshot_diff(expected_snapshot, recorded_snapshot)

    if save_recorded_snapshot:
        assert not failed
        save_snapshot(full_snapshot_path, recorded_snapshot)
        sys.exit(0)

    if failed:
        sys.exit(1)

    print("Recorded snapshot matches the expected one")

def save_snapshot(path: str, snapshot: dict) -> None:
    with open(path, "w") as f:
        json.dump(snapshot, f)

def record_snapshot(executable: str, dirname: str) -> None:
    proc = subprocess.run([executable], capture_output=True, cwd=dirname)
    snapshot = {
        "code": proc.returncode,
        "stdout": proc.stdout.decode("utf-8"),
        "stderr": proc.stderr.decode("utf-8"),
    }
    return snapshot

def report_snapshot_diff(expected: dict, actual: dict) -> (bool, bool):
    failed = False

    if expected["code"] != actual["code"]:
        print(f"Expected exit code: {expected["code"]}\n\nActual exit code: {actual["code"]}\n")
        failed = True

    if expected["stdout"] != actual["stdout"]:
        print(f"Expected stdout: {expected["stdout"]}\n\nActual stdout: {actual["stdout"]}\n")
        failed = True

    if expected["stderr"] != actual["stderr"]:
        print(f"Expected stderr: {expected["stderr"]}\n\nActual stderr: {actual["stderr"]}\n")
        failed = True

    if failed:
        if fail_on_mismatch:
            print("The recorded snapshot does not match the expected one")
            sys.exit(1)

        confirmation = input("Save new snapshot as correct? [y/N] ")
        if confirmation.lower().startswith("y"):
            return (False, True)

        return (True, False)

    return (False, False)

def load_snapshot(path: str) -> dict:
    with open(path) as f:
        return json.load(f)

if __name__ == "__main__":
    main()
