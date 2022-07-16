#pragma once

#include "linux/core/stat.h"
#include "linux/core/maps.h"
#include <csignal>
#include <optional>

struct proc_parser {
    static std::optional<stat> get_stat(pid_t pid);
    static std::optional<maps> get_maps(pid_t pid);
};
