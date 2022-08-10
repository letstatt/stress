from types import NoneType
from typing import Callable
from utils.structs import Section, Test, TestProgress
import sys


def print_progress(tests: list[Section]) -> Callable[[int, int, TestProgress], NoneType]:
    print("\x1B[2J\x1B[HTests:")  # clear console and move cursor to (0, 0)
    print()

    indentation = max([len(section.name) for section in tests]) + 12

    for i in range(len(tests)):
        section = tests[i]
        print(i + 1, section.name, end="")
        print(" " * (indentation - len(section.name)), end="")
        print("[" + "_" * len(section.tests) + "]")
    print()

    return lambda section_index, test_index, status:\
        update_progress(section_index, test_index, status, indentation + 3, len(tests) + 3)


def update_progress(section_index: int, test_index: int, status: TestProgress, indentation: int, bottom: int):
    print("\x1B[H", end="")  # move cursor to (0, 0)
    print("\x1B[" + str(2 + section_index) + "B", end="")  # move cursor V
    print("\x1B[" + str(indentation + test_index) + "C", end="")  # move cursor >
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
