#pragma once

#include <set>

struct maps {

    struct entry {
        unsigned long start = 0;
        unsigned long end = 0;
        std::string permissions{};
        // offset, device_id, inode, map_name omitted

        bool operator < (entry const& e) const {
            return start < e.start;
        }
    };

    inline void add(entry&& e) {
        entries.emplace(std::move(e));
    }

    inline std::set<entry> const& getEntries() const {
        return entries;
    }

private:
    std::set<entry> entries;
};
