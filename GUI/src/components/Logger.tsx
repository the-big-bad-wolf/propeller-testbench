import { createSignal, onCleanup } from "solid-js";

interface WebSocketData {
	i: number;
	force_measurements: number[];
	// Add more properties as needed
}

const WebSocketComponent = () => {
	const [force_measurements, setData] = createSignal<number[] | null>(null);
	const socket = new WebSocket("ws://192.168.137.157:81");

	socket.onerror = event => {
		console.error("WebSocket error observed:", event);
		socket.close();
	};
	socket.onopen = () => {
		console.log("WebSocket connection opened");
	};
	// Handle incoming messages
	socket.onmessage = event => {
		const data: WebSocketData = JSON.parse(event.data);
		// Do something with the parsed message
		console.log(data);
		setData(data.force_measurements);
	};

	// Clean up the WebSocket connection when the component is unmounted
	onCleanup(() => {
		socket.close();
	});

	return (
		<div>
			<h1>Live Data Stream</h1>
			<p>{force_measurements()}</p>
		</div>
	);
};

export default WebSocketComponent;
