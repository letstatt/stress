from utils.test_launcher import *


# a+b problem

prefix = "sources/a_plus_b/"

runAndOK(["stress", "-g", prefix + "gen.py", prefix + "sum.py", prefix + "sum.cpp"])
runAndOK(["stress", "-g", prefix + "gen.py", prefix + "sum.py", prefix + "sum_with_spaces.py"])
runAndOK(["stress", "-g", prefix + "gen.py", "-v", prefix + "verifier.py", prefix + "sum.py"])
runAndOK(["stress", "-g", prefix + "gen.py", "-v", prefix + "verifier.py", prefix + "sum_with_spaces.py"])

# find the ones problem

prefix = "sources/find_the_ones/"

runAndOK(["stress", "-g", prefix + "gen.py", prefix + "solution.py", prefix + "solution.py"])
runAndOK(["stress", "-g", prefix + "gen.py", prefix + "solution.py", prefix + "solution_with_spaces.py"])
