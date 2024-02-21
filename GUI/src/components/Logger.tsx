import { createSignal, onCleanup, onMount } from "solid-js";
import { Chart, Title, Tooltip, Legend, Colors } from "chart.js";
import { Line } from "solid-chartjs";

interface WebSocketData {
	i: number;
	force_measurements: number[];
	// Add more properties as needed
}

interface chartData {
	labels: string[];
	datasets: {
		label: string;
		data: number[];
	}[];
}

const WebSocketComponent = () => {
	const [force_measurements, setData] = createSignal<number[] | null>(null);
	const [chartData, setChartData] = createSignal<chartData>({
		labels: ["January", "February", "March", "April", "May"],
		datasets: [
			{
				label: "Force",
				data: [50, 60, 70, 80, 90],
			},
		],
	});

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

		const newChartData = {
			labels: [...chartData().labels.slice(-10), data.i.toString()],
			datasets: [{ label: "Force", data: [...chartData().datasets[0].data.slice(-10), data.i] }],
		};
		setChartData(newChartData);
	};

	onMount(() => {
		Chart.register(Title, Tooltip, Legend, Colors);
	});
	onCleanup(() => {
		socket.close();
	});

	return (
		<div>
			<h1>Live Data Stream</h1>
			<p>{force_measurements()}</p>
			<Line
				type="line"
				data={chartData()}
				options={{
					responsive: false,
					maintainAspectRatio: false,
				}}
				width={720}
				height={360}
			/>
		</div>
	);
};

export default WebSocketComponent;
