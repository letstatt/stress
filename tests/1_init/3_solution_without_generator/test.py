import subprocess, sys

p = subprocess.run(["stress", "test.py"], capture_output=True, text=True)
sys.stderr.write(p.stdout.strip())
exit(0 if p.returncode != 0 else 1)
