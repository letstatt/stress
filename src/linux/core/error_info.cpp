#include "linux/core/error_info.h"
#include <iostream> // todo: for debug only

void error_info::set(const siginfo_t &siginfo, const user_regs_struct & registers) {
    si_signo = siginfo.si_signo;
    si_code = siginfo.si_code;
    si_errno = siginfo.si_errno;

    if (si_signo == SIGSEGV) {
        _si_addr = reinterpret_cast<unsigned long>(siginfo.si_addr);
    }

#ifdef __x86_64__
    stackPointer = registers.rsp;
    instructionPointer = registers.rip;
#else
#error "this architecture is not supported"
#endif

    empty = false;
}

void error_info::set(struct stat &&s) {
    stat = std::move(s);
}

void error_info::set(struct maps &&m) {
    maps = std::move(m);
}

std::string error_info::explanation() const {
    if (empty) {
        return {};
    }

    if (si_signo == SIGSEGV) {
        if (si_code == SEGV_MAPERR) {
            // nullptr
            if (_si_addr == 0) {
                return "nullptr dereference";
            }
            // stack overflow
            {
                maps::entry e;
                e.start = stackPointer;
                auto const &entries = maps.getEntries();
                auto it = entries.lower_bound(e);
                if (it != entries.end()) {
                    maps::entry const &entry = *it;
                    //std::cout << entry.start << " " << entry.end << " " << stackPointer << " " << _si_addr << std::endl;

                    // case 1: stack pointer and si_addr are out of stack area (or sp in on the edge)
                    if (stackPointer <= entry.start && entry.start - stackPointer <= __WORDSIZE
                        && _si_addr < entry.start && entry.start - _si_addr <= __WORDSIZE) {
                        return "stack overflow";
                    }
                    // case 2: stack pointer is in, si_addr is out
                    if (it != entries.begin()) {
                        maps::entry const &entry = *(--it);
                        //std::cout << entry.start << " " << entry.end << " " << stackPointer << " " << _si_addr << std::endl;
                        if (entry.start <= stackPointer && stackPointer <= entry.end
                        && _si_addr < entry.start && entry.start - _si_addr <= __WORDSIZE) {
                            return "stack overflow";
                        }
                    }
                }
            }
            // general page fault
            return "SEGV_MAPERR, unavailable memory address";
        } else if (si_code == SEGV_ACCERR) {
            // no execution permission
            std::cout << stat.startcode << " " << stat.endcode << " " << instructionPointer << std::endl;
            if (instructionPointer < stat.startcode || instructionPointer > stat.endcode) {
                return "data execution attempt";
            }
            // read-only memory
            {
                maps::entry e;
                e.start = _si_addr;
                auto const &entries = maps.getEntries();
                auto it = entries.lower_bound(e);
                if (it != entries.end() && it->start == _si_addr) {
                    maps::entry const &entry = *it;
                    bool writable = entry.permissions.find('w') != std::string::npos;
                    bool readable = entry.permissions.find('r') != std::string::npos;
                    if (readable && !writable) {
                        return "write to read-only memory";
                    }
                }
                if (it != entries.begin()) {
                    maps::entry const &entry = *(--it);
                    if (entry.start < _si_addr && entry.end >= _si_addr) {
                        bool writable = entry.permissions.find('w') != std::string::npos;
                        bool readable = entry.permissions.find('r') != std::string::npos;
                        if (readable && !writable) {
                            return "write to read-only memory";
                        }
                    }
                }
            }
            // general access violation
            return "SEGV_ACCERR, memory permissions violation";
        }
        return {};
    }

    return {};
}