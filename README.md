# stress

This project was created to automate stress testing of solutions
for ACM-like contests.

Now the project is my sandbox for improving my programming skills,
since in the process of improving the implementation, I often came
across non-obvious obstacles that needed to be solved.

## Navigation

You can read through anchors:

* [Basics](#basics)
* [A closer look to features](#a-closer-look)
* [Console options explanation](#options)
* [Making a generator](#tests-generator)
* [Making a verifier](#verifier)
* [Languages support](#languages-support)

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

Let me show you some patterns.

### Detect runtime error

* This line runs 10 tests created by your test generator
with solution you give to test.
* Generator - is not the only one source of tests, find additional info below.
* It's able to check **only** for `Runtime error` verdict.
```
stress -g generator solution_to_test
```

### Check for wrong answer

* Let's define **prime solution** as a solution, which is maybe slower, but always correct.
* If the prime set, then stress will start both solutions and check equality of outputs.
* By default, the check is soft, so whitespaces will be skipped.
* It can check for `Wrong answer` verdict.
```
stress -g generator solution_to_test prime
```

### If correct answers vary

* If correct answers vary you should get a verifier for your task.
* **Verifier** accepts the test input and the output and make the decision of it.
* Prime solution and verifier cannot be set together.
```
stress -g generator -v verifier solution_to_test
```

### Execution limits

* Stress supports **time** and **memory limiting** for your prime and solution to test.
* More information is written below, in section where each parameter is describing.
```
stress -g generator -tl 10 -ml 50 solution
```

## A closer look

### Disk writings avoidance
* Disk I/O is **not used** when transferring generated tests and outputs data to programs.
* Stress improves its performance that way and also cares about disks' health. 

### Multithreaded stress-testing
* It allows to start **several workers**, sharing a single console and log file.
* Each worker processes its chain **independently** - receives a test, an output, makes a result.
* More information is written below, in section where each parameter is describing.
```
stress -g generator -mt solution prime
```

### Support for various file formats

* If stress knows **how to run** your files, it will run them.
* If it knows that given files **need to be compiled** at first,
it will try to compile and run them.
* Otherwise, if it doesn't know how to process your files, it shuts down.
```
stress -g gen.exe solution.cpp prime.java
```

### Compilation cache
* By default, files, which needs to be compiled, **will be recompiled**
each time stress starts.
* To use cached programs, if they are presented, special flags can be used.
* Do not to use the option mindlessly not to test outdated programs.
```
stress -g gen.cpp -c gp solution.cpp prime.java
```

### Logging

* Stress will store logs to `./stress/logs/*tag*_*time*.txt`
* By default, **tag** is a filename of solution to test,
or define it using the option.
* Logging can be disabled by special option.
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

There is an example of log file:
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

## Options

### General

To set the **count of tests** use parameter `-n`.
```
stress -g gen -n 50 to_test // 50 tests
stress -g gen -n 1000 to_test // 1000 tests
```

Use parameter `-p` to **pause** after each failure.
You can check logs at that moment and continue testing
or stop further testing.
```
$stress -g gen -n 10 -p to_test.cpp

Test 1, OK
Test 2, OK
Test 3, Runtime error
(press any key to continue or Ctrl+C to exit)
```

### Cache policy

Default behavior is recompiling files on each start.
Use parameter `-c` with cache flags `gvtp` **not to
recompile** files if cached ones exist.
```
// NOTE:
// `g` is generator
// `v` is verifier
// `t` is solution to test
// `p` is prime solution

// use cached generator if possible (most common case)
stress -g generator.cpp -c g to_test.cpp

// use cached generator and prime (yet another common case)
stress -g generator.cpp -c gp to_test.cpp prime.cpp

// also correct
stress -g generator.cpp -c pg to_test.cpp prime.cpp
```

Be careful of setting `-c` option on every testing, because
if your solutions have same names, inappropriate cached files will be used.

### Test sources

Use parameter `-g` to set test generator. No test formatting restrictions.
```
stress -g generator to_test
```

Each time generator starts it gets a random unsigned integer in stdin to init its random.
It's recommended to use the number to let you reproduce the test generation chain.
To manually initialize tester's random generator, use parameter `-seed`.
Default value is maximum unsigned integer value.
```
stress -g generator -seed 1337 to_test
```

Use parameter `-t` to set file with tests. Each next test must be separated
from the previous one by two or more line separators (CR or CRLF).
```
stress -g tests.txt to_test
```
There is an example of file with tests:
```
$cat tests.txt

2
- 7152
- 8580

1
+ 7204 adhzd

2
+ 260 oylmk
+ 3304 sq
```

### Time and memory

Parameter `-st` can be used to see **how much time** did
your solution spend on each test and **how many memory** did it use.

It tries to measure time and memory as accurate as possible.
```
$stress -g generator -n 3 -st solution

Test 1, OK            3 ms, 2 MB
Test 2, OK            5 ms, 2 MB
Test 3, OK            11 ms, 4 MB

Average time: 6 ms
Maximum time: 11 ms
Completed in: 59 ms
```

The stress-tester supports **time limiting** for your solution
during each test by parameter `-tl` (milliseconds). _This option implicitly set `-st`._
```
$stress -g generator -n 3 -tl 10 solution

Test 1, OK            3 ms, 2 MB
Test 2, OK            5 ms, 2 MB
Test 3, TL            11 ms, 4 MB

Average time: 6 ms
Maximum time: 11 ms
Completed in: 59 ms
```

Also, it supports **memory limiting** for solution to test
by parameter `-ml` (megabytes). _This option implicitly set `-st`._

```
$stress -g generator -n 3 -ml 3 solution

Test 1, OK            3 ms, 2 MB
Test 2, OK            5 ms, 2 MB
Test 3, ML            11 ms, 4 MB

Average time: 6 ms
Maximum time: 11 ms
Completed in: 59 ms
```

### Prime solution

If you want to **limit execution resources** for prime solution too,
there are `-ptl` and `-pml` analogues for the options described above.
Exceeding limits of prime solution will not lead to failure and not be logged.
_These options don't set `-st`._

```
$stress -g generator -n 3 -ptl 30 solution prime

Test 1, Skipped by limits
Test 2, OK
Test 3, OK

Stress-testing is over. Solution is correct
```

`Runtime error` of prime solution is a **critical error** by default
and shuts the stress-tester down (the test will be stored).
In case of using an unstable prime solution, that sometimes got non-zero
exit code or caught bad signals, there is a way not to interrupt
testing by **ignoring runtime errors of prime solution**.

Use parameter `-pre`.

```
$stress -g gen solution prime

Test 1, RE of prime solution

Stress-testing is over. Solution is correct
Check solution_14-02-47.txt
```
```
$stress -g gen -ignorepre solution prime

Test 1, RE of prime solution
Test 2, OK
Test 3, OK
...
```

### Multithreading

If you use parameter `-mt` and your computer has N available threads,
stress will be processing N-1 tests at once (not lower than 2).
To set number of workers manually, use parameter `-w`.

**Be careful** of starting multithreaded testing without being sure
that your programs support multiple running instances.
```
stress -g generator -mt solution prime

[*] Workers count: 7
[*] Ready

...
```

### Logging

Some programming languages use a virtual machine to run its bytecode.
That's why some uncaught exceptions and errors may not be available to
stress-tester by means of OS, but in most cases VMs write stacktraces
and error descriptions in **stderr**.

If you want to log **stderr dumps** as well as tests itself,
use parameter `-stderr`. It's useful with Java, Python, etc.

```
$stress -g gen -stderr DivisionByZero.java
$cat ./stress/logs/DivisionByZero_16-59-37.txt

-------- TEST 1 --------
verdict: RE, 159 ms, 8 MB
non-zero exit code (0x1)
stderr dump: >>>
Exception in thread "main" java.lang.ArithmeticException: / by zero
	at DivisionByZero.main(DivisionByZero.java:5)

<<<

3
- 966
+ 2442 qcuxm
+ 8255 uc
```

To easily orientate in `./stress/logs/` folder use can set
**tag of logfile** by parameter `-tag`.

So logfiles will have names like this: `./stress/logs/*tag*_*time*.txt`

```
$stress -g gen -n 3 -tag lab1-A solution_a

Test 1, OK
Test 2, Wrong asnwer
Test 3, Runtime error

Stress-testing is over. Solution is broken
Check lab1-A_14-02-47.txt
```

To **disable logger** (not to write logs for that session) use parameter `-dnl`.

```
$stress -g gen -n 3 -dnl my_solution

Test 1, OK
Test 2, Wrong asnwer
Test 3, Runtime error

Stress-testing is over. Solution is broken
Logger disabled
```

### Verifier

To set a custom verifier, use parameter `-v`. Note, that prime solution and verifier cannot be set together.
```
stress -g gen -v verifier solution
```

To enable **strict built-in verifier** (i.e. which zero-tolerant to whitespaces)
instead of default one (which skips whitespaces), use parameter `-vstrict`.
```
$stress -g gen solution prime
...
OK, whitespaces skipped, answers are correct
```
```
$stress -g gen -vstrict solution prime
...
Solution is broken, outputs are different
```

### Miscellaneous

To make presentation in console during testing more compact,
you can use parameter `-cv` to **collapse consequent identical verdicts**.

```
$stress -g gen -cv solution

Test 1,  OK
Test 2,  Runtime error             (+1)
Test 4,  OK
Test 5,  Runtime error             (+1)
Test 7,  OK                        (+3)

Stress-testing is over. Solution is broken
Check solution_17-46-47.txt
```

