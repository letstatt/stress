#include "linux/parsing/proc_parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>

std::optional<maps> proc_parser::get_maps(pid_t pid) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/maps");
    if (!f.is_open()) {
        return std::nullopt;
    }
    std::string line;
    std::string word;
    maps m{};
    while (std::getline(f, line), !line.empty()) {
        // parse addresses
        size_t i = 0;
        maps::entry e{};
        while (i < line.size() && line[i] != '-') {
            e.start *= 16;
            if (line[i] <= '9') {
                e.start += line[i++] - '0';
            } else {
                e.start += line[i++] - 'a' + 10;
            }
        }
        ++i;
        if (i < line.size()) {
            while (i < line.size() && line[i] != ' ') {
                e.end *= 16;
                if (line[i] <= '9') {
                    e.end += line[i++] - '0';
                } else {
                    e.end += line[i++] - 'a' + 10;
                }
            }
            ++i;
            if (i < line.size()) {
                static char permissions[] = {'r', 'w', 'x', 'p'};
                for (int j = 0; j < 4 && i < line.size(); ++i, ++j) {
                    e.permissions[j] = (line[i] == permissions[j]);
                }
                if (i < line.size() && line[i] == ' ') {
                    m.add(std::move(e));
                    continue;
                }
            }
        }
        // otherwise some bad things happened, attempt failed
        return std::nullopt;
    }
    f.close();
    return m;
}
