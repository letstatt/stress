#define INVOKER_MACROS

#include "invoker.h"

#undef INVOKER_MACROS

#include "terminal.h"
#include "units/unit.h"
#include "core/run.h"
#include "core/runtime_config.h"
#include "win/core/error_info.h"

#include <windows.h>
#include <memoryapi.h>
#include <psapi.h>
#include <thread>
#include <optional>
#include <cassert>

#undef min

namespace {
    constexpr size_t BATCH_SIZE = 256;
    constexpr size_t WATCHER_INTERVAL_MS = 5;

    struct HandleWrapper {
        HANDLE handle = INVALID_HANDLE_VALUE;

        HandleWrapper() = default;

        explicit HandleWrapper(HANDLE h) : handle(h) {}

        HandleWrapper(HandleWrapper const &) = delete;

        HandleWrapper(HandleWrapper &&hw) noexcept: handle(hw.release()) {}

        HandleWrapper &operator=(HandleWrapper const &) = delete;

        HandleWrapper &operator=(HandleWrapper &&) = delete;

        HANDLE release() {
            HANDLE tmp = handle;
            handle = INVALID_HANDLE_VALUE;
            return tmp;
        }

        ~HandleWrapper() {
            if (handle != INVALID_HANDLE_VALUE) {
                CloseHandle(handle);
            }
        }
    };

    void writer(HandleWrapper &&wrapper, std::string const &in) {
        DWORD dwWritten = 0;

        for (size_t pos = 0; pos < in.size(); pos += dwWritten) {
            auto chunkSize = std::min(in.size(), pos + BATCH_SIZE) - pos;

            if (!WriteFile(wrapper.handle, &in.c_str()[pos], chunkSize, &dwWritten, nullptr)) {
                //oh, OK, do whatever you want
                break;
            }

            if (terminal::interrupted()) {
                break;
            }
        }
    }

    void reader(HandleWrapper &&wrapper, std::string &out) {
        CHAR buffer[BATCH_SIZE];
        DWORD dwRead;

        while (true) {
            auto success = ReadFile(wrapper.handle, &buffer, BATCH_SIZE, &dwRead, nullptr);
            if (!success || dwRead == 0) {
                //oh, OK, do whatever you want
                break;
            }
            if (terminal::interrupted()) {
                break;
            }
            out.append(buffer, dwRead);
        }
    }

    void watcher(HANDLE hProcess, HANDLE hThread, units::unit const &unit, execution_result &result) {
        using namespace std::chrono;

        // we created a suspended process, so lets resume it
        ResumeThread(hThread);

        // memory counting
        PROCESS_MEMORY_COUNTERS memCounters;
        ZeroMemory(&memCounters, sizeof(memCounters));

        // time counting
        auto start = steady_clock::now();
        size_t elapsed = 0;

        while (!terminal::interrupted()
               && (unit.timeLimit == 0 || elapsed <= unit.timeLimit)
               && (unit.memoryLimit == 0 || memCounters.PeakWorkingSetSize <= unit.memoryLimit)) {

            if (WaitForSingleObject(hProcess, WATCHER_INTERVAL_MS) == WAIT_OBJECT_0) {
                break; // process is dead
            }

            elapsed = duration_cast<milliseconds>(steady_clock::now() - start).count();
            GetProcessMemoryInfo(hProcess, &memCounters, sizeof(memCounters));
        }

        // prevent 0 ms in stats
        if (elapsed == 0) {
            elapsed = duration_cast<milliseconds>(steady_clock::now() - start).count();
            GetProcessMemoryInfo(hProcess, &memCounters, sizeof(memCounters));
        }

        // if process exceeded limits, but still works, kill it
        if (terminal::interrupted()
            || (elapsed > unit.timeLimit && unit.timeLimit != 0)
            || (memCounters.PeakWorkingSetSize > unit.memoryLimit && unit.memoryLimit != 0)) {
            TerminateProcess(hProcess, 0);
        }

        result.memory = memCounters.PeakWorkingSetSize;
        result.time = elapsed;
    }

