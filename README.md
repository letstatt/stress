# stress

This project was created to automate stress testing of solutions
for ACM-like contests.

Now the project is my sandbox for improving my programming skills,
since in the process of improving the implementation, I often came
across non-obvious obstacles that needed to be solved.

## Navigation

You can read through anchors:

* [Read basics](#basics)
* [A closer look at features](#a-closer-look-at-features)
* [Overview of console options](#options)
* [Languages support](#languages-support)
* [What is a generator?](#generator)
* [What is a verifier?](#verifier)

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
-mt          Allow multithreaded testing
-w n         Set count of workers

Logging:
-stderr      Log stderr on runtime errors (useful with Java, Python, etc.)
-tag         Set tag of log file
-dnl         Disable logger (do not log)

Verifier:
-v file      Path to output verifier
-vstrict     Use strict comparison of outputs

Misc:
-cv          Collapse identical verdicts
```


# Basics

Stress-testing is always about the testee and test sources.
The app helps you to automate the process by generating tests
(or getting from the other source) and making them given to
the program you want to test, logging everything well.

Once the verdict is not `Accepted` or OK, the input of the test
is stored into a log file to let you reproduce the problem.
Internal analyzer also tries to predict the possible crash reason,
based on some debugging tools.

Let me show you some basics.

### Runtime error detection

* This line runs 10 tests created by your test generator
with the solution you give to test.
* Generators are not the only one source of tests, find additional info below.
* It's able to check **only** for the `Runtime error` verdict.
```
stress -g generator solution_to_test
```

### Single correct answer

* Let's define a **prime solution** as a solution, which is maybe slower, but always correct.
* If the prime is set, then stress will start both solutions and check equality of outputs.
* By default, the check is soft, so whitespaces will be skipped.
* It can check for the `Wrong answer` verdict.
```
stress -g generator solution_to_test prime
```

### Multiple correct answers

* If there are multiple correct answers, you should create or get a **verifier** for your task.
* Verifiers accept the test input and the output and make the decision about it.
* Prime solution and verifier cannot be set together.
```
stress -g generator -v verifier solution_to_test
```

### Execution limits

* Stress supports **time** and **memory limiting** for your solution to test.
* Also, analogues are available for prime solution.
```
stress -g generator -tl 10 -ml 50 solution
```

### Logging

* Stress will store logs to `./stress/logs/*tag*_*time*.txt`
* By default, **tag** is a filename of the solution to test,
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
In addition to test input, it also stores the verdict received,
how long your solution has been working, how much memory it has
used and tries to predict the reason of `Runtime error`.

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

# A closer look at features

### Disk writings avoidance
* Disk I/O is **not used** when transferring generated tests and outputs data to programs.
* Stress improves its performance that way and also cares about disks' health.

### Tiny debugger inside
* It prevents the appearance of Windows Error Reporting in case of `Runtime error`.
* Also, it allows to predict the possible crash reason to let you find the bug easily.
* However, precision and abilities are OS-dependent.

### Multithreaded stress-testing
* It allows to start **several workers**, sharing a single console and log file.
* Each worker processes its chain **independently** - receives a test, an output and makes a result.
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
* To use cached programs, if they are present, special flags can be used.
* Do not to use the option mindlessly not to test outdated programs.
```
stress -g gen.cpp -c gp solution.cpp prime.java
```

## Options

### Basic

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
if your solutions have the same names, inappropriate cached files will be used.

### Test sources: generator

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

### Test sources: file

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
your solution spend on each test and **how much memory** did it use.

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
testing by **ignoring runtime errors of prime solution**, use parameter `-pre`.

Before:
```
$stress -g gen solution prime

Test 1, RE of prime solution

Stress-testing is over. Solution is correct
Check solution_14-02-47.txt
```
After:
```
$stress -g gen -pre solution prime

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

To **disable logger** (not to write logs for that session) use parameter `-dnl`. This means is "Do Not Log".

```
$stress -g gen -n 3 -dnl my_solution

Test 1, OK
Test 2, Wrong asnwer
Test 3, Runtime error

Stress-testing is over. Solution is broken
Logger disabled
```

### Verifier

To set a custom verifier, use parameter `-v`.
Note, that prime solution and verifier cannot be set together, because it's useless.
```
stress -g gen -v verifier solution
```

By default, the built-in verifier is soft, so whitespaces will be skipped.
To enable **strict built-in verifier** (i.e. which zero-tolerant to whitespaces)
instead of default one, use parameter `-vstrict`.

Before:
```
$stress -g gen solution prime
...
OK, whitespaces skipped, answers are correct
```
After:
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

## Languages support

This part is about how the stress-tester tries to deal with various files.

Default compilation commands can be unsuitable for you.
Check for them ahead. Sometime I will implement feature to
let you set compilation commands without sources editing,
but now you can just compile your files by yourself.

### PE (.exe)
Can be run natively on Windows

### Batch files (.bat)
Can be run natively on Windows

### ELF (Linux binary files)
Can be run natively on Linux

### Shell scripts (.sh)
Can be run natively on Linux

### Python (.py)
Can be run by the command below:
```
Windows:
  python {$PATH}
Linux:
  python3 {$PATH}
```

### C++ (.cpp)
Can be compiled by the command below:
```
c++ -std=c++17 -static -w -o ${COMPILED_PATH} ${PATH}
```
Then it can be run as common executable.

### Rust (.rs)
Can be compiled by the command below:
```
rustc -o ${COMPILED_PATH} --crate-name test ${PATH}
```
Then it can be run as common executable.

### Java (.java)
Suitable only for single `.java` files.
* **No classpath** support for this file format.
* **Packages are not** also supported.

Can be compiled by the command below:
```
javac -d ${CACHE_DIR} ${PATH}
```
Then in can be run as the file below:
```
${CACHE_DIR}/${FILENAME}.class
```

### Java (.class)
Suitable only for single `.class` files.
* **No classpath** support for this file format.
* **Packages are not** also supported.

Can be run by the command below:
```
java -cp ${DIR} ${FILENAME}
```

### Java (.jar)
Seems to be the best variant to test Java applications.

Can be run by the command below:
```
java -jar ${PATH}
```

## Generator

A generator is one of sources of tests.
It can be passed to stress by parameter `-g`.

### What does it do
1. Generates a valid test corresponding to your task.
2. Writes it to `stdout` (e.g. `printf`, `cout`, `print`, `System.out.print`, etc.).

If a generator can't be started or get runtime error,
then the stress shuts down - it is a critical error.

### Note for Windows users
It has been seen, that on MinGW C++ line separators in `stdout` are
automatically converted to `\r\n`.

Since that happens, if you need to have strictly `\n` instead of `\r\n` in your
tests, use the pattern below to change the behavior:

```
#include <fcntl.h>
#include <io.h>
...

int main() {
    _setmode(fileno(stdout), _O_BINARY);
    ...
}

```

This sets `stdout` to binary mode.

## Verifier

A custom verifier can be used to approve output of the solution to test in a more appropriate way.

Why? Because built-in verifier never allows multiple correct answers, but only checks
for equality of the outputs between solution to test and prime solution.

Interaction between stress and verifier is through `stdin` and `stdout`.

### It receives the following things from `stdin` 

1. A test
2. Line separator
3. Output of solution to test

It is important to understand to what the using of line separator leads.
For example, if you use `input()` in Python to parse the input,
you should skip the line by an extra `input()` calling.

### Correct verifier answer
is a **printed** word "OK" (also "AC"), "WA" or "PE" to `stdout` without quotes.

1. "OK" or "AC" means the answer is correct.
2. "WA" means the answer is wrong.
3. "PE" means "Presentation error". It is an optional verifier status saying that the answer has improper format.

If verifier can't be started, get runtime error or print the improper result
(so-called "Verification error"), then the stress shuts down - it
is a critical error.
