TinyCThread v1.2
================

https://tinycthread.github.io


About
-----

TinyCThread is a minimalist, portable, threading library for C, intended to
make it easy to create multi threaded C applications.

The library is closesly modeled after the C11 standard, but only a subset is
implemented at the moment.

See the documentation in the doc/html directory for more information.


Using TinyCThread
-----------------

To use TinyCThread in your own project, just add tinycthread.c and
tinycthread.h to your project. In your own code, do:

#include <tinycthread.h>


Building the test programs
--------------------------

From the test folder, issue one of the following commands:

Linux, Mac OS X, OpenSolaris etc:
  make   (you may need to use gmake on some systems)

Windows/MinGW:
  mingw32-make

Windows/MS Visual Studio:
  nmake /f Makefile.msvc


History
-------

v1.2 - Unreleased
  - Updated API to better match the final specification (e.g. removed mtx_try)
  - Improved Windows support, including TSS destructors
  - Added once support
  - Improved unit testing
  - Assorted bug fixes.

v1.1 - 2012.9.8
  - First release.
  - Updated API to better match the final specification (e.g. removed xtime).
  - Some functionality still missing (mtx_timedlock, TSS destructors under
    Windows, ...).

v1.0 - Never released
  - Development version based on C11 specification draft.



License
-------

Copyright (c) 2012 Marcus Geelnard
              2013-2014 Evan Nemerson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.

