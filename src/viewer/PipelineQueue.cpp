#include "viewer/PipelineQueue.h"

#include <utility>

namespace viewer {
	PipelineQueue &PipelineQueue::instance() {
		static PipelineQueue queue;
		return queue;
	}

	void PipelineQueue::publish(std::string json) {
		{
			std::lock_guard<std::mutex> lock(mutex);
			latest = std::move(json);
			seq++;
		}
		cond.notify_all();
	}

	uint64_t PipelineQueue::waitForNext(uint64_t &lastSeq, std::string &out) {
		std::unique_lock<std::mutex> lock(mutex);
		cond.wait(lock, [&] { return seq != lastSeq; });
		out = latest;
		lastSeq = seq;
		return seq;
	}
}
