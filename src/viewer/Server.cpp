#include "viewer/Server.h"

#include <atomic>
#include <cstdio>
#include <string>
#include <thread>

#include "httplib.h"

#include "viewer/FrameQueue.h"
#include "viewer/PipelineQueue.h"

namespace viewer {
	namespace {
		// Serialize a frame to the JSON the browser expects: {"seq":N,"strips":[["#rrggbb",...],...]}. Built by hand
		// (no JSON library) since it's just hex formatting.
		std::string frameToJson(uint64_t seq, const Frame &frame) {
			std::string out;
			out.reserve(16 + frame.strips.size() * 64);
			out += "{\"seq\":";
			out += std::to_string(seq);
			out += ",\"strips\":[";
			for (size_t i = 0; i < frame.strips.size(); i++) {
				if (i) out += ',';
				out += '[';
				const auto &strip = frame.strips[i];
				for (size_t j = 0; j < strip.size(); j++) {
					if (j) out += ',';
					char hex[10];
					std::snprintf(hex, sizeof(hex), "\"#%06x\"", strip[j] & 0xFFFFFF);
					out += hex;
				}
				out += ']';
			}
			out += "]}";
			return out;
		}
	}

	void startServer(int port) {
		std::thread([port] {
			auto *server = new httplib::Server();

			// This process is now a data backend only: it serves the two Server-Sent Events streams below and nothing
			// else. The UI is a separate Vite/React app (see web/) that connects to these streams (proxied in dev), so
			// there is no longer a page served from here.

			// Server-Sent Events pixel stream. Each connected browser gets its own chunked response that blocks on the
			// FrameQueue condvar until a new frame is published, then writes one `data: <json>\n\n` event. A slow or
			// gone client only stalls its own thread here - never the render loop, which just overwrites the mailbox.
			server->Get("/stream", [](const httplib::Request &, httplib::Response &res) {
				res.set_header("Cache-Control", "no-cache");
				res.set_chunked_content_provider(
					"text/event-stream",
					[](size_t /*offset*/, httplib::DataSink &sink) {
						uint64_t lastSeq = 0;
						Frame frame;
						// Loop until the client disconnects (sink.write returns false on a broken pipe).
						while (true) {
							uint64_t seq = FrameQueue::instance().waitForNext(lastSeq, frame);
							std::string event = "data: " + frameToJson(seq, frame) + "\n\n";
							if (!sink.write(event.data(), event.size())) break;
						}
						return true;
					});
			});

			// Server-Sent Events pipeline stream. Same shape as /stream but carries the pipeline's serialized JSON
			// (structure + live state, from PipelineQueue) instead of pixels. Each event is `data: <envelope>\n\n`,
			// where the envelope wraps the pipeline JSON with its sequence number: {"seq":N,"pipeline":<json>}. The
			// pipeline JSON is already valid JSON, so it is spliced in raw (not quoted). Like /stream, a slow/gone
			// client only stalls its own thread, never the render loop.
			server->Get("/pipeline", [](const httplib::Request &, httplib::Response &res) {
				res.set_header("Cache-Control", "no-cache");
				res.set_chunked_content_provider(
					"text/event-stream",
					[](size_t /*offset*/, httplib::DataSink &sink) {
						uint64_t lastSeq = 0;
						std::string json;
						while (true) {
							uint64_t seq = PipelineQueue::instance().waitForNext(lastSeq, json);
							std::string event =
								"data: {\"seq\":" + std::to_string(seq) + ",\"pipeline\":" + json + "}\n\n";
							if (!sink.write(event.data(), event.size())) break;
						}
						return true;
					});
			});

			std::printf("LedPipelines preview serving on http://127.0.0.1:%d\n", port);
			std::fflush(stdout);

			// Blocks this thread for the life of the process; the server is intentionally never torn down.
			server->listen("127.0.0.1", port);
		}).detach();
	}
}
