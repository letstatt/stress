import subprocess, sys, os, re

p = subprocess.run(["stress", "-g", "src/gen_a_plus_b.py", "-n", "10", "src/exit_1.py", "src/sum.cpp"], capture_output=True, text=True)
sys.stderr.write(p.stdout.strip())

if p.returncode:
    exit(p.returncode)

logfile = None

for entry in os.scandir("stress/logs"):
    if entry.is_file() and re.match(r".*?[/\\]exit_1_.*", entry.path):
        logfile = entry

if logfile is None:
    sys.stderr.write("\n\nlogfile not found")
    exit(1)
else:
    cnt = p.stdout.count("Runtime error")
    if cnt != 10:
        sys.stderr.write("\n\nexpected 10 REs, got " + str(cnt))
        exit(1)
    exit(0)
