#include "linux/core/maps.h"

void maps::add(entry&& e) {
    entries.emplace(std::move(e));
}

std::set<maps::entry> const& maps::getEntries() const {
    return entries;
}

std::optional<maps::entry> maps::getSectionByAddr(unsigned long addr) const {
    entry e;
    e.start = addr;

    auto it = entries.lower_bound(e);
    if (it != entries.end()) {
        maps::entry const &entry = *it;
        if (entry.in(addr)) {
            return entry;
        }
    }
    if (it != entries.begin()) {
        maps::entry const &entry = *(--it);
        if (entry.in(addr)) {
            return entry;
        }
    }
    return std::nullopt;
}

std::optional<maps::entry> maps::getExactSectionByEndAddr(unsigned long addr) const {
    entry e;
    e.start = addr;

    // [0,1];[1,3];[4,5]
    // wanna find [,3]
    // getSectionByAddr found [1,3] - ok

    // [0,1];[1,3];[3,5]
    // wanna find [,3]
    // getSectionByAddr found [3,5] - wrong, must be [1,3]

    auto it = entries.lower_bound(e);
    if (it != entries.begin()) {
        maps::entry const &entry = *(--it);
        if (entry.end == addr) {
            return entry;
        }
    }
    return std::nullopt;
}
