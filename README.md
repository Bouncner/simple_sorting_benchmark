Nothing fancy to see here. Just some rough measurements and plotting.
We assume you are using a folder called `rel` for the CMake build.

Code mostly written by @Alexander-Dubrawski. Kudos!

Currently, we don't store which compiler is used. On MacOS, we usually use clang and on Linux GCC. Thus can be stark differences in the performance of `std::sort` due to different implementations in the stdlib.
