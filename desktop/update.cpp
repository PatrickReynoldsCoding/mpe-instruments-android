#include "update.h"
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include "mpe/package.h"

extern char** environ;
namespace fs = std::filesystem;

namespace {
const char* kRepo = "PatrickReynoldsCoding/odo";
const char* kTagPrefix = "desktop-v";
const char* kAssetSuffix = "-linux-x86_64.tar.gz";

// Runs a program and waits; its output to `out` (a file) if given. True if it exited 0.
bool spawn(const std::vector<std::string>& args, const std::string& out = "") {
    std::vector<char*> argv;
    for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    if (!out.empty()) posix_spawn_file_actions_addopen(&fa, 1, out.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    posix_spawn_file_actions_addopen(&fa, 2, "/dev/null", O_WRONLY, 0);
    pid_t pid;
    int rc = posix_spawnp(&pid, argv[0], &fa, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&fa);
    if (rc != 0) return false;
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return false;
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

std::string slurp(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
}  // namespace

Updater::Updater(std::string currentVersion, std::string dir) : current_(std::move(currentVersion)), dir_(std::move(dir)) {}

Updater::~Updater() { if (worker_.joinable()) worker_.join(); }

std::string Updater::version() const { std::lock_guard<std::mutex> g(lock_); return version_; }
std::string Updater::message() const { std::lock_guard<std::mutex> g(lock_); return message_; }

void Updater::fail(const std::string& why) {
    { std::lock_guard<std::mutex> g(lock_); message_ = why; }
    state_ = State::Failed;
}

void Updater::run(void (Updater::*job)()) {
    if (worker_.joinable()) worker_.join();
    worker_ = std::thread([this, job] { (this->*job)(); });
}

void Updater::check() {
    State s = state_;
    if (s == State::Checking || s == State::Downloading || s == State::Installed) return;
    state_ = State::Checking;
    run(&Updater::doCheck);
}

void Updater::install() {
    if (state_ != State::Available) return;
    state_ = State::Downloading;
    run(&Updater::doInstall);
}

// "0.0.10" > "0.0.9": number by number; anything after a '-' is ignored.
bool Updater::newer(const std::string& candidate, const std::string& current) {
    auto parts = [](const std::string& v) {
        std::vector<long> out;
        std::stringstream ss(v.substr(0, v.find('-')));
        std::string p;
        while (std::getline(ss, p, '.')) out.push_back(std::strtol(p.c_str(), nullptr, 10));
        return out;
    };
    auto a = parts(candidate), b = parts(current);
    for (size_t i = 0; i < std::max(a.size(), b.size()); i++) {
        long x = i < a.size() ? a[i] : 0, y = i < b.size() ? b[i] : 0;
        if (x != y) return x > y;
    }
    return false;
}

void Updater::doCheck() {
    const std::string tmp = dir_ + "/.update";
    std::error_code ec;
    fs::create_directories(tmp, ec);
    const std::string list = tmp + "/releases.json";
    if (!spawn({"curl", "-fsSL", "--max-time", "20", "-H", "Accept: application/vnd.github+json", "-A", "odo-desktop/" + current_,
              std::string("https://api.github.com/repos/") + kRepo + "/releases?per_page=30"}, list))
        return fail("Could not reach GitHub");
    std::string err;
    mpe::Json releases = mpe::Json::parse(slurp(list), &err);
    if (releases.type != mpe::Json::Array) return fail("GitHub's answer was not understood");
    std::string best, url, digest;
    for (size_t i = 0; i < releases.size(); i++) {
        const mpe::Json& r = releases[i];
        std::string tag = r["tag_name"].str();
        if (r["draft"].boolean() || r["prerelease"].boolean() || tag.rfind(kTagPrefix, 0) != 0) continue;
        std::string v = tag.substr(std::string(kTagPrefix).size());
        if (!best.empty() && !newer(v, best)) continue;
        const mpe::Json& assets = r["assets"];
        for (size_t k = 0; k < assets.size(); k++) {
            std::string name = assets[k]["name"].str();
            if (name.size() > std::string(kAssetSuffix).size() &&
                name.compare(name.size() - std::string(kAssetSuffix).size(), std::string::npos, kAssetSuffix) == 0) {
                best = v;
                url = assets[k]["browser_download_url"].str();
                digest = assets[k]["digest"].str();
            }
        }
    }
    std::lock_guard<std::mutex> g(lock_);
    if (!best.empty() && newer(best, current_)) {
        version_ = best; url_ = url; digest_ = digest;
        message_ = "Update to " + best;
        state_ = State::Available;
    } else {
        message_ = "Up to date";
        state_ = State::UpToDate;
    }
}

void Updater::doInstall() {
    std::string url, digest, version;
    { std::lock_guard<std::mutex> g(lock_); url = url_; digest = digest_; version = version_; message_ = "Downloading " + version + "..."; }
    const std::string tmp = dir_ + "/.update", tarball = tmp + "/odo.tar.gz", fresh = tmp + "/new";
    std::error_code ec;
    fs::remove_all(fresh, ec);
    fs::create_directories(fresh, ec);
    if (!spawn({"curl", "-fsSL", "--max-time", "600", "-o", tarball, url})) return fail("The download failed");
    // GitHub gives each file's SHA-256: the download must match it.
    if (digest.rfind("sha256:", 0) != 0) return fail("GitHub gave no checksum for the download");
    if (!spawn({"sha256sum", tarball}, tmp + "/sum.txt")) return fail("Could not check the download");
    if (slurp(tmp + "/sum.txt").substr(0, 64) != digest.substr(7)) return fail("The download is damaged (checksum)");
    if (!spawn({"tar", "-xzf", tarball, "-C", fresh, "--strip-components=1"})) return fail("Could not unpack the download");
    if (!fs::exists(fresh + "/odo") || !fs::is_directory(fresh + "/library")) return fail("The download is not odo");
    // Swap: the running odo keeps its old file open, so replacing it is safe.
    const std::string old = tmp + "/old";
    fs::remove_all(old, ec);
    fs::create_directories(old, ec);
    fs::rename(dir_ + "/odo", old + "/odo", ec);
    if (ec) return fail("Could not replace odo: " + ec.message());
    fs::rename(fresh + "/odo", dir_ + "/odo", ec);
    if (ec) { fs::rename(old + "/odo", dir_ + "/odo", ec); return fail("Could not replace odo"); }
    fs::rename(dir_ + "/library", old + "/library", ec);
    fs::rename(fresh + "/library", dir_ + "/library", ec);
    for (const char* extra : {"README.txt", "odo.svg", "install.sh"}) {
        fs::remove(dir_ + "/" + extra, ec);
        fs::rename(fresh + "/" + extra, dir_ + "/" + extra, ec);
    }
    fs::remove(tarball, ec);
    std::lock_guard<std::mutex> g(lock_);
    message_ = "Installed " + version + ": tap to restart";
    state_ = State::Installed;
}

bool Updater::restart(char** argv) {
    if (state_ != State::Installed) return false;
    std::string exe = dir_ + "/odo";
    argv[0] = const_cast<char*>(exe.c_str());
    execv(exe.c_str(), argv);
    fail("Could not start the new odo");
    return false;
}
