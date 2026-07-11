Import("env")

# Registers a `serve` custom target so `pio run -t serve` builds the viewer and then launches it. The target depends
# on the compiled program node, so PlatformIO builds (or rebuilds only what changed) before running. The program
# serves the preview at http://127.0.0.1:8420 and runs until you stop it with Ctrl-C.

# Use $PROGPATH, not "$BUILD_DIR/$PROGNAME": PROGPATH is the full path to the built executable WITH the platform's
# extension (program.exe on Windows, bare `program` on macOS/Linux). Building the path from PROGNAME drops the .exe, so
# on Windows the dependency node never matches the real artifact and SCons fails with "program not found ... needed by
# target serve".
program = env.subst("$PROGPATH")

env.AddTarget(
    name="serve",
    dependencies=program,
    actions=[program],
    title="Serve",
    description="Build then run the preview server (http://127.0.0.1:8420)",
    always_build=True,
)
