#pragma once

#include <string>
#include <csignal>
#include "sys/user.h"
#include "linux/core/stat.h"
#include "linux/core/maps.h"

struct error_info {

    void set(siginfo_t const&, user_regs_struct const&);
    void set(struct stat &&);
    void set(struct maps &&);

    std::string explanation() const;

private:
    bool empty = true;

    int si_signo;
    int si_errno;
    int si_code;
    unsigned long _si_addr;

    unsigned long stackPointer;
    unsigned long instructionPointer;

    stat stat;
    maps maps;
};