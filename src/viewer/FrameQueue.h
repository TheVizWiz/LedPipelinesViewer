#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <vector>

namespace viewer {
	// One rendered frame: the pixels of every registered strip, grouped per strip (matching the library's per-strip
	// populateFastLed loop). Each pixel is a packed 0xRRGGBB value.
	struct Frame {
		std::vector<std::vector<uint32_t> > strips;
	};

	// A coalescing single-slot mailbox between the render loop (producer) and any number of SSE client threads
	// (consumers). publish() overwrites the stored latest frame and bumps a sequence number, so a slow or absent
	// browser only ever causes frames to be dropped - never buffered unboundedly, and never blocking the render loop.
	// Consumers wait for the sequence to advance past the last one they delivered, then take a snapshot.
	class FrameQueue {
	public:
		static FrameQueue &instance();

		// Producer side: store the latest frame and wake all waiting consumers. Cheap and non-blocking.
		void publish(Frame frame);

		// Consumer side: block until seq advances beyond lastSeq, then copy out the latest frame and return its seq.
		// lastSeq is updated in place so the caller can pass it back on the next call.
		uint64_t waitForNext(uint64_t &lastSeq, Frame &out);

	private:
		FrameQueue() = default;

		std::mutex mutex;
		std::condition_variable cond;
		Frame latest;
		uint64_t seq = 0;
	};
}