    DWORD debugger(HANDLE hProcess, error_info& errInfo) {
        MEMORY_BASIC_INFORMATION mb_info;
        memset(&mb_info, 0, sizeof(MEMORY_BASIC_INFORMATION));
        DWORD exceptionCode = 0;
        DEBUG_EVENT event;
        bool debugging = true;

        while (debugging) {
            DWORD dwContinueStatus = DBG_CONTINUE;
            if (!WaitForDebugEvent(&event, INFINITE)) {
                break;
            }
            switch (event.dwDebugEventCode) {
                case CREATE_PROCESS_DEBUG_EVENT:
                    CloseHandle(event.u.CreateProcessInfo.hFile);
                    break;
                case LOAD_DLL_DEBUG_EVENT:
                    CloseHandle(event.u.LoadDll.hFile);
                    break;
                case EXCEPTION_DEBUG_EVENT:
                    exceptionCode = event.u.Exception.ExceptionRecord.ExceptionCode;

                    // hardware breakpoint, skip
                    if (exceptionCode == EXCEPTION_BREAKPOINT) {
                        exceptionCode = 0;
                        break;
                    }

                    // real exceptions (probably)
                    dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
                    if (!event.u.Exception.dwFirstChance) {
                        // store additional info
                        if (exceptionCode == EXCEPTION_ACCESS_VIOLATION) {
                            if (VirtualQueryEx(
                                    hProcess,
                                    reinterpret_cast<void*>(event.u.Exception.ExceptionRecord.ExceptionInformation[1]),
                                    &mb_info,
                                    sizeof(MEMORY_BASIC_INFORMATION)) != 0) {
                                errInfo.storeMemoryInfo(mb_info);
                            }

                            errInfo.storeAccessViolation(
                                    event.u.Exception.ExceptionRecord.ExceptionInformation[0],
                                    event.u.Exception.ExceptionRecord.ExceptionInformation[1]);
                        }
                        TerminateProcess(hProcess, 0);
                    } else {
                        exceptionCode = 0;
                    }
                    break;
                case EXIT_PROCESS_DEBUG_EVENT:
                case RIP_EVENT:
                    debugging = false;
                    break;
            }
            ContinueDebugEvent(
                    event.dwProcessId,
                    event.dwThreadId,
                    dwContinueStatus);
        }
        return exceptionCode;
    }
}

namespace invoker {

    const char EXEC_EXT[] = ".exe";
    const char SHELL_EXT[] = ".bat";

    void initializer::customInit(exec_rules & executor, comp_rules & compiler, substitutions_map &) {
        addRules(executor,
                 E_RULE(".py", "python", "${PATH}"));

        addRules(compiler,
                 C_RULE(".kt", "${CACHE_DIR}/${FILENAME}.jar", "cmd", "/C", "kotlinc", "${PATH}",
                        "-include-runtime", "-d", "${CACHE_DIR}/${FILENAME}.jar"));
    }
}

namespace invoker {

