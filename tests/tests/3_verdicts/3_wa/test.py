from utils.test_launcher import *


prefix = "sources/a_plus_b/"

run = lambda *args: runAndVerdict(*args, v=Verdict.WA)

run(["stress", "-g", prefix + "gen.py", prefix + "sum.py", "sources/zero.py"])
run(["stress", "-g", prefix + "gen.py", "sources/zero.py", prefix + "sum.py"])
run(["stress", "-g", prefix + "gen.py", prefix + "sum.py", "sources/zero.py", "-vstrict"])
run(["stress", "-g", prefix + "gen.py", prefix + "sum.py", prefix + "sum_with_spaces.py", "-vstrict"])
run(["stress", "-g", prefix + "gen.py", "-v", prefix + "verifier.py", "sources/zero.py"])
