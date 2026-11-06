#include "mpe/faust_loader.h"
#include <dlfcn.h>
#include <sys/stat.h>
#include <cerrno>
#include <fstream>
#include <map>
#include <memory>
#include <vector>
#include "faust/dsp/llvm-dsp.h"
#include "faust/gui/meta.h"
#include "mpe/engine.h"

namespace mpe {

namespace {

bool makeDirs(const std::string& path) {
    for (size_t i = 1; i <= path.size(); i++) {
        if (i == path.size() || path[i] == '/') {
            std::string part = path.substr(0, i);
            if (mkdir(part.c_str(), 0755) != 0 && errno != EEXIST) return false;
        }
    }
    return true;
}

// libfaust carries its own copy of LLVM. Linked normally, its LLVM symbols would shadow those
// of the graphics driver's LLVM (Mesa compiles shaders with it on AMD GPUs), which then
// crashes. So it is loaded privately (RTLD_LOCAL | RTLD_DEEPBIND), through its C entry points.
struct Faust {
    using Create = llvm_dsp_factory* (*)(const char*, const char*, int, const char**, const char*, char*, int);
    using Delete = bool (*)(llvm_dsp_factory*);
    Create create = nullptr;
    Delete destroy = nullptr;
    std::string error;
    Faust() {
        void* h = nullptr;
        for (const char* name : {"libfaust.so.2", "libfaust.so"})
            if ((h = dlopen(name, RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND))) break;
        if (!h) { error = std::string("libfaust: ") + dlerror(); return; }
        create = reinterpret_cast<Create>(dlsym(h, "createCDSPFactoryFromString"));
        destroy = reinterpret_cast<Delete>(dlsym(h, "deleteCDSPFactory"));
        if (!create || !destroy) { error = "libfaust: no createCDSPFactoryFromString"; create = nullptr; }
    }
};
Faust& faust() { static Faust f; return f; }

struct Factory {
    llvm_dsp_factory* f = nullptr;
    ~Factory() { if (f) faust().destroy(f); }
};

// The same options the compiled-in instruments are built with, so they sound the same.
std::shared_ptr<Factory> compile(const std::string& name, const std::string& source, const std::string& libDir,
                                 const char* process, std::string* error) {
    if (!faust().create) { if (error) *error = faust().error; return nullptr; }
    std::vector<const char*> argv = {"-I", libDir.c_str(), "-vec", "-vs", "32"};
    if (process) { argv.push_back("-pn"); argv.push_back(process); }
    char err[4096] = {0};
    auto out = std::make_shared<Factory>();
    out->f = faust().create(name.c_str(), source.c_str(), int(argv.size()), argv.data(), "", err, -1);
    if (!out->f) { if (error) *error = name + ": " + err; return nullptr; }
    return out;
}

}  // namespace

int loadInstrumentPackage(const Package& pkg, const std::string& cacheDir, std::string* error) {
    auto fail = [&](const std::string& why) { if (error) *error = why; return -1; };
    if (!pkg.isInstrument()) return fail(pkg.id() + ": not an instrument package");
    const Json& m = pkg.manifest;
    if (m["contract"].str("mpe-voice/1") != "mpe-voice/1") return fail(pkg.id() + ": contract " + m["contract"].str() + " is not known");
    const std::string& source = pkg.file("instrument.dsp");
    if (source.empty()) return fail(pkg.id() + ": no instrument.dsp");

    // Its libraries, as files: <cache>/<id>@<version>/lib/*.lib
    std::string safe = pkg.id() + "@" + pkg.version();
    for (char& c : safe) if (c == '/') c = '_';
    const std::string libDir = cacheDir + "/" + safe + "/lib";
    if (!makeDirs(libDir)) return fail("cannot make " + libDir);
    for (const auto& kv : pkg.files) {
        if (kv.first.rfind("lib/", 0) != 0 || kv.first.find("..") != std::string::npos) continue;
        std::ofstream(libDir + "/" + kv.first.substr(4), std::ios::binary) << kv.second;
    }

    auto voice = compile(pkg.id(), source, libDir, nullptr, error);
    if (!voice) return -1;
    std::shared_ptr<Factory> post;
    if (m["post"].boolean()) {
        post = compile(pkg.id() + " (post)", source, libDir, "post", error);
        if (!post) return -1;
    }

    // Its declarations, as the compiled-in instruments give them.
    auto declared = std::make_shared<std::map<std::string, std::string>>();
    {
        struct Collect : Meta {
            std::map<std::string, std::string>& out;
            explicit Collect(std::map<std::string, std::string>& o) : out(o) {}
            void declare(const char* k, const char* v) override { out[k] = v; }
        } collect(*declared);
        std::unique_ptr<dsp> probe(voice->f->createDSPInstance());
        if (!probe) return fail(pkg.id() + ": no instance");
        probe->metadata(&collect);
    }

    InstrumentInfo info;
    info.id = pkg.id();
    info.group = m["tags"][size_t(0)].str("more");
    info.name = m["name"].str(pkg.id());
    info.description = m["description"].str();
    info.version = pkg.version();
    for (size_t i = 0; i < m["aliases"].size(); i++) info.aliases.push_back(m["aliases"][i].str());
    info.create = [voice]() -> dsp* { return voice->f->createDSPInstance(); };
    if (post) info.createPost = [post]() -> dsp* { return post->f->createDSPInstance(); };
    info.meta = [declared](const char* key) -> const char* {
        auto it = declared->find(key);
        return it == declared->end() ? "" : it->second.c_str();
    };
    return addInstrument(std::move(info));
}

}  // namespace mpe
