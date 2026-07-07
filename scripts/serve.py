Import("env")

# Registers a `serve` custom target so `pio run -t serve` builds the viewer and then launches it. The target depends
# on the compiled program node, so PlatformIO builds (or rebuilds only what changed) before running. The program
# serves the preview at http://127.0.0.1:8420 and runs until you stop it with Ctrl-C.

program = env.subst("$BUILD_DIR/${PROGNAME}")

env.AddTarget(
    name="serve",
    dependencies=program,
    actions=[program],
    title="Serve",
    description="Build then run the preview server (http://127.0.0.1:8420)",
    always_build=True,
)
