import { useState } from 'react';
import { useSSE } from './useSSE.ts';
import type { PipelineEvent, StageNode, StageState } from './types.ts';

// Keys on a node that are structural, not config - handled specially, never shown as plain fields.
const STRUCTURAL_KEYS = new Set(['type', 'id', 'state', 'stage', 'children']);

// Format a serialized value for display. Colors ({r,g,b,a}) and HSVA ({h,s,v,a}) render as compact swatches/tuples;
// everything else falls back to a JSON-ish string.
function formatValue(value: unknown): string {
	if (value === null || value === undefined) return '—';
	if (typeof value === 'object') {
		const o = value as Record<string, unknown>;
		if ('r' in o && 'g' in o && 'b' in o) return `rgba(${o.r}, ${o.g}, ${o.b}, ${o.a})`;
		if ('h' in o && 's' in o && 'v' in o) return `hsva(${o.h}, ${o.s}, ${o.v}, ${o.a})`;
		return JSON.stringify(value);
	}
	return String(value);
}

// A colored swatch preview for RGBA/HSVA config values, so colors read at a glance.
function swatch(value: unknown): string | null {
	if (value && typeof value === 'object') {
		const o = value as Record<string, number>;
		if ('r' in o && 'g' in o && 'b' in o) return `rgb(${o.r}, ${o.g}, ${o.b})`;
	}
	return null;
}

function ConfigFields({ node }: { node: StageNode }) {
	const entries = Object.entries(node).filter(([k]) => !STRUCTURAL_KEYS.has(k));
	if (entries.length === 0) return null;
	return (
		<div className="node-fields">
			{entries.map(([k, v]) => {
				const sw = swatch(v);
				return (
					<>
						<span className="k" key={k + ':k'}>{k}</span>
						<span className="v" key={k + ':v'}>
							{sw && (
								<span style={{
									display: 'inline-block', width: 10, height: 10, borderRadius: 2,
									background: sw, marginRight: 6, verticalAlign: 'middle',
									boxShadow: 'inset 0 0 0 1px rgba(255,255,255,0.2)',
								}} />
							)}
							{formatValue(v)}
						</span>
					</>
				);
			})}
		</div>
	);
}

// State fields (Step 4): everything in the state object EXCEPT runningState (which drives the box color + badge and is
// shown there). activeChildren is a nested structural array, handled by the recursion below, so it's skipped here.
function StateFields({ state }: { state: StageState }) {
	const entries = Object.entries(state).filter(([k]) => k !== 'runningState' && k !== 'activeChildren');
	if (entries.length === 0) return null;
	return (
		<div className="node-fields">
			{entries.map(([k, v]) => (
				<>
					<span className="k state" key={k + ':k'}>{k}</span>
					<span className="v" key={k + ':v'}>{formatValue(v)}</span>
				</>
			))}
		</div>
	);
}

// State keys shown in the COMPACT view (details off). A small allowlist rather than the full state dump: the always-on
// fields plus per-effect-type extras. Grows as we decide more fields are worth seeing at a glance.
const COMPACT_STATE_KEYS = ['elapsedPercentage'];
const COMPACT_STATE_KEYS_BY_TYPE: Record<string, string[]> = {
	Moving: ['currentPosition'], // Moving's current offset along the strip.
};

// Render only the allowlisted state fields for a node, used when details are off. Skips anything not present so a stage
// missing a whitelisted field (e.g. a non-timed effect has no elapsedPercentage) simply shows nothing extra.
function CompactStateFields({ node }: { node: StageNode }) {
	const state = node.state;
	if (!state) return null;
	const keys = [...COMPACT_STATE_KEYS, ...(COMPACT_STATE_KEYS_BY_TYPE[node.type] ?? [])];
	const entries = keys.filter((k) => k in state).map((k) => [k, state[k]] as const);
	if (entries.length === 0) return null;
	return (
		<div className="node-fields">
			{entries.map(([k, v]) => (
				<>
					<span className="k state" key={k + ':k'}>{k}</span>
					<span className="v" key={k + ':v'}>{formatValue(v)}</span>
				</>
			))}
		</div>
	);
}

function Node({ node, details }: { node: StageNode; details: boolean }) {
	const running = node.state?.runningState ?? 'UNSPECIFIED';

	// activeChildren (Spawner) are live children carried inside state, not the static children[] of a pipeline. Render
	// them as nested boxes too, so a Spawner shows its live spawned effects.
	const activeChildren = (node.state?.activeChildren as StageNode[] | undefined) ?? [];

	return (
		<div className={`node run-${running}`}>
			<div className="node-head">
				<span className="node-type">{node.type}</span>
				<span className="node-id">{node.id}</span>
				<span className={`node-running run-${running}`}>{running}</span>
			</div>

			{/* In "details" mode: full config + all state fields. With details off: the head (type + id + running
			    badge) plus a small allowlist of state fields (elapsedPercentage, and Moving's currentPosition) - the
			    bare minimum plus a few whitelisted values worth seeing at a glance. Structure (inner/children) always
			    renders, in both modes, since it defines the tree rather than being a detail. */}
			{details ? (
				<>
					<ConfigFields node={node} />
					{node.state && <StateFields state={node.state} />}
				</>
			) : (
				<CompactStateFields node={node} />
			)}

			{/* Wrapper: a single inner stage, nested below this box's own fields. */}
			{node.stage && (
				<div className="node-inner">
					<Node node={node.stage} details={details} />
				</div>
			)}

			{/* Pipeline: ordered child stages, side by side. */}
			{node.children && node.children.length > 0 && (
				<div className="node-children">
					{node.children.map((child) => <Node node={child} key={child.id} details={details} />)}
				</div>
			)}

			{/* Spawner: live spawned children from state. */}
			{activeChildren.length > 0 && (
				<div className="node-children">
					{activeChildren.map((child) => <Node node={child} key={child.id} details={details} />)}
				</div>
			)}
		</div>
	);
}

export function TreeView() {
	const { data, status } = useSSE<PipelineEvent>('/pipeline');
	// Details on = full config + state per box (the default). Off = just type/id/running per box - a compact view for
	// scanning structure and what's running. More fields may be whitelisted into the compact view later.
	const [details, setDetails] = useState(true);

	return (
		<>
			<div className="bar">
				<div className={'status ' + status}>{status === 'live' ? 'live' : status + '…'}</div>
				<label className="control">
					<input type="checkbox" checked={details} onChange={(e) => setDetails(e.target.checked)} />
					details
				</label>
			</div>
			<div className="view-scroll">
				<div className="tree">
					{data?.pipeline
						? <Node node={data.pipeline} details={details} />
						: <div style={{ opacity: 0.5 }}>waiting for pipeline…</div>}
				</div>
			</div>
		</>
	);
}
