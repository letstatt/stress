from operator import countOf
import subprocess, sys, os, re
from enum import Enum
import shutil
from typing import Any, Callable

class Verdict(Enum):
    OK = "OK"
    RE = "Runtime error"
    WA = "Wrong answer"
    PE = "Presentation error"
    TL = "Time limit exceeded"
    ML = "Memory limit exceeded"


class Result:
    captureGroup = '|'.join([Verdict._member_map_[i].value for i in Verdict._member_map_])
    pattern = r"\bTest \d+,\s+({})\b".format(captureGroup)

    def __init__(self, p: subprocess.CompletedProcess[str], countOfTests: int):
        self.returncode = p.returncode
        self.stdout = p.stdout.strip()
        self.logs: list[os.DirEntry[str]] = []
        self.count = countOfTests

        try:
            for entry in os.scandir("./stress/logs"):
                if entry.is_file():
                    self.logs.append(entry)
        except FileNotFoundError:
            pass
        
        self.verdicts: list[str] = re.findall(self.pattern, self.stdout)
        print2(self.pattern)
        print2("\n")
        print2(self.stdout)
        print2("\n")
        print2(self.verdicts)
        


def print2(s: Any):
    sys.stderr.write(str(s))
    sys.stderr.flush()


def run(args: list[str], ok_fun: Callable[[Result], bool], die=False, recompile: bool=False, count: int=10):
    args2 = args\
        + (["-c", "gvtp"] if not recompile else [])\
        + (["-n", str(count)] if count >= 0 else [])

    p = subprocess.run(args2, capture_output=True, text=True)

    r = Result(p, count)
    ok = ok_fun(r)

    if not ok:
        print2(r.stdout)
    if ok and die:
        exit(1)
    if not ok:
        exit(1)


def runAndRet(*args, **kwargs):
    run(*args, ok_fun=lambda r: r.returncode == 0, **kwargs)


def runAndRetInv(*args, **kwargs):
    run(*args, ok_fun=lambda r: r.returncode != 0, **kwargs)


def runAndVerdict(*args, v: Verdict, **kwargs):
    def check(r: Result, v: Verdict):
        s = v.value
        exp = r.count
        got = r.verdicts.count(v.value)
        if got != exp:
            print2("expected {} of {}, got {}\n\n".format(exp, s, got))
        return got == exp

    run(*args, ok_fun=lambda r: check(r, v), **kwargs)


def runAndOK(*args, **kwargs):
    runAndVerdict(*args, v=Verdict.OK, **kwargs)


def clean_logs():
    shutil.rmtree("./stress/logs")

