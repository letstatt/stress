#define INVOKER_MACRO

#include "invoker.h"

#undef INVOKER_MACRO

#include "terminal.h"
#include "core/runtime_config.h"
#include "linux/core/error_info.h"
#include <sys/wait.h>
#include <unistd.h>
#include <thread>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/user.h>
#include <cassert>
#include <cstring>
#include <unordered_set>
#include "linux/parsing/proc_parser.h"

#ifdef __x86_64__
#define register_ip rip
#define register_sp rsp
#else
#error "you should define macros for important registers"
#endif

namespace {
    constexpr size_t BATCH_SIZE = 256;
    constexpr size_t WATCHER_INTERVAL_MS = 5;
    constexpr size_t WATCHER_INTERVAL_US = WATCHER_INTERVAL_MS * 1000;

    struct HandleWrapper {
        int handle = -1;

        HandleWrapper() = default;

        explicit HandleWrapper(int h) : handle(h) {}

        HandleWrapper(HandleWrapper const &) = delete;

        HandleWrapper(HandleWrapper &&hw) noexcept: handle(hw.release()) {}

        HandleWrapper &operator=(HandleWrapper const &) = delete;

        HandleWrapper &operator=(HandleWrapper &&) = delete;

        int release() {
            int tmp = handle;
            handle = -1;
            return tmp;
        }

        ~HandleWrapper() {
            if (handle != -1) {
                // on Linux handle is always closed after calling close() and getting
                // errno == EINTR, regardless POSIX standard
                close(handle);
            }
        }
    };

    void writer(HandleWrapper &&wrapper, std::string const &in) {
        ssize_t bytesWritten;

        for (size_t pos = 0; pos < in.size();) {
            auto chunkSize = std::min(in.size(), pos + BATCH_SIZE) - pos;

            bytesWritten = write(wrapper.handle, &in.c_str()[pos], chunkSize);

            if (bytesWritten == -1) {
                if (errno == EINTR && !terminal::interrupted()) {
                    // try again
                    continue;
                } else {
                    // error occurred or interrupted
                    return;
                }
            } else {
                pos += (size_t) bytesWritten;
            }
        }
    }

    void reader(HandleWrapper &&wrapper, std::string &out) {
        char buffer[BATCH_SIZE];
        ssize_t bytesRead;

        while (true) {
            bytesRead = read(wrapper.handle, buffer, BATCH_SIZE);
            if (bytesRead == -1) {
                if (errno == EINTR && !terminal::interrupted()) {
                    // try again
                    continue;
                } else {
                    // error occurred or interrupted
                    break;
                }
            } else if (bytesRead == 0) {
                // EOF
                break;
            }
            out.append(buffer, bytesRead);
        }
    }

