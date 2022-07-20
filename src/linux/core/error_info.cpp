#include "linux/core/error_info.h"
#include <sstream>

void error_info::setSigInfo(const siginfo_t &siginfo) {
    si_signo = siginfo.si_signo;
    si_code = siginfo.si_code;
    si_errno = siginfo.si_errno;

    switch (si_signo) {
        case SIGILL:
        case SIGFPE:
        case SIGSEGV:
        case SIGBUS:
            _si_addr = reinterpret_cast<unsigned long>(siginfo.si_addr);
        default:
            break;
    }

    m_empty = false;
}

void error_info::setInstructionPointer(unsigned long ip) {
    instructionPointer = ip;
}

void error_info::setStackSection(unsigned long sp, std::optional<maps::entry> entry) {
    stackPointer = sp;
    stackEntry = std::move(entry);
}

void error_info::setMappings(std::optional<maps>&& m_opt) {
    mappings_opt = std::move(m_opt);
}

bool error_info::empty() const {
    return m_empty;
}

std::string error_info::explanation() const {
    if (m_empty) {
        return {};
    }

    std::string prefix;
    auto fmt = [&prefix, this](std::string const& s) {
        std::stringstream str;
        str << prefix;
        if (!s.empty()) {
            str << ", " << s;
        } else {
            str << ", si_code " << si_code;
        }
        str << ", si_addr " << std::hex << _si_addr;
        return str.str();
    };

    if (si_signo == SIGSEGV) {
        // nullptr
        if (_si_addr == 0) {
            return "nullptr dereference";
        }
        // stack overflow (can be both SEGV_MAPPER and SEGV_ACCERR if guard pages are presented)
        if (stackEntry.has_value()) {
            maps::entry const &entry = stackEntry.value();
            if (entry.start - stackPointer <= __WORDSIZE && stackPointer <= entry.end
                && _si_addr < entry.start && entry.start - _si_addr <= __WORDSIZE) {
                return "stack overflow";
            }
        }

        if (si_code == SEGV_MAPERR) {
            // general page fault
            /*std::stringstream str;
            str << "segfault: page not mapped\n\n";
            if (stackEntry.has_value()) {
                maps::entry const &entry = stackEntry.value();
                str << "stack bounds: " << std::hex << entry.start << " " << entry.end << '\n';
            } else {
                str << "stack bounds are unavailable\n";
            }
            str << "stack pointer: " << stackPointer << '\n';
            str << "si_addr: " << _si_addr << '\n';
            return str.str();*/
            return "segfault: page not mapped";

        } else if (si_code == SEGV_ACCERR) {
            // check if violated segment is available.
            if (mappings_opt.has_value()) {
                maps const& maps = mappings_opt.value();
                std::optional<maps::entry> entry_opt = maps.getSectionByAddr(_si_addr);

                if (entry_opt.has_value()) {
                    maps::entry const& entry = entry_opt.value();

                    if (_si_addr == instructionPointer) {
                        // execution failure.
                        // if readable and writable, but not executable -> data execution attempt.
                        if (entry.permissions[0] && entry.permissions[1] && !entry.permissions[2]) {
                            return "data execution attempt";
                        }
                    } else {
                        // read-write failure.
                        // check if read-only memory violated.
                        if (entry.permissions[0] && !entry.permissions[1]) {
                            return "write to read-only memory";
                        }
                    }
                }
            }
            // general access violation
            return "segfault: memory permissions violation";
        }
        return "segfault: reason is unknown";

    } else if (si_signo == SIGFPE) {
        switch (si_code) {
            case FPE_INTDIV:
                return "integer division by zero";
            case FPE_INTOVF:
                return "integer overflow";
            case FPE_FLTDIV:
                return "floating-point division by zero";
            case FPE_FLTOVF:
                return "floating-point overflow";
            case FPE_FLTUND:
                return "floating-point underflow";
            case FPE_FLTRES:
                return "floating-point inexact result";
            case FPE_FLTINV:
                return "floating-point invalid operation";
            case FPE_FLTSUB:
                return "arithmetic exception: subscript out of range";
            default:
                return "arithmetic exception";
        }

    } else if (si_signo == SIGILL) {
        prefix = "SIGILL";
        switch (si_code) {
            case ILL_ILLOPC:
                return fmt("illegal opcode");
            case ILL_ILLOPN:
                return fmt("illegal operand");
            case ILL_ILLADR:
                return fmt("illegal addressing mode");
            case ILL_ILLTRP:
                return fmt("illegal trap");
            case ILL_PRVOPC:
                return fmt("privileged opcode");
            case ILL_PRVREG:
                return fmt("privileged register");
            case ILL_COPROC:
                return fmt("coprocessor error");
            case ILL_BADSTK:
                return fmt("internal stack error");
            default:
                return fmt("");
        }

    } else if (si_signo == SIGBUS) {
        prefix = "SIGBUS";
        switch (si_code) {
            case BUS_ADRALN:
                return fmt("invalid address alignment");
            case BUS_ADRERR:
                return fmt("nonexistent physical address");
            case BUS_OBJERR:
                return fmt("object specific hardware error");
            default:
                return fmt("");
        }
    }

    return {};
}