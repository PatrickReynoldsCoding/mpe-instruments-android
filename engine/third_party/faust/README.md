# Faust architecture headers

Copied unchanged from Faust 2.81.10 (`/usr/include/faust` from the Ubuntu `faust-common`
package). The engine needs these six headers to wrap the C++ that `faust` generates.

They are kept in the repo, not taken from the system, because the Android build is a
cross-compile: putting the build machine's `/usr/include` on the include path breaks it.

Licence: GNU LGPL 2.1 or later, with the Faust architecture exception, which allows a larger
work that includes them to be distributed under terms of your choice. See the header of each file.

The `faust` compiler itself is still needed on the build machine, to turn each `.dsp` into C++.
