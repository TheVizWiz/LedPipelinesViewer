#include "viewer/FrameQueue.h"

namespace viewer {
	FrameQueue &FrameQueue::instance() {
		static FrameQueue queue;
		return queue;
	}

	void FrameQueue::publish(Frame frame) {
		{
			std::lock_guard<std::mutex> lock(mutex);
			latest = std::move(frame);
			seq++;
		}
		cond.notify_all();
	}

	uint64_t FrameQueue::waitForNext(uint64_t &lastSeq, Frame &out) {
		std::unique_lock<std::mutex> lock(mutex);
		cond.wait(lock, [&] { return seq != lastSeq; });
		out = latest;
		lastSeq = seq;
		return seq;
	}
}
