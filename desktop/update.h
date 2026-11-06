// Updates from inside odo for computers. The releases are on GitHub, tagged desktop-vX.Y.Z (not
// marked latest, which stays odo for Android's). Ask GitHub for the newest, download its tarball,
// check it against the SHA-256 GitHub gives, put the new odo and library in place of the old,
// and start the new one. Uses the system's curl, sha256sum and tar. The only network access the
// app has is this, to GitHub.
#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <thread>

class Updater {
public:
    enum class State { Idle, Checking, UpToDate, Available, Downloading, Installed, Failed };

    // dir: where odo is installed (the folder of the running executable).
    Updater(std::string currentVersion, std::string dir);
    ~Updater();

    void check();                   // in the background
    void install();                 // the available one, in the background
    bool restart(char** argv);      // after Installed: become the new odo (only returns if that fails)

    State state() const { return state_; }
    std::string version() const;    // the newer version found
    std::string message() const;    // a few words for the screen
    const std::string& current() const { return current_; }

    static bool newer(const std::string& candidate, const std::string& current);

private:
    void run(void (Updater::*job)());
    void doCheck();
    void doInstall();
    void fail(const std::string& why);

    std::string current_, dir_;
    std::atomic<State> state_{State::Idle};
    mutable std::mutex lock_;
    std::string version_, url_, digest_, message_;
    std::thread worker_;
};
