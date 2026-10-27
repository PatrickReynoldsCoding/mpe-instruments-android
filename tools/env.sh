# Source this for a shell that can build and run the app on this machine:
#   source tools/env.sh
# Paths are where the toolchain was installed locally; change them for another machine.
export JAVA_HOME="$HOME/.local/jdk/jdk-17.0.21+9"
export ANDROID_HOME="$HOME/Android/Sdk"
export PATH="$JAVA_HOME/bin:$HOME/.local/gradle/gradle-8.9/bin:$ANDROID_HOME/platform-tools:$ANDROID_HOME/emulator:$ANDROID_HOME/cmdline-tools/latest/bin:$PATH"
