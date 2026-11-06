// The table of instruments: those compiled in (listed by the generated builtins.cpp), then any
// added while running (instrument packages compiled by libfaust).
#include <deque>
#include <mutex>
#include "mpe/engine.h"

namespace mpe {
void builtinInstruments(std::vector<InstrumentInfo>& out);   // generated (CMakeLists.txt)

namespace {
std::mutex& lock() { static std::mutex m; return m; }
// A deque: adding never moves an entry, so a reference handed out stays good.
std::deque<InstrumentInfo>& table() {
    static std::deque<InstrumentInfo>* t = [] {
        auto* d = new std::deque<InstrumentInfo>();
        std::vector<InstrumentInfo> v;
        builtinInstruments(v);
        for (auto& i : v) d->push_back(std::move(i));
        return d;
    }();
    return *t;
}
}  // namespace

int instrumentCount() {
    std::lock_guard<std::mutex> g(lock());
    return int(table().size());
}

const InstrumentInfo& instrumentAt(int index) {
    std::lock_guard<std::mutex> g(lock());
    return table().at(size_t(index));
}

int addInstrument(InstrumentInfo info) {
    std::lock_guard<std::mutex> g(lock());
    auto& t = table();
    // The same id again (a package loaded twice): the newer one replaces it in finds, both stay valid.
    t.push_back(std::move(info));
    return int(t.size()) - 1;
}

int findInstrument(const char* id) {
    if (!id) return -1;
    std::lock_guard<std::mutex> g(lock());
    auto& t = table();
    for (int i = int(t.size()) - 1; i >= 0; i--) {     // the latest added first
        if (t[size_t(i)].id == id) return i;
        for (const auto& a : t[size_t(i)].aliases) if (a == id) return i;
    }
    return -1;
}

}  // namespace mpe
