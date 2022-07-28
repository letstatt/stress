import subprocess, sys, os, re

def run(args):
    args2 = args + ["-n", "10"]
    p = subprocess.run(args2, capture_output=True, text=True)

    if p.returncode:
        sys.stderr.write(p.stdout.strip())
        exit(p.returncode)

    
    cnt = p.stdout.count("Wrong answer")
    if cnt != 10:
        sys.stderr.write(p.stdout.strip())
        sys.stderr.write("\n\nargs: " + str(args2))
        sys.stderr.write("\nexpected 10 WAs, got " + str(cnt))
        exit(1)


run(["stress", "-g", "src/gen_a_plus_b.py", "src/sum.py", "src/zero.py"])
run(["stress", "-g", "src/gen_a_plus_b.py", "src/zero.py", "src/sum.py"])
run(["stress", "-g", "src/gen_a_plus_b.py", "src/sum.py", "src/zero.py", "-vstrict"])
run(["stress", "-g", "src/gen_a_plus_b.py", "src/sum.py", "src/sum_with_spaces.py", "-vstrict"])
run(["stress", "-g", "src/gen_a_plus_b.py", "-v", "src/verifier.py", "src/zero.py"])
