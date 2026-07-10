Import("env")

# Windows-only build adjustments for the native previewer, applied before compilation.
#
# The viewer builds with the GCC/Clang flag set everywhere (-std=gnu++2a, -pthread). On Windows we target MinGW-w64
# (GCC), so those flags carry over unchanged - the one thing POSIX gives for free but Windows does not is the sockets
# library. cpp-httplib (vendor/httplib.h) calls WSAStartup and the winsock2 API; under MSVC it self-links ws2_32 via
# `#pragma comment(lib, ...)`, but MinGW honors no such pragma, so we must link ws2_32 (and wsock32) explicitly.
#
# On macOS/Linux this script is a no-op: sockets live in libc, so there is nothing to add.

import sys


def is_windows_host():
    # PlatformIO runs this on the build host; the native platform compiles for that same host. sys.platform is
    # "win32" on Windows (both CPython and the bundled PlatformIO Python), which is what we key off.
    return sys.platform.startswith("win")


if is_windows_host():
    # Winsock: ws2_32 provides WSAStartup / socket / send / recv; wsock32 covers a few legacy symbols some MinGW
    # builds still reference. Order matters for the GNU linker (libraries after the objects that use them), and
    # AddLinkFlags appends, so httplib's object files precede these on the link line.
    env.Append(LIBS=["ws2_32", "wsock32"])
    print("windows_flags.py: linking ws2_32 + wsock32 for MinGW winsock support")
