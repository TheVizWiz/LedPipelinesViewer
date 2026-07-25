import { useState } from 'react';
import { PixelView } from './PixelView.tsx';
import { TreeView } from './TreeView.tsx';

type Tab = 'pixels' | 'tree';

// Tabs are driven by location.pathname so /tree deep-links to the tree view (and / to pixels), while switching tabs is
// a client-side pushState with no reload. Both views mount their own SSE subscription; the inactive one is unmounted
// so it isn't consuming a stream in the background.
function tabForPath(path: string): Tab {
	return path.startsWith('/tree') ? 'tree' : 'pixels';
}

export function App() {
	const [tab, setTab] = useState<Tab>(tabForPath(window.location.pathname));

	const go = (next: Tab) => {
		setTab(next);
		window.history.pushState(null, '', next === 'tree' ? '/tree' : '/');
	};

	return (
		<>
			<div className="bar">
				<div className="tabs">
					<button className={'tab' + (tab === 'pixels' ? ' active' : '')} onClick={() => go('pixels')}>
						pixels
					</button>
					<button className={'tab' + (tab === 'tree' ? ' active' : '')} onClick={() => go('tree')}>
						tree
					</button>
				</div>
			</div>
			{tab === 'pixels' ? <PixelView /> : <TreeView />}
		</>
	);
}
