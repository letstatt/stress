import os
import string
import re
import sys
from enum import Enum
import subprocess
import platform


class TestProgress(Enum):
    NOT_TESTED = 1
    TESTING = 2
    FAILED = 3
    OK = 4


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
            "non-unique tests' ids assertion in section " + s["entry"].path
        s.pop("entry")
    return sections


def print_progress(tests_progress):
    os.system("clear" if platform.system() == "Linux" else "cls")
    print("\x1B[2J\x1B[HTests:")
    print()
    indentation = 0
    for section in tests_progress:
        indentation = max(indentation, len(section["name"]))
    indentation += 12
    for i in range(len(tests_progress)):
        section = tests_progress[i]
        print(i + 1, section["name"], end="")
        print(" " * (indentation - len(section["name"])), end="")
        print("[", end="")
        for test in section["tests"]:
            ch = "_"
            if test["state"] == TestProgress.TESTING:
                ch = "*"
            elif test["state"] == TestProgress.OK:
                ch = "+"
            elif test["state"] == TestProgress.FAILED:
                ch = "X"
            print(ch, end="")
        print("]")
    print()


def run_test(test_entry):
    test_entry["state"] = TestProgress.TESTING
    stderr = b""

    with subprocess.Popen([sys.executable, "test.py"],
                          cwd=test_entry["cwd"],
                          stdin=subprocess.PIPE,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE) as p:
        p.stdin.close()
        p.stdout.close()
        for line in iter(p.stderr.readline, b''):
            stderr += line
        p.wait()
        exit_code = p.returncode

    if exit_code != 0:
        test_entry["state"] = TestProgress.FAILED
        return {
            "test": test_entry,
            "exit_code": exit_code,
            "stderr": bytes.decode(stderr, encoding="utf-8")
        }
    else:
        test_entry["state"] = TestProgress.OK
        return None


def run_tests(section_name, tests):
    for test in tests:
        print_progress(sections)
        result = run_test(test)
        print_progress(sections)
        if result is not None:
            print("Test failed")
            print("Name:", section_name + "/" + result["test"]["name"])
            print("Exit code:", result["exit_code"])
            print("Stderr:")
            print(result["stderr"])
            return False
    return True


sections = load_tests()

for section in sections:
    if not run_tests(section["name"], section["tests"]):
        break

# Tests:
# init             [**_____]
# verdicts         [______]
# logger           [+++]
# analyzer         [_______]
