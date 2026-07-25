Import("env")

import os
import subprocess
import sys

# Registers a `serve` custom target: `pio run -t serve` builds the C++ backend, launches the Vite dev server (which
# opens the browser and serves the React UI on :5173, proxying the SSE streams to the backend on :8420), then runs the
# backend in the foreground. Ctrl-C stops the backend and tears the Vite server down with it.
#
# The UI is no longer served by C++ - it is a separate Vite/React app under web/. Run `npm install` in web/ once before
# the first serve (see web/README or the top-level README); this script does NOT install dependencies.

project_dir = env.subst("$PROJECT_DIR")
web_dir = os.path.join(project_dir, "web")
program = env.subst("$PROGPATH")


def serve(*_args, **_kwargs):
    node_modules = os.path.join(web_dir, "node_modules")
    if not os.path.isdir(node_modules):
        sys.stderr.write(
            "\n[serve] web/node_modules not found. Run `npm install` in the web/ directory first:\n"
            "          cd web && npm install\n\n"
        )
        env.Exit(1)

    # Start Vite in the background. It serves the UI and opens the browser (server.open in vite.config.ts). npm is
    # resolved from PATH; shell=False keeps args explicit and cross-platform-ish (npm.cmd on Windows is handled by
    # PlatformIO users invoking through a shell if needed).
    npm = "npm.cmd" if sys.platform.startswith("win") else "npm"
    print("[serve] starting Vite dev server (web/) …")
    vite = subprocess.Popen([npm, "run", "dev"], cwd=web_dir)

    try:
        # Run the C++ backend in the foreground. It serves the SSE streams the Vite frontend connects to. Blocks until
        # the user stops it (Ctrl-C).
        print("[serve] starting C++ backend: " + program)
        subprocess.call([program])
    finally:
        # Tear down Vite when the backend exits (or on Ctrl-C), so no orphaned dev server is left running.
        print("\n[serve] stopping Vite dev server …")
        vite.terminate()
        try:
            vite.wait(timeout=5)
        except subprocess.TimeoutExpired:
            vite.kill()


# always_build + a dependency on the program node so PlatformIO compiles (or incrementally rebuilds) the backend before
# `serve` runs, exactly as the old target did.
env.AddTarget(
    name="serve",
    dependencies=program,
    actions=[serve],
    title="Serve",
    description="Build the backend, launch the Vite UI (:5173), and run the backend (:8420)",
    always_build=True,
)
