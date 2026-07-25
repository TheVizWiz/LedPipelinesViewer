import { useEffect, useRef, useState } from 'react';
import { useSSE } from './useSSE.ts';
import type { FrameEvent } from './types.ts';

// The inset hairline that outlines every cell (visible even on unlit pixels so the grid is legible).
const GRID_OUTLINE = 'inset 0 0 0 1px rgba(255,255,255,0.04)';
const REFERENCE_PX_SIZE = 14;

// Build the box-shadow for one pixel: its own color glowing, with extent/opacity scaling by brightness x bloom, plus
// a size factor so the glow stays proportional as pixels resize. Ported from the original vanilla viewer.
function shadowFor(r: number, g: number, b: number, bloom: number, pxSize: number): string {
	const brightness = Math.max(r, g, b) / 255;
	const strength = brightness * bloom;
	if (strength <= 0.001) return GRID_OUTLINE;

	const sizeScale = pxSize / REFERENCE_PX_SIZE;
	const coreBlur = strength * 10 * sizeScale, coreSpread = strength * 3 * sizeScale;
	const haloBlur = strength * 28 * sizeScale, haloSpread = strength * 8 * sizeScale;
	const coreAlpha = Math.min(1, 0.35 + 0.65 * strength);
	const haloAlpha = Math.min(0.85, 0.55 * strength);

	return GRID_OUTLINE +
		`, 0 0 ${coreBlur}px ${coreSpread}px rgba(${r},${g},${b},${coreAlpha})` +
		`, 0 0 ${haloBlur}px ${haloSpread}px rgba(${r},${g},${b},${haloAlpha})`;
}

export function PixelView() {
	const { data, status } = useSSE<FrameEvent>('/stream');
	const [bloom, setBloom] = useState(1); // 0..2, from the slider (value/100)
	const [pxSize, setPxSize] = useState(14);
	const rootRef = useRef<HTMLDivElement>(null);

	const strips = data?.strips ?? [];

	// Restyle pixels imperatively each render: React owns the DOM structure (one .px per pixel), but per-frame we set
	// background + box-shadow directly rather than through props, matching the original's approach and avoiding a
	// style object per pixel per frame. Runs after paint whenever the frame or slider values change.
	useEffect(() => {
		const rootEl = rootRef.current;
		if (!rootEl) return;
		const stripEls = rootEl.querySelectorAll<HTMLDivElement>('.pixels');
		strips.forEach((strip, i) => {
			const pxEls = stripEls[i]?.children;
			if (!pxEls) return;
			for (let j = 0; j < strip.length; j++) {
				const el = pxEls[j] as HTMLDivElement | undefined;
				if (!el) continue;
				const hex = strip[j];
				el.style.background = hex;
				const r = parseInt(hex.slice(1, 3), 16);
				const g = parseInt(hex.slice(3, 5), 16);
				const b = parseInt(hex.slice(5, 7), 16);
				el.style.boxShadow = shadowFor(r, g, b, bloom, pxSize);
			}
		});
	}, [data, bloom, pxSize, strips]);

	return (
		<>
			<div className="bar">
				<div className={'status ' + status}>{status === 'live' ? 'live' : status + '…'}</div>
				<div className="control">
					<label htmlFor="bloom">bloom</label>
					<input id="bloom" type="range" min={0} max={200} value={bloom * 100}
						onChange={(e) => setBloom(+e.target.value / 100)} />
					<span className="value">{Math.round(bloom * 100)}%</span>
				</div>
				<div className="control">
					<label htmlFor="size">size</label>
					<input id="size" type="range" min={4} max={48} value={pxSize}
						onChange={(e) => setPxSize(+e.target.value)} />
					<span className="value">{pxSize}px</span>
				</div>
			</div>
			<div className="view-scroll">
				<div ref={rootRef} style={{ ['--px-size' as string]: pxSize + 'px' }}>
					{strips.map((strip, i) => (
						<div className="strip" key={i}>
							<div className="strip-label">strip {i} · {strip.length} px</div>
							<div className="pixels">
								{strip.map((_, j) => <div className="px" key={j} />)}
							</div>
						</div>
					))}
				</div>
			</div>
		</>
	);
}
