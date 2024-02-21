import { createSignal, onCleanup } from "solid-js";

const WebSocketComponent = () => {
	const [data, setData] = createSignal("");

	// Create a WebSocket connection
	const socket = new WebSocket("ws://192.168.137.157:81");

	socket.onerror = event => {
		console.error("WebSocket error observed:", event);
	};
	socket.onopen = () => {
		console.log("WebSocket connection opened");
	};
	// Handle incoming messages
	socket.onmessage = event => {
		const message = JSON.parse(event.data);
		// Do something with the parsed message
		console.log(message);
		setData(message);
	};

	// Clean up the WebSocket connection when the component is unmounted
	onCleanup(() => {
		socket.close();
	});

	return (
		<div>
			<h1>Live Data Stream</h1>
			<p>{data().i}</p>
		</div>
	);
};

export default WebSocketComponent;
