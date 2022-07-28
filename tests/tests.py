import os
import string
import re
import sys
from enum import Enum
import subprocess
import shutil


class TestProgress(Enum):
    NOT_TESTED = 1
    TESTING = 2
    FAILED = 3
    OK = 4
    SKIPPED = 5


def load_sections():
    sections = []
    ids = set()

    for entry in os.scandir('.'):
        if entry.is_dir():
            m = re.match(r".*?(\d+)_(.*)", entry.path)
            if m and len(m.groups()) == 2:
                g = m.groups()
                sections.append({"name": g[1], "entry": entry})
                ids.add(g[1])

    assert len(ids) == len(sections), "non-unique sections' ids assertion"
    return sections


def load_tests():
    sections = load_sections()

    for s in sections:
        s["tests"] = []
        ids = set()

        for entry in os.scandir(s["entry"]):
            if entry.is_dir():
                m = re.match(r".*?(\d+)_(.*)", entry.path[len(s["entry"].path):])
                if m and len(m.groups()) == 2:
                    test_loader = None
                    for entry2 in os.scandir(entry):
                        if entry2.is_file() and re.match(r".*?[/\\]test.py", entry2.path):
                            assert test_loader is None, "regexp recognized two test loaders in " + entry.path
                            test_loader = entry2
                            break

                    if test_loader is not None:
                        g = m.groups()
                        s["tests"].append(
                            {"name": g[1], "cwd": entry, "loader": test_loader, "state": TestProgress.NOT_TESTED})
                        ids.add(g[1])

        assert len(ids) == len(s["tests"]),\
            "non-unique tests' ids assertion in section " + s["name"]
        s.pop("entry")
    return sections


def update_progress(section_index, test_index, status, indentation, bottom):
    print("\x1B[H", end="")  # move cursor to (0, 0)
    print("\x1B[" + str(2 + section_index) + "B", end="")  # move cursor
    print("\x1B[" + str(indentation + test_index) + "C", end="")  # move cursor
    ch = "_"
    if status == TestProgress.TESTING:
        ch = "*"
    elif status == TestProgress.OK:
        ch = "+"
    elif status == TestProgress.FAILED:
        ch = "X"
    elif status == TestProgress.SKIPPED:
        ch = "~"
    print(ch + "\x1B[H", end="")  # edit symbol and move cursor to (0, 0)
    print("\x1B[" + str(bottom) + "B", end="")  # move cursor to bottom
    sys.stdout.flush()


def print_progress(tests):
    print("\x1B[2J\x1B[HTests:")
    print()
    indentation = 0
    for section in tests:
        indentation = max(indentation, len(section["name"]))
    indentation += 12
    for i in range(len(tests)):
        section = tests[i]
        print(i + 1, section["name"], end="")
        print(" " * (indentation - len(section["name"])), end="")
        print("[" + "_" * len(section["tests"]) + "]")
    print()
    return lambda section_index, test_index, status:\
        update_progress(section_index, test_index, status, indentation + 3, len(tests) + 3)


def run_test(test_entry):
    test_entry["state"] = TestProgress.TESTING
    test_entry["update_console"]()
    stderr = ""

    with subprocess.Popen([sys.executable, "test.py"],
                          cwd=test_entry["cwd"],
                          text=True,
                          stdin=subprocess.PIPE,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE) as p:
        p.stdin.close()
        p.stdout.close()
        for line in iter(p.stderr.readline, ''):
            stderr += line
        p.wait()
        exit_code = p.returncode

    if exit_code != 0:
        test_entry["state"] = TestProgress.FAILED
        test_entry["update_console"]()
        return {
            "test": test_entry,
            "exit_code": exit_code,
            "stderr": stderr
        }
    else:
        test_entry["state"] = TestProgress.OK
        test_entry["update_console"]()
        return None


def run_tests(section_name, tests):
    for test in tests:
        result = run_test(test)
        if result is not None:
            print("Test failed")
            print("Name:", section_name + "/" + result["test"]["name"])
            print("Exit code:", result["exit_code"])
            print("Stderr:")
            print(result["stderr"])
            return False
    return True


def clean(tests):
    for section in tests:
        for test in section["tests"]:
            for entry in os.scandir(test["cwd"]):
                if entry.is_dir() and entry.name == "stress":
                    shutil.rmtree(entry)


tests = load_tests()
clean(tests)

editor_func = print_progress(tests)
success = True

for i in range(len(tests)):
    for j in range(len(tests[i]["tests"])):
        tests[i]["tests"][j]["update_console"] =\
            lambda di=i, dj=j: editor_func(di, dj, tests[di]["tests"][dj]["state"])


for section in tests:
    if len(sys.argv) > 1 and section["name"] not in sys.argv:
        for test in section["tests"]:
            test["state"] = TestProgress.SKIPPED
            test["update_console"]()
        continue
    if not run_tests(section["name"], section["tests"]):
        success = False
        break

if success:
    clean(tests)

# Tests:
# init             [**_____]
# verdicts         [______]
# logger           [+++]
# analyzer         [_______]
