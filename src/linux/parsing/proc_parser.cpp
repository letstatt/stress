#include "linux/parsing/proc_parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>

std::optional<stat> proc_parser::get_stat(pid_t pid) {
    std::ifstream f("/proc/" + std::to_string(pid) + "/stat");
    if (!f.is_open()) {
        return std::nullopt;
    }
    std::string content;
    std::getline(f, content); // get all the content
    if (f.fail()) {
        return std::nullopt;
    }
    // f.eof() should be true
    f.close();

    size_t i = content.find_last_of(')');
    if (i == std::string::npos) {
        return std::nullopt;
    }

    // skip fields
    size_t skip = 24;
    for (; i < content.size(); ++i) {
        if (content[i] == ' ' && --skip == 0) {
            break;
        }
    }

    stat s{};
    ++i;

    if (i < content.size() && skip == 0) {
        while (i < content.size() && content[i] != ' ') {
            s.startcode *= 10;
            s.startcode += content[i++] - '0';
        }
        ++i;
        if (i < content.size()) {
            while (i < content.size() && content[i] != ' ') {
                s.endcode *= 10;
                s.endcode += content[i++] - '0';
            }
            return s;
        }
    }

    return std::nullopt;
}

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
                while (i < line.size() && line[i] != ' ') {
                    e.permissions += line[i++];
                }
                if (i < line.size()) {
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
