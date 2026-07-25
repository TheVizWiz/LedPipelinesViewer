Import("env")

# Pre-build setup for the native previewer, applied on every platform before compilation. Three jobs:
#
# 1. Compile the out-of-tree stubs (stubs/stubs_impl.cpp), which define the FastLED/Serial globals and the real-clock
#    timing functions (millis/micros/delay). These live in stubs/ - a sibling of src/, NOT under it - because the
#    headers there deliberately shadow the library's FastLED.h/Arduino.h (via -I stubs). Selecting them through
#    build_src_filter with a "+<../stubs>" glob is unreliable: reaching outside src/ with ".." works on macOS/clang
#    but silently compiles nothing on Windows/MinGW, leaving millis()/micros() undefined at link time. Adding the file
#    explicitly by absolute path via BuildSources() is deterministic on every platform.
#
# 2. On Windows, link the winsock libraries. cpp-httplib (vendor/httplib.h) calls WSAStartup and the winsock2 API;
#    under MSVC it self-links ws2_32 via `#pragma comment(lib, ...)`, but MinGW honors no such pragma, so ws2_32 (and
#    wsock32) must be linked explicitly. No-op on macOS/Linux, where sockets live in libc.
#
# 3. Always force pipeline.cpp to recompile by deleting its stale object. pipeline.cpp is the file you edit to define
#    your effect (the viewer's analog of an Arduino sketch), so it changes constantly. The build's up-to-date check can
#    be fooled into skipping its recompile - e.g. a restore/checkout that gives the source an mtime OLDER than the
#    existing object leaves the previous pipeline linked into the binary. Deleting the object each pre-build guarantees
#    the running binary always reflects the current pipeline.cpp. The cost is one file's recompile per build (~1s), which
#    is negligible for the one file that changes most.

import os
import sys

project_dir = env.subst("$PROJECT_DIR")
build_dir = env.subst("$BUILD_DIR")

# Job 1: compile stubs/stubs_impl.cpp regardless of host. Absolute source path + a build path under $BUILD_DIR so the
# object lands with the rest of the build output.
stub_src = os.path.join(project_dir, "stubs", "stubs_impl.cpp")
env.BuildSources(os.path.join(build_dir, "stubs"), os.path.join(project_dir, "stubs"))
print("build_setup.py: compiling out-of-tree stubs from " + os.path.join(project_dir, "stubs"))

# Job 2: Windows-only winsock linkage.
if sys.platform.startswith("win"):
    env.Append(LIBS=["ws2_32", "wsock32"])
    print("build_setup.py: linking ws2_32 + wsock32 for MinGW winsock support")

# Job 3: delete the stale pipeline.cpp object so it always recompiles. Guarded so a missing object (clean build) is
# fine. The path mirrors where the LDF places src/ objects: $BUILD_DIR/src/pipeline.o.
pipeline_obj = os.path.join(build_dir, "src", "pipeline.o")
if os.path.exists(pipeline_obj):
    os.remove(pipeline_obj)
    print("build_setup.py: removed stale " + pipeline_obj + " to force pipeline.cpp recompile")
