// The shapes emitted by the C++ backend's two SSE streams. These mirror the JSON produced by the LedPipelines
// serializer (LedPipelineStage::toJson) and the viewer's frame serializer.

// --- /stream : rendered pixels ---
export interface FrameEvent {
	seq: number;
	// One array of "#rrggbb" strings per strip.
	strips: string[][];
}

// --- /pipeline : pipeline structure + live state ---

export type RunningState = 'NOT_STARTED' | 'RUNNING' | 'DONE' | 'UNSPECIFIED';

// The per-stage "state" sub-object (present only because the viewer calls toJson(true)). runningState is always there;
// the rest are optional and vary by effect (timeline fields, Moving's currentPosition, Loop's currentNumLoops, etc.).
export interface StageState {
	runningState: RunningState;
	// Index signature: state fields differ per effect type and we display whatever is present without hardcoding each.
	[key: string]: string | number | boolean | unknown;
}

// A single serialized stage. `type` and `id` are always present. `state` is present under toJson(true). A wrapper
// carries its single inner as `stage`; a pipeline carries `children`; a source carries neither. Every other key is a
// config field (color, positions, runtimeMs, ...) whose set depends on the effect type - kept in an index signature so
// the tree can display them generically.
export interface StageNode {
	type: string;
	id: string;
	state?: StageState;
	stage?: StageNode; // wrappers (and Shared) - the single wrapped inner
	children?: StageNode[]; // pipelines - ordered child stages
	[key: string]: unknown; // config fields
}

export interface PipelineEvent {
	seq: number;
	pipeline: StageNode;
}
