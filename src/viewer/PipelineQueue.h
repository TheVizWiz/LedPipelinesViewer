#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>

namespace viewer {
	// A coalescing single-slot mailbox for the pipeline's serialized JSON, exactly analogous to FrameQueue but carrying
	// a string instead of pixels. The render loop publishes the pipeline's toJson(true) each frame (structure + live
	// state); any number of SSE clients on /pipeline wait for the sequence to advance and take the latest snapshot. A
	// slow or absent browser only drops intermediate snapshots - it never buffers unboundedly or blocks the render loop.
	class PipelineQueue {
	public:
		static PipelineQueue &instance();

		// Producer side: store the latest pipeline JSON and wake all waiting consumers. Cheap and non-blocking.
		void publish(std::string json);

		// Consumer side: block until seq advances beyond lastSeq, then copy out the latest JSON and return its seq.
		// lastSeq is updated in place so the caller can pass it back on the next call.
		uint64_t waitForNext(uint64_t &lastSeq, std::string &out);

	private:
		PipelineQueue() = default;

		std::mutex mutex;
		std::condition_variable cond;
		std::string latest;
		uint64_t seq = 0;
	};
}
