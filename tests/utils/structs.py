import os
from typing import Any
from enum import Enum

class TestProgress(Enum):
    NOT_TESTED = 1
    TESTING = 2
    FAILED = 3
    OK = 4
    SKIPPED = 5


class Test:

    def __init__(self, name: str, loader_path: os.DirEntry[str]):
        self.name = name
        self.loader_path = loader_path
        self.status = TestProgress.NOT_TESTED
        self.status_update_hook = lambda: None
        pass

    def __setattr__(self, name: str, val: Any) -> None:
        self.__dict__[name] = val
        if name == "status" and hasattr(self, "status_update_hook"):
            self.status_update_hook()


class Section:
    
    def __init__(self, name: str, entry: os.DirEntry[str]):
        self.name = name
        self.tests: list[Test] = []
        self.entry = entry

    def addTest(self, test: Test):
        self.tests.append(test)

