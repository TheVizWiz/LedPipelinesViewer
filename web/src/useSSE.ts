import { useEffect, useRef, useState } from 'react';

export type ConnectionStatus = 'connecting' | 'live' | 'disconnected';

// Subscribe to a Server-Sent Events endpoint that emits one JSON object per `message`. Returns the latest parsed
// payload (or null before the first event) and the connection status. EventSource auto-reconnects, so a dropped
// backend recovers on its own once it is back. Malformed events are ignored rather than throwing.
export function useSSE<T>(path: string): { data: T | null; status: ConnectionStatus } {
	const [data, setData] = useState<T | null>(null);
	const [status, setStatus] = useState<ConnectionStatus>('connecting');
	// Keep the latest payload in a ref too, so high-frequency streams (60fps pixels) don't force a re-render storm on
	// consumers that only read `data` - though here we do setState per event; see PixelView for the throttling note.
	const latest = useRef<T | null>(null);

	useEffect(() => {
		const es = new EventSource(path);
		es.onopen = () => setStatus('live');
		es.onmessage = (e) => {
			try {
				const parsed = JSON.parse(e.data) as T;
				latest.current = parsed;
				setData(parsed);
			} catch {
				/* ignore malformed event */
			}
		};
		es.onerror = () => setStatus('disconnected');
		return () => es.close();
	}, [path]);

	return { data, status };
}
