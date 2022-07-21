# stress

This project was created to automate stress testing solutions
for ACM-like contests.

Now the project is my sandbox for improving my programming skills,
since in the process of improving the implementation, I often came
across non-obvious obstacles that needed to be solved.

## Navigation

You can read through anchors:

* [Basics](#basics)
* [Run options](#options)
* [How to make generator](#tests-generator)
* [How to make verifier](#verifier)
* [Programming languages support](#languages-support)

Or take into account just the usage message.

```
stress v1.0 by letstatt

Syntax:
stress tests [options] to_test [prime]

General:
-p           Pause each time test fails
-c [gvtp]    Do not recompile files if compiled ones cached
-n n         Run n tests (default: 10)

Tests:
-g file      Path to test generator
-t file      Path to file with tests
-s seed      Start generator with a specific seed

Limits:
-st          Display time and peak memory statistics
-tl ms       Set time limit in milliseconds
-ml mb       Set memory limit in MB

Prime:
-ptl ms      Set time limit for prime
-pml mb      Set memory limit for prime
-pre         Do not stop if prime got RE

Threading:
-mt          Multithreading
-w           Count of workers

Logging:
-stderr      Log stderr on runtime errors (useful with Java, Python, etc.)
-tag         Set tag of log file
-dnl         Disable logger (do not log)

Verifier:
-v file      Path to output verifier
-vstrict     Use strict comparison with prime solution

Misc:
-cv          Collapse identical verdicts
```


## Basics

Stress-testing is always about the testee and test sources.
The app helps you to automate the process by generating tests
and making them given to the program you want to test,
logging everything well.

Once the verdict is not `Accepted` or OK, the input of the test
is stored into a log file to let you reproduce the problem.
Internal analyzer also tries to predict the possible crash reason,
based on some debugging tools.

Let me show you a small how-to.

### Detect runtime error

* This line runs 10 tests created by your test generator
with solution you give to test.
* It's able to check **only** for `Runtime error` verdict.
```
stress -g generator solution_to_test
```

### Check for wrong answer

* If **prime solution** (which is maybe slower, but always correct) set,
then stress-tester will also check equality of outputs.
* By default, the check is soft, so whitespaces will be excluded.
* It also checks for `Wrong answer` verdict.
```
stress -g generator solution_to_test prime
```

* If correct answers may vary you should get a **verifier** for your
task, which accepts the test input and the output and make the decision of it.
* Note that verifier and prime solution couldn't be set together 
```
stress -g generator -v verifier solution_to_test
```

### Limit execution resources

* Stress supports **time** and **memory limiting** for your solution to test.
* More information is written below, in section where each parameter is describing.
```
stress -g generator -tl 10 -ml 50 solution
```

## What else?

### File formats

* If stress knows **how to run** your files, it will run them.
* If it knows that given files **need to be compiled** at first,
it will try to somehow compile and run them.
```
stress -g gen.exe solution.cpp prime.java
```

Otherwise, if it doesn't know how to process your files, it shuts down.

### Logging

* Stress will store logs to `./stress/logs/*tag*_*time*.txt`
* By default, tag is a filename of solution to test,
or define it using the option.
```
$stress -g gen.py -n 3 my_solution.py

Test 1, OK
Test 2, Wrong asnwer
Test 3, Runtime error

Stress-testing is over. Solution is broken
Check my_solution_14-02-47.txt
```
In addition to test input, it also stores how long your
solution has been working, how many memory it used
and tries to predict the reason of `Runtime error`.
```
$cat ./stress/logs/my_solution_14-02-47.txt

-------- TEST 2 --------
verdict: WA, 14 ms, 2 MB

1
- 4226


-------- TEST 3 --------
verdict: RE, 8 ms, 4 MB
integer division by zero

3
? 347
? 3460
- 8071
```

