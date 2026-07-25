#pragma once

namespace viewer {
	// Start the preview data backend on 127.0.0.1:<port> on a background thread and return immediately. It serves two
	// Server-Sent Events streams and no HTML: GET /stream (rendered pixels, from the global FrameQueue) and
	// GET /pipeline (the pipeline's serialized structure + live state, from the global PipelineQueue). The UI is a
	// separate Vite/React app (web/) that connects to these streams. Call this once before the render loop.
	void startServer(int port);
}
