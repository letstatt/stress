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
#include "linux/parsing/proc_parser.h"

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

    bool debugger(execution_result &result, units::unit const &unit, pid_t pid) {
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

                } else if ((status >> 16) == PTRACE_EVENT_EXEC) {
                    // caught execvp event.
                    // avoid further traceexec events.
                    ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL);
                    // break to continue debugging
                    break;

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

        // MONITORING
        // watcher is running, do debugger work

#ifndef __x86_64__
#error "this architecture is not supported yet."
#endif

        std::unordered_map<int, error_info> snapshots;
        // no guarantees for correctness if SA_NODEFER was set in the tracee

        siginfo_t lastCaughtSignal;
        struct user_regs_struct registers{}; // x86_64 only

        std::optional<stat> stat_opt;
        std::optional<maps> maps_opt;

        // todo: prepare code for multithreaded processes
        // current implementation detects errors well
        // only if the main thread handles signals.
        //ptrace(PTRACE_SETOPTIONS, pid, 0, );

        // start
        ptrace(PTRACE_CONT, pid, 0, 0);
        bool terminatedByWatcher = false;
        bool work = true;

        // time counting
        using namespace std::chrono;
        auto start = steady_clock::now(); // start timer
        size_t elapsed;

        // memory counting (kilobytes)
        struct rusage rusage{};

        while (work) {
            // wait4 is not standardized on Linux (!)
            while (wait4(pid, &status, WNOHANG, &rusage) == 0) {
                elapsed = duration_cast<milliseconds>(steady_clock::now() - start).count();

                if (terminal::interrupted()
                    || (unit.timeLimit != 0 && elapsed > unit.timeLimit)
                    || (unit.memoryLimit != 0 && rusage.ru_maxrss * 1024ull > unit.memoryLimit)) { // todo: avoid multiplication
                    // notice: the signal will be intercepted by a tracer
                    terminatedByWatcher = true;
                    kill(pid, SIGKILL);
                    work = false;
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
                    /*ptrace(PTRACE_GETSIGINFO, pid, 0, &lastCaughtSignal);
                    ptrace(PTRACE_GETREGS, pid, 0, &registers);
                    long w = ptrace(PTRACE_PEEKTEXT, pid, registers.rip, 0);
                    terminal::syncOutput("[!] word ", w, " ", *((unsigned long*) &w), '\n');
                    terminal::pause();*/
                    if (signal == SIGSEGV) {
                        if (!stat_opt.has_value()) {
                            // should be loaded once
                            stat_opt = proc_parser::get_stat(pid);
                        }
                        if (!maps_opt.has_value()) {
                            // should be loaded once on single-thread app
                            // if multithreaded, should be reloaded everytime :_(
                            maps_opt = proc_parser::get_maps(pid);
                        }
                        ptrace(PTRACE_GETSIGINFO, pid, 0, &lastCaughtSignal);
                        ptrace(PTRACE_GETREGS, pid, 0, &registers);
                        if (!snapshots.count(SIGSEGV)) {
                            snapshots.emplace(SIGSEGV, error_info{});
                        }
                        error_info & info = snapshots.at(SIGSEGV);
                        info.set(lastCaughtSignal, registers);
                        if (maps_opt.has_value()) {
                            info.set(std::move(maps_opt.value()));
                            maps_opt = std::nullopt;
                        }
                    }
                }

                // send back what it caught
                // if signal kills process, will lastCaughtSignal
                // and registers remain the same? todo: ?
                ptrace(PTRACE_CONT, pid, 0, WSTOPSIG(status));
                continue;
            } else if (WIFEXITED(status)) {
                // process exited normally, check return code
                result.error.storeExitCode(WEXITSTATUS(status));
                break;
            } else if (WIFSIGNALED(status)) {
                // process terminated by a signal
                int signal = WTERMSIG(status);

                // if not killed manually, set error code
                if (!terminatedByWatcher || signal != SIGKILL) {
                    result.error.storeErrCode(signal);

                    /*error_info info{};
                    info.set(lastCaughtSignal, registers);

                    terminal::syncOutput("[!] si_code ", lastCaughtSignal.si_code, '\n');
                    terminal::syncOutput("[!] si_errno ", lastCaughtSignal.si_errno, '\n');

                    terminal::syncOutput("[!] rsp ", registers.rsp, '\n');
                    terminal::syncOutput("[!] rip ", registers.rip, '\n');*/

                    if (signal == SIGSEGV) {
                        //terminal::syncOutput("[!] si_addr ", lastCaughtSignal.si_addr, '\n');
                        if (snapshots.count(SIGSEGV)) {
                            error_info & info = snapshots.at(SIGSEGV);
                            if (stat_opt.has_value()) {
                                info.set(std::move(stat_opt.value()));
                            }
                            result.error.storeErrInfo(info);

                        }
                    }
                } // otherwise, process was killed manually
                break;
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