import subprocess, sys

p = subprocess.run(["stress", "-g", "dummy.rs", "dummy.rs"], capture_output=True, text=True)
sys.stderr.write(p.stdout.strip())
exit(p.returncode)