    bool warmup(pid_t pid) {
        int status;

        // WARM-UP
        // execution of this block should not take a long time.
        // (waiting before child process is ready to execvp)

        while (true) {
            while (waitpid(pid, &status, 0) == -1) {
                // do not use relaxed reading,
                // because this waitpid is blocking, and
                // you mightn't have another chance to exit
                if ((errno != 0 && errno != EINTR)
                    || terminal::interrupted(std::memory_order_seq_cst)) {

                    if (errno != 0 && errno != EINTR) {
                        terminal::syncOutput(
                                "[!] Syscall waitpid failed at warm-up, error ", errno, '\n');
                    }
                    // if waitpid failed or interrupted, kill process
                    kill(pid, SIGKILL);
                    while (waitpid(pid, &status, 0), errno != ECHILD); // todo: dirty?
                    return false;
                }
            }

            // here process stopped or exited

            if (WIFSTOPPED(status)) {
                if (WSTOPSIG(status) == SIGSTOP) {
                    // caught self-risen signal to avoid ambiguity.
                    // set options to die if stress dies and to catch execvp.
                    ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL | PTRACE_O_TRACEEXEC);
                    ptrace(PTRACE_CONT, pid, 0, 0);

                } else if (status >> 8 == (SIGTRAP | PTRACE_EVENT_EXEC << 8)) {
                    // we have stopped just after execvp.
                    // avoid further traceexec events.
                    ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL);
                    // break to continue debugging
                    return true;

                } else {
                    // do not allow forked process to receive other signals
                    ptrace(PTRACE_CONT, pid, 0, 0);
                }
            } else {
                // forked process died
                if (WIFSIGNALED(status)) {
                    terminal::syncOutput(
                            "[!] Forked process died (signal ", WTERMSIG(status), ")\n");
                } else {
                    terminal::syncOutput(
                            "[!] Forked process died (errno ", WEXITSTATUS(status), ")\n");
                }
                return false;
            }
        }
    }

    bool debugger(execution_result &result, units::unit const &unit, pid_t pid) {
        if (!warmup(pid)) {
            return false;
        }

        // MONITORING
        // declare some stuff and do debugger work

        // to distinguish stack overflow and general access violation
        // we need to associate each thread with its stack area.
        std::unordered_map<pid_t, std::optional<maps::entry>> stacks;
        std::optional<maps> mappings_opt;

        struct user_regs_struct regs;
        memset(&regs, 0, sizeof(regs));

        // read registers
        ptrace(PTRACE_GETREGS, pid, 0, &regs);

        // read sections from /proc/pid/maps
        mappings_opt = proc_parser::get_maps(pid);

        if (mappings_opt.has_value()) {
            maps const& maps = mappings_opt.value();
            // add main thread stack
            stacks[pid] = maps.getSectionByAddr(regs.register_sp);
        }

        // store temporary signal info here.
        siginfo_t siginfo;
        memset(&siginfo, 0, sizeof(siginfo));

        // associate received bad signals with some state
        std::unordered_map<int, error_info> snapshots;

        // save threads' pid here.
        std::unordered_set<pid_t> threads{pid};

        // TRACE CLONE seems to be the only interesting option.
        // TRACE FORKS and VFORKS are not, because after them
        // the user must call exec not to break memory, probably.
        ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL | PTRACE_O_TRACECLONE);

        // todo: what about exec?

        // start
        ptrace(PTRACE_CONT, pid, 0, 0);
        bool terminatedByWatcher = false;
        int status;

        // time counting
        using namespace std::chrono;
        auto start = steady_clock::now(); // start timer
        size_t elapsed;

        // memory counting (kilobytes)
        struct rusage rusage{};

        while (true) {
            pid_t child;

            // wait4 is not standardized on Linux
            while ((child = wait4(-1, &status, WNOHANG, &rusage)) == 0) {
                elapsed = duration_cast<milliseconds>(steady_clock::now() - start).count();

                if (terminal::interrupted()
                    || (unit.timeLimit != 0 && elapsed > unit.timeLimit)
                    || (unit.memoryLimit != 0 && rusage.ru_maxrss * 1024ull > unit.memoryLimit)) { // todo: avoid multiplication
                    terminatedByWatcher = true;
                    for (auto victim: threads) {
                        kill(victim, SIGKILL);
                    }
                    break;
                }

                usleep(WATCHER_INTERVAL_US);
            }

            if (errno == EINTR) {
                // wait4 interrupted by a signal, continue
                continue;
            } else if (errno == ECHILD) {
                // process has been terminated, break
                // it should never enter this branch btw.
                break;
            } else if (WIFSTOPPED(status)) {
                // tracee was stopped, let's check a signal.
                // WIFSTOPPED != 0, so it isn't SIGKILL, so process is alive.
                int signal = WSTOPSIG(status);

                if (signal != SIGTRAP) {
                    if (signal != SIGSTOP && signal != SIGCONT) {
                        error_info &info = snapshots[signal];

                        // fill signal info
                        ptrace(PTRACE_GETSIGINFO, child, 0, &siginfo);
                        info.setSigInfo(siginfo);

                        // on SIGSEGV additional info is available
                        if (signal == SIGSEGV) {
                            ptrace(PTRACE_GETREGS, child, 0, &regs);

                            // we should update mappings, because stacks grow,
                            // and old top bounds are already invalid.
                            mappings_opt = proc_parser::get_maps(pid);

                            // update thread stack area
                            if (mappings_opt.has_value() && stacks[child].has_value()) {
                                stacks[child] = mappings_opt.value()
                                        .getExactSectionByEndAddr(stacks[child].value().end);
                            }

                            info.setInstructionPointer(regs.register_ip);
                            info.setStackSection(regs.register_sp, stacks[child]);
                            info.setMappings(std::move(mappings_opt));
                        }
                    }

                    // send back what it caught
                    ptrace(PTRACE_CONT, child, 0, WSTOPSIG(status));

                } else {
                    if (status >> 8 == (SIGTRAP | PTRACE_EVENT_CLONE << 8)) {
                        // new thread created.
                        // let's get its pid and stack.
                        pid_t new_thread;
                        ptrace(PTRACE_GETEVENTMSG, child, 0, &new_thread);
                        ptrace(PTRACE_GETREGS, new_thread, 0, &regs);
                        threads.insert(new_thread);

                        // update mappings
                        // todo: could be optimized not to store all maps
                        mappings_opt = proc_parser::get_maps(pid);

                        // add thread stack
                        if (mappings_opt.has_value()) {
                            stacks[new_thread] = mappings_opt.value()
                                    .getSectionByAddr(regs.register_sp);
                        }

                        // resume new thread
                        ptrace(PTRACE_CONT, new_thread, 0, 0);
                    }
                    // resume thread from SIGTRAP
                    ptrace(PTRACE_CONT, child, 0, 0);
                }
            } else if (WIFEXITED(status)) {
                // process exited normally, check return code
                if (!result.error.hasErrorInfo()) {
                    result.error.storeExitCode(WEXITSTATUS(status));
                }
                threads.erase(child);
                if (threads.empty()) {
                    // wait for all the threads
                    break;
                }
            } else if (WIFSIGNALED(status)) {
                // process terminated by a signal
                int signal = WTERMSIG(status);

                // if not killed manually, set error code and additional info
                if ((!terminatedByWatcher || signal != SIGKILL) && !result.error.hasErrorInfo()) {
                    result.error.storeErrCode(signal);

                    if (snapshots.count(signal)) {
                        result.error.storeErrInfo(snapshots[signal]);
                    }
                } // otherwise, process was killed manually
                threads.erase(child);
                if (threads.empty()) {
                    // wait for all the threads
                    break;
                }
            } else {
                // another error
                terminal::syncOutput(
                        "[!] Debugger failed, error ", errno, '\n');
            }
        }

        // prevent 0 ms in stats
        if (elapsed == 0) {
            elapsed = duration_cast<milliseconds>(steady_clock::now() - start).count();
        }

        result.time = elapsed;
        size_t bytes = (size_t) rusage.ru_maxrss * 1024ul;
        result.memory = bytes < (size_t) rusage.ru_maxrss ? SIZE_MAX : bytes;
        return true;
    }

}

