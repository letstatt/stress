#pragma once

#include <string>
#include <csignal>
#include "sys/user.h"
#include "linux/core/maps.h"

struct error_info {

    void setSigInfo(siginfo_t const&);
    void setInstructionPointer(unsigned long ip);
    void setStackSection(unsigned long sp, std::optional<maps::entry> entry);
    void setMappings(std::optional<maps>&&);
    bool empty() const;

    std::string explanation() const;

private:
    bool m_empty = true;

    int si_signo;
    int si_errno;
    int si_code;
    unsigned long _si_addr;

    unsigned long stackPointer;
    unsigned long instructionPointer;

    std::optional<maps::entry> stackEntry;
    std::optional<maps> mappings_opt;
};