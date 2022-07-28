import subprocess, sys, os, re

def run(args):
    p = subprocess.run(args + ["-c", "gvtp"], capture_output=True, text=True)

    if p.returncode:
        sys.stderr.write(p.stdout.strip())
        exit(p.returncode)

    logfile = None

    for entry in os.scandir("stress/logs"):
        if entry.is_file() and re.match(r".*?[/\\]sum_.*", entry.path):
            logfile = entry

    if logfile is not None:
        sys.stderr.write(p.stdout.strip())
        sys.stderr.write("\n\nargs: " + str(args))
        sys.stderr.write("\n\nlogfile is found")
        exit(1)


# a+b problem

prefix = "src/a_plus_b/"

run(["stress", "-g", prefix + "gen.py", prefix + "sum.py", prefix + "sum.cpp"])
run(["stress", "-g", prefix + "gen.py", prefix + "sum.py", prefix + "sum_with_spaces.py"])
run(["stress", "-g", prefix + "gen.py", "-v", prefix + "verifier.py", prefix + "sum.py"])
run(["stress", "-g", prefix + "gen.py", "-v", prefix + "verifier.py", prefix + "sum_with_spaces.py"])


# find the ones problem

prefix = "src/find_the_ones/"

run(["stress", "-g", prefix + "gen.py", prefix + "solution.py", prefix + "solution.py"])
run(["stress", "-g", prefix + "gen.py", prefix + "solution.py", prefix + "solution_with_spaces.py"])
