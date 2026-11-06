#include "mpe/wasm_dsp.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <vector>
#include "faust/dsp/dsp.h"
#include "faust/gui/UI.h"
#include "faust/gui/meta.h"
#include "mpe/engine.h"
#include "mpe/package.h"
#include "wasm3.h"
#include "m3_env.h"   // ResizeMemory: room for the buffers after the dsp

namespace mpe {

namespace {

// What every voice of one instrument shares: its module and its description.
struct Module {
    std::string bytes;               // wasm3 reads from these while a voice runs
    std::string name;
    int size = 0, inputs = 0, outputs = 0;
    struct Param { std::string label; int offset; float init, min, max, step; };
    std::vector<Param> params;
    std::map<std::string, std::string> meta;
};

void params(const Json& items, std::vector<Module::Param>& out) {
    for (size_t i = 0; i < items.size(); i++) {
        const Json& it = items[i];
        if (it["items"].type == Json::Array) { params(it["items"], out); continue; }
        std::string type = it["type"].str();
        if (type == "hbargraph" || type == "vbargraph") continue;
        out.push_back({it["label"].str(), int(it["index"].num()), float(it["init"].num()), float(it["min"].num()),
                       float(it["max"].num(1)), float(it["step"].num())});
    }
}

std::shared_ptr<Module> describe(const std::string& wasm, const std::string& json, std::string* error) {
    std::string err;
    Json d = Json::parse(json, &err);
    if (d.type != Json::Object) { if (error) *error = "JSON: " + err; return nullptr; }
    auto m = std::make_shared<Module>();
    m->bytes = wasm;
    m->name = d["name"].str();
    m->size = int(d["size"].num());
    m->inputs = int(d["inputs"].num());
    m->outputs = int(d["outputs"].num());
    params(d["ui"], m->params);
    const Json& meta = d["meta"];
    for (size_t i = 0; i < meta.size(); i++)
        for (const auto& kv : meta[i].o) m->meta[kv.first] = kv.second.str();
    return m;
}

// The C library functions Faust's WebAssembly imports (module "env", names with a leading _).
#define F1(n) m3ApiRawFunction(w_##n) { m3ApiReturnType(float) m3ApiGetArg(float, a) m3ApiReturn(n(a)); }
#define F2(n) m3ApiRawFunction(w_##n) { m3ApiReturnType(float) m3ApiGetArg(float, a) m3ApiGetArg(float, b) m3ApiReturn(n(a, b)); }
F1(acosf) F1(asinf) F1(atanf) F1(cosf) F1(sinf) F1(tanf) F1(expf) F1(logf) F1(log10f) F1(tanhf) F1(coshf) F1(sinhf)
F1(acoshf) F1(asinhf) F1(atanhf) F1(roundf) F1(rintf) F1(ceilf) F1(floorf) F1(sqrtf) F1(fabsf) F1(truncf)
F2(powf) F2(atan2f) F2(fmodf) F2(remainderf) F2(fminf) F2(fmaxf)
static float exp10f_(float a) { return std::pow(10.f, a); }
m3ApiRawFunction(w_exp10f) { m3ApiReturnType(float) m3ApiGetArg(float, a) m3ApiReturn(exp10f_(a)); }

struct Import { const char* name; const char* sig; M3RawCall fn; };
const Import kImports[] = {
    {"_acosf", "f(f)", w_acosf}, {"_asinf", "f(f)", w_asinf}, {"_atanf", "f(f)", w_atanf}, {"_cosf", "f(f)", w_cosf},
    {"_sinf", "f(f)", w_sinf}, {"_tanf", "f(f)", w_tanf}, {"_expf", "f(f)", w_expf}, {"_logf", "f(f)", w_logf},
    {"_log10f", "f(f)", w_log10f}, {"_tanhf", "f(f)", w_tanhf}, {"_coshf", "f(f)", w_coshf}, {"_sinhf", "f(f)", w_sinhf},
    {"_acoshf", "f(f)", w_acoshf}, {"_asinhf", "f(f)", w_asinhf}, {"_atanhf", "f(f)", w_atanhf}, {"_roundf", "f(f)", w_roundf},
    {"_rintf", "f(f)", w_rintf}, {"_ceilf", "f(f)", w_ceilf}, {"_floorf", "f(f)", w_floorf}, {"_sqrtf", "f(f)", w_sqrtf},
    {"_fabsf", "f(f)", w_fabsf}, {"_truncf", "f(f)", w_truncf}, {"_exp10f", "f(f)", w_exp10f},
    {"_powf", "f(ff)", w_powf}, {"_atan2f", "f(ff)", w_atan2f}, {"_fmodf", "f(ff)", w_fmodf},
    {"_remainderf", "f(ff)", w_remainderf}, {"_fminf", "f(ff)", w_fminf}, {"_fmaxf", "f(ff)", w_fmaxf},
};

constexpr int kChunk = 512;          // frames per call into the module; longer blocks are split

// One voice: its own wasm3 instance of the module. The dsp lives at address 0 of its memory;
// after it, the pointer tables and buffers for compute().
class WasmDsp : public ::dsp {
public:
    explicit WasmDsp(std::shared_ptr<Module> m) : m_(std::move(m)), zones_(m_->params.size()) {
        for (size_t i = 0; i < zones_.size(); i++) zones_[i] = m_->params[i].init;
        env_ = m3_NewEnvironment();
        rt_ = m3_NewRuntime(env_, 64 * 1024, nullptr);
        IM3Module mod = nullptr;
        if (!env_ || !rt_ || m3_ParseModule(env_, &mod, reinterpret_cast<const uint8_t*>(m_->bytes.data()), uint32_t(m_->bytes.size())))
            return;
        if (m3_LoadModule(rt_, mod)) { m3_FreeModule(mod); return; }
        for (const Import& im : kImports) m3_LinkRawFunction(mod, "env", im.name, im.sig, im.fn);
        m3_FindFunction(&fInit_, rt_, "init");
        m3_FindFunction(&fInstanceInit_, rt_, "instanceInit");
        m3_FindFunction(&fConstants_, rt_, "instanceConstants");
        m3_FindFunction(&fResetUI_, rt_, "instanceResetUserInterface");
        m3_FindFunction(&fClear_, rt_, "instanceClear");
        m3_FindFunction(&fCompute_, rt_, "compute");
        // The pointer tables (inputs then outputs) and the buffers, after the dsp.
        io_ = (uint32_t(m_->size) + 15u) & ~15u;
        const uint32_t channels = uint32_t(m_->inputs + m_->outputs);
        buffers_ = io_ + 4 * channels + 16;
        uint32_t need = buffers_ + channels * kChunk * 4;
        uint32_t size = 0;
        uint8_t* mem = m3_GetMemory(rt_, &size, 0);
        if (mem && size < need && !ResizeMemory(rt_, (need + 65535) / 65536)) mem = m3_GetMemory(rt_, &size, 0);
        if (!mem || size < need || !fCompute_ || !fInit_) return;
        for (uint32_t c = 0; c < channels; c++) {
            uint32_t p = buffers_ + c * kChunk * 4;
            std::memcpy(mem + io_ + 4 * c, &p, 4);
        }
        ok_ = true;
    }
    ~WasmDsp() override {
        if (rt_) m3_FreeRuntime(rt_);
        if (env_) m3_FreeEnvironment(env_);
    }

    bool ok() const { return ok_; }
    int getNumInputs() override { return m_->inputs; }
    int getNumOutputs() override { return m_->outputs; }
    void buildUserInterface(UI* ui) override {
        ui->openVerticalBox(m_->name.c_str());
        for (size_t i = 0; i < zones_.size(); i++) {
            const auto& p = m_->params[i];
            ui->addHorizontalSlider(p.label.c_str(), &zones_[i], p.init, p.min, p.max, p.step);
        }
        ui->closeBox();
    }
    int getSampleRate() override { return sr_; }
    void init(int sr) override { sr_ = sr; call2(fInit_, sr); pull(); }
    void instanceInit(int sr) override { sr_ = sr; call2(fInstanceInit_, sr); pull(); }
    void instanceConstants(int sr) override { sr_ = sr; call2(fConstants_, sr); }
    void instanceResetUserInterface() override { if (ok_ && fResetUI_) m3_CallV(fResetUI_, int32_t(0)); pull(); }
    void instanceClear() override { if (ok_ && fClear_) m3_CallV(fClear_, int32_t(0)); }
    ::dsp* clone() override { return new WasmDsp(m_); }
    void metadata(Meta* m) override { for (const auto& kv : m_->meta) m->declare(kv.first.c_str(), kv.second.c_str()); }

    void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs) override {
        if (!ok_) { for (int c = 0; c < m_->outputs; c++) std::fill(outputs[c], outputs[c] + count, 0.f); return; }
        uint32_t size = 0;
        uint8_t* mem = m3_GetMemory(rt_, &size, 0);
        for (size_t i = 0; i < zones_.size(); i++) std::memcpy(mem + m_->params[i].offset, &zones_[i], 4);
        for (int done = 0; done < count; done += kChunk) {
            int n = std::min(kChunk, count - done);
            for (int c = 0; c < m_->inputs; c++) std::memcpy(mem + buffers_ + uint32_t(c) * kChunk * 4, inputs[c] + done, size_t(n) * 4);
            if (m3_CallV(fCompute_, int32_t(0), int32_t(n), int32_t(io_), int32_t(io_ + 4 * uint32_t(m_->inputs)))) {
                for (int c = 0; c < m_->outputs; c++) std::fill(outputs[c] + done, outputs[c] + done + n, 0.f);
                continue;
            }
            mem = m3_GetMemory(rt_, &size, 0);
            for (int c = 0; c < m_->outputs; c++)
                std::memcpy(outputs[c] + done, mem + buffers_ + uint32_t(m_->inputs + c) * kChunk * 4, size_t(n) * 4);
        }
    }

private:
    void call2(IM3Function f, int sr) { if (ok_ && f) m3_CallV(f, int32_t(0), int32_t(sr)); }
    // The module's own values of the controls (after init they are the defaults).
    void pull() {
        if (!ok_) return;
        uint32_t size = 0;
        uint8_t* mem = m3_GetMemory(rt_, &size, 0);
        for (size_t i = 0; i < zones_.size(); i++) std::memcpy(&zones_[i], mem + m_->params[i].offset, 4);
    }

    std::shared_ptr<Module> m_;
    std::vector<FAUSTFLOAT> zones_;
    IM3Environment env_ = nullptr;
    IM3Runtime rt_ = nullptr;
    IM3Function fInit_ = nullptr, fInstanceInit_ = nullptr, fConstants_ = nullptr, fResetUI_ = nullptr, fClear_ = nullptr, fCompute_ = nullptr;
    uint32_t io_ = 0, buffers_ = 0;
    int sr_ = 48000;
    bool ok_ = false;
};

}  // namespace

int loadWasmInstrument(const std::string& id, const std::string& wasm, const std::string& json,
                       const std::string& postWasm, const std::string& postJson, std::string* error) {
    auto voice = describe(wasm, json, error);
    if (!voice) return -1;
    {
        WasmDsp probe(voice);
        if (!probe.ok()) { if (error) *error = id + ": wasm3 could not run it (or its memory is too small)"; return -1; }
    }
    std::shared_ptr<Module> post;
    if (!postWasm.empty()) {
        post = describe(postWasm, postJson, error);
        if (!post) return -1;
        WasmDsp probe(post);
        if (!probe.ok()) { if (error) *error = id + " (post): wasm3 could not run it"; return -1; }
    }
    InstrumentInfo info;
    info.id = id;
    info.group = "wasm";
    info.name = voice->meta.count("name") ? voice->meta["name"] : voice->name;
    info.description = voice->meta.count("description") ? voice->meta["description"] : "";
    info.create = [voice]() -> ::dsp* { return new WasmDsp(voice); };
    if (post) info.createPost = [post]() -> ::dsp* { return new WasmDsp(post); };
    info.meta = [voice](const char* key) -> const char* {
        auto it = voice->meta.find(key);
        return it == voice->meta.end() ? "" : it->second.c_str();
    };
    return addInstrument(std::move(info));
}

}  // namespace mpe
