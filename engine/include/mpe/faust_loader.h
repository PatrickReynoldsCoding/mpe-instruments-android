// Instruments from packages while running: compiled with libfaust (LLVM) to native code, as fast
// as the compiled-in ones. For computers (MPE_LIBFAUST); phones compile their packages in.
#pragma once
#include <string>
#include "mpe/package.h"

namespace mpe {

// Compile an instrument package and add it to the instrument table. Its libraries are written
// under cacheDir (Faust reads imports from files). Slow for big instruments (a second or two):
// call off the audio thread. Returns its index, or -1 and *error.
int loadInstrumentPackage(const Package& pkg, const std::string& cacheDir, std::string* error = nullptr);

}  // namespace mpe
