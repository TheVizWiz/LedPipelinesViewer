#pragma once

namespace viewer {
	// Start the preview HTTP server on 127.0.0.1:<port> on a background thread and return immediately. Serves the
	// viewer page at GET / and a Server-Sent Events pixel stream at GET /stream. Frames are pulled from the global
	// FrameQueue, so nothing needs to be wired up beyond calling this once before the render loop.
	void startServer(int port);
}