    bool execute(runtime_config const &cfg,
                 units::unit const &unit,
                 std::string const &in,
                 std::string &out,
                 std::string &err,
                 execution_result &result) {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        SECURITY_ATTRIBUTES saAttr;

        ZeroMemory(&saAttr, sizeof(saAttr));
        ZeroMemory(&pi, sizeof(pi));
        ZeroMemory(&si, sizeof(si));

        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = nullptr;

        HandleWrapper STDIN_READ;
        HandleWrapper STDIN_WRITE;
        HandleWrapper STDOUT_READ;
        HandleWrapper STDOUT_WRITE;
        HandleWrapper STDERR_READ;
        HandleWrapper STDERR_WRITE;

        if (!CreatePipe(&STDOUT_READ.handle, &STDOUT_WRITE.handle, &saAttr, 0)
            || !CreatePipe(&STDIN_READ.handle, &STDIN_WRITE.handle, &saAttr, 0)
            || !CreatePipe(&STDERR_READ.handle, &STDERR_WRITE.handle, &saAttr, 0)
            || !SetHandleInformation(STDERR_READ.handle, HANDLE_FLAG_INHERIT, 0)
            || !SetHandleInformation(STDOUT_READ.handle, HANDLE_FLAG_INHERIT, 0)
            || !SetHandleInformation(STDIN_WRITE.handle, HANDLE_FLAG_INHERIT, 0)) {
            terminal::syncOutput("[!] Execution preparing failed, error ", GetLastError(), '\n');
            return false;
        }

        si.cb = sizeof(si);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = STDOUT_WRITE.handle;
        si.hStdInput = STDIN_READ.handle;
        si.hStdError = STDERR_WRITE.handle;

        std::stringstream stream;
        command cmd = getExecutionCommand(cfg, unit);
        assert(!cmd.empty());

        stream << std::quoted(cmd[0]);
        for (size_t i = 1; i < cmd.size(); ++i) {
            stream << " " << std::quoted(cmd[i]);
        }
        std::string cmdline = stream.str(); // not const because of CreateProcessA

        DWORD creationFlags = 0;
        creationFlags |= CREATE_SUSPENDED; // to measure time more accurately
        creationFlags |= DEBUG_ONLY_THIS_PROCESS; // use debugger to prevent WER and dumps
        creationFlags |= CREATE_DEFAULT_ERROR_MODE; // just in case...

        if (!CreateProcessA(
                nullptr, cmdline.data(), nullptr, nullptr, TRUE, creationFlags, nullptr, nullptr, &si, &pi)) {
            terminal::syncOutput("[!] Can't create a process, error ", GetLastError(), '\n');
            return false;
        }

        CloseHandle(STDIN_READ.release());
        CloseHandle(STDOUT_WRITE.release());
        CloseHandle(STDERR_WRITE.release());

        std::thread writerInstance(writer, std::move(STDIN_WRITE), std::ref(in));
        std::thread readerInstance(reader, std::move(STDOUT_READ), std::ref(out));
        std::thread errReaderInstance(reader, std::move(STDERR_READ), std::ref(err));
        std::thread watcherInstance(watcher, pi.hProcess, pi.hThread, std::ref(unit), std::ref(result));

        error_info errInfo;

        DWORD exceptionCode = debugger(pi.hProcess, errInfo);

        writerInstance.join();
        readerInstance.join();
        errReaderInstance.join();
        watcherInstance.join();

        // process is dead here

        if (exceptionCode) {
            result.error.storeErrCode(exceptionCode);
            result.error.storeErrInfo(errInfo);

        } else {
            DWORD exitCode = 0;
            if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
                // but if failed, then never mind
                result.error.storeExitCode(exitCode);
            }
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }

    namespace utils {
        bool compile(
                runtime_config const &cfg,
                units::unit &unit,
                path &&compiledPath,
                command const &cmd) {
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            SECURITY_ATTRIBUTES saAttr;

            ZeroMemory(&saAttr, sizeof(saAttr));
            ZeroMemory(&pi, sizeof(pi));
            ZeroMemory(&si, sizeof(si));

            saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
            saAttr.bInheritHandle = FALSE;
            saAttr.lpSecurityDescriptor = nullptr;

            std::stringstream stream;
            assert(!cmd.empty());

            stream << std::quoted(cmd[0]);
            for (size_t i = 1; i < cmd.size(); ++i) {
                stream << " " << std::quoted(cmd[i]);
            }
            std::string cmdline = stream.str();

            path stressFolder = "stress";
            path cacheFolder = stressFolder / "cache";

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

            if (!CreateProcessA(
                    nullptr, cmdline.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
                terminal::syncOutput("[!] Can't create a process, error ", GetLastError(), '\n');
                return false;
            }

            WaitForSingleObject(pi.hProcess, INFINITE);

            DWORD exitCode = 0;
            GetExitCodeProcess(pi.hProcess, &exitCode);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            if (exitCode == 0) {
                unit.file = std::move(compiledPath);
                return true;

            } else {
                return false;
            }
        }
    }
}
