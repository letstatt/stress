#pragma once

#include <set>
#include <optional>

struct maps {

    struct entry {
        unsigned long start = 0;
        unsigned long end = 0;
        bool permissions[4]; // read, write, execute, private

        inline bool in(unsigned long addr) const {
            return (start <= addr) & (addr <= end);
        }

        inline bool operator < (entry const& e) const {
            return start < e.start;
        }
    };

    void add(entry&& e);

    std::set<entry> const& getEntries() const;

    std::optional<entry> getSectionByAddr(unsigned long addr) const;

    std::optional<entry> getExactSectionByEndAddr(unsigned long addr) const;

private:
    std::set<entry> entries;
};
