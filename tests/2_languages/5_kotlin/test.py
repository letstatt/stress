import subprocess, sys

p = subprocess.run(["stress", "-g", "dummy.kt", "dummy.kt"], capture_output=True, text=True)
sys.stderr.write(p.stdout.strip())
exit(p.returncode)