namespace invoker {

    const char EXEC_EXT[] = "";
    const char SHELL_EXT[] = ".sh";

    void initializer::customInit(exec_rules &, comp_rules &, substitutions_map &) {}
}

namespace invoker {
    bool execute(runtime_config const &cfg,
                 units::unit const &unit,
                 std::string const &in,
                 std::string &out,
                 std::string &err,
                 execution_result &result) {

        // pipe[0] - reading, pipe[1] - writing
        HandleWrapper STDERR_PIPE[2];
        HandleWrapper STDOUT_PIPE[2];
        HandleWrapper STDIN_PIPE[2];
        int pipesTmp[2];

        if (pipe(pipesTmp) < 0) {
            terminal::syncOutput(
                    "[!] Execution preparing failed, error ", errno, '\n');
            return false;
        }

        STDIN_PIPE[0].handle = pipesTmp[0];
        STDIN_PIPE[1].handle = pipesTmp[1];

        if (pipe(pipesTmp) < 0) {
            terminal::syncOutput(
                    "[!] Execution preparing failed, error ", errno, '\n');
            return false;
        }

        STDOUT_PIPE[0].handle = pipesTmp[0];
        STDOUT_PIPE[1].handle = pipesTmp[1];

        if (pipe(pipesTmp) < 0) {
            return false;
        }

        STDERR_PIPE[0].handle = pipesTmp[0];
        STDERR_PIPE[1].handle = pipesTmp[1];

        // prepare cmdline
        command cmd = getExecutionCommand(cfg, unit);
        assert(!cmd.empty());
        std::vector<char *> args(cmd.size() + 1, nullptr);

        for (size_t i = 0; i < cmd.size(); ++i) {
            args[i] = cmd[i].data();
        }

        pid_t pid;

        switch (pid = fork()) {
            case -1: {
                terminal::syncOutput(
                        "[!] Can't create a process, error ", errno, '\n');
                return false;
            }

            case 0: {
                // child's code
                ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
                raise(SIGSTOP);

                close(STDERR_PIPE[0].release());
                close(STDOUT_PIPE[0].release());
                close(STDIN_PIPE[1].release());

                // no need to catch EINTR (seems to work atomically)
                dup2(STDERR_PIPE[1].handle, STDERR_FILENO);
                dup2(STDOUT_PIPE[1].handle, STDOUT_FILENO);
                dup2(STDIN_PIPE[0].handle, STDIN_FILENO);

                close(STDERR_PIPE[1].release());
                close(STDOUT_PIPE[1].release());
                close(STDIN_PIPE[0].release());

                execvp(args[0], args.data());

                // couldn't exec: avoid cleaning tasks, just direct exit
                _exit(errno);
            }

            default: {
                // parent's code
                close(STDIN_PIPE[0].release());
                close(STDOUT_PIPE[1].release());
                close(STDERR_PIPE[1].release());

                std::thread writerInstance(writer, std::move(STDIN_PIPE[1]), std::ref(in));
                std::thread readerInstance(reader, std::move(STDOUT_PIPE[0]), std::ref(out));
                std::thread errReaderInstance(reader, std::move(STDERR_PIPE[0]), std::ref(err));

                bool ret = debugger(result, unit, pid);

                writerInstance.join();
                readerInstance.join();
                errReaderInstance.join();

                return ret;
            }

        }
    }

    namespace utils {
        bool compile(
                runtime_config const &cfg,
                units::unit &unit,
                path &&compiledPath,
                command const &cmd) {
            assert(!cmd.empty());
            std::vector<char *> args(cmd.size() + 1, nullptr);

            for (size_t i = 0; i < cmd.size(); ++i) {
                args[i] = const_cast<char *>(cmd[i].data());
            }

            path stressFolder = cfg.workingDirectory / "stress";
            path cacheFolder = stressFolder / "cache";

            using namespace std::filesystem; // todo: why adl failed?

            // todo: remove copypaste
            if (exists(compiledPath) && cfg.useCached.count(unit.cat)) {
                unit.file = std::move(compiledPath);
                terminal::syncOutput("[*] Use cached ", unit.category(), "\n");
                return true; // have cached one
            }

            std::error_code errCode;

            // mkdir ${CWD}/stress
            if (!exists(stressFolder) && !create_directory(stressFolder, errCode)) {
                terminal::syncOutput("[!] Can't create ", stressFolder.string(), '\n');
                return false;
            }

            // mkdir ${CWD}/stress/cache
            if (!exists(cacheFolder) && !create_directory(cacheFolder, errCode)) {
                terminal::syncOutput("[!] Can't create ", cacheFolder.string(), '\n');
                return false;
            }

            if (exists(compiledPath)) {
                terminal::syncOutput("[*] Recompile ", unit.category(), '\n');
            } else {
                terminal::syncOutput("[*] Compile ", unit.category(), '\n');
            }

            pid_t pid;

            switch (pid = fork()) {
                case -1: {
                    terminal::syncOutput(
                            "[!] Can't create a process, error ", errno, '\n');
                    return false;
                }

                case 0: {
                    // child's code
                    execvp(args[0], args.data());

                    // couldn't exec: avoid cleaning tasks, just direct exit
                    _exit(errno);
                }

                default: {
                    // parent's code
                    int status;

                    while (waitpid(pid, &status, 0) == -1) {
                        // do not use relaxed memory order, because this waitpid is blocking
                        // you don't have another chance to exit
                        if ((errno != 0 && errno != EINTR)
                            || terminal::interrupted(std::memory_order_seq_cst)) {
                            if (errno != 0 && errno != EINTR) {
                                terminal::syncOutput(
                                        "[!] Syscall waitpid failed, error ", errno, '\n');
                            }
                            kill(pid, SIGKILL);
                            while (waitpid(pid, &status, 0), errno != ECHILD); // todo: dirty?
                            return false;
                        }
                    }

                    if (WIFEXITED(status)) {
                        unit.file = compiledPath;
                        if (WEXITSTATUS(status) == ENOENT) { // todo: dirty
                            terminal::syncOutput("[!] Compiler not found\n");
                        }
                        return (WEXITSTATUS(status) == 0);
                    }
                    return false;
                }
            }
        }
    }

}