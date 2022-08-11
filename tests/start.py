import os
import sys
import subprocess
import shutil

from utils.tests_loader import Section, Test, TestProgress, load_tests
from utils.console import print_progress


ENV_SEPARATOR = ';' if sys.platform == "win32" else ':'


def run_test(test: Test):
    test.status = TestProgress.TESTING
    stderr = ""

    with subprocess.Popen([sys.executable, test.loader_path.path],
                          cwd=os.getcwd(),
                          text=True,
                          stdin=subprocess.PIPE,
                          stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE,
                          env={**os.environ, 'PYTHONPATH': ENV_SEPARATOR.join(sys.path)}) as p:
        p.stdin.close()
        p.stdout.close()
        for line in iter(p.stderr.readline, ''):
            stderr += line
        p.wait()
        exit_code = p.returncode

    if exit_code != 0:
        test.status = TestProgress.FAILED
        return {
            "test": test,
            "exit_code": exit_code,
            "stderr": stderr
        }
    else:
        test.status = TestProgress.OK
        return None


def run_tests(section: Section):
    for test in section.tests:
        result = run_test(test)
        if result is not None:
            print("Test failed")
            print("Name:", section.name + "/" + result["test"].name)
            print("Exit code:", result["exit_code"])
            print("Stderr:")
            print(result["stderr"])
            return False
    return True


def clean():
    for entry in os.scandir("."):
        if entry.is_dir() and entry.name == "stress":
            shutil.rmtree("./utils/../sources/../tests/../stress")  # check if cwd is correct
            break
            

tests = load_tests()
clean()

if "clean" in sys.argv:
    print("cleaned, shutdown...")
    exit(0)

editor_func = print_progress(tests)
success = True

for i in range(len(tests)):
    for j in range(len(tests[i].tests)):
        tests[i].tests[j].status_update_hook =\
            lambda di=i, dj=j: editor_func(di, dj, tests[di].tests[dj].status)


for section in tests:
    if len(sys.argv) > 1 and section.name not in sys.argv:
        for test in section.tests:
            test.status = TestProgress.SKIPPED
        continue
    if not run_tests(section):
        success = False
        break

if success:
    clean()

exit(0 if success else 1)

# Tests:
# init             [**_____]
# verdicts         [______]
# logger           [+++]
# analyzer         [_______]
