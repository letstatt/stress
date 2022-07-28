import subprocess, sys

p = subprocess.run(["stress", "-g", "dummy.py", "dummy.py", "dummy.py"], capture_output=True, text=True)
sys.stderr.write(p.stdout.strip())
exit(p.returncode)
