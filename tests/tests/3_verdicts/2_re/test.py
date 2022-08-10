from utils.test_launcher import *


prefix = "sources/a_plus_b/"

runAndVerdict(["stress", "-g", prefix + "gen.py", "sources/exit_1.py", prefix + "sum.cpp"], v=Verdict.RE)
