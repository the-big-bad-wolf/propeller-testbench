import { createSignal, onCleanup, onMount } from "solid-js";
import { Chart, Title, Tooltip, Legend, Colors } from "chart.js";
import { Line } from "solid-chartjs";

interface WebSocketData {
	i: number;
	force_measurements: number[];
}

interface chartData {
	labels: string[];
	datasets: {
		label: string;
		data: number[];
	}[];
}

const WebSocketComponent = () => {
	const chartData: chartData = {
		labels: [],
		datasets: [
			{
				label: "Force",
				data: [],
			},
		],
	};
	const socket = new WebSocket("ws://192.168.137.150:81");
	const [ref, setRef] = createSignal<HTMLCanvasElement | null>(null);
	const [sliderValue, setSliderValue] = createSignal(0);

	socket.onerror = event => {
		console.error("WebSocket error observed:", event);
		socket.close();
	};

	socket.onopen = () => {
		console.log("WebSocket connection opened");
	};

	socket.onmessage = event => {
		const data: WebSocketData = JSON.parse(event.data);

		if (chartData.labels.length > 10) {
			chartData.labels.shift();
			chartData.datasets[0].data.shift();
		}
		chartData.labels.push(data.force_measurements.length.toString());
		chartData.datasets[0].data.push(...data.force_measurements);
		Chart.getChart(ref()!)?.update();
	};

	onMount(() => {
		Chart.register(Title, Tooltip, Legend, Colors);
	});
	onCleanup(() => {
		socket.close();
	});

	return (
		<div class="flex">
			<div class="h-[90vh] w-2/3">
				<h1 class="text-2xl ml-1">Live Data Stream</h1>
				<Line
					ref={setRef}
					type="line"
					data={chartData}
					options={{
						scales: {
							y: {
								ticks: {
									callback: function (value: number, index: number, ticks: number[]) {
										return value + "N";
									},
								},
								min: 0,
								max: 100,
							},
							x: {
								min: 0,
								max: 10,
							},
						},
						responsive: true,
						maintainAspectRatio: false,
					}}
					width={400}
					height={100}
				/>
			</div>
			<div class="w-1/3 mx-4 flex-col">
				<h1 class="text-2xl ml-1 mb-2 text-center">Control Panel</h1>
				<div>
					<h2 class="mb-2">Motor Speed</h2>
					<input
						type="range"
						min={-127}
						max={127}
						value={sliderValue()}
						step={1}
						class="range range-primary w-full"
						onInput={event => setSliderValue(Number(event.currentTarget.value))}
					/>
					<label>{sliderValue()}</label>
				</div>
				<div></div>
				<div class="flex w-full justify-evenly my-6">
					<button
						class="btn btn-error"
						onclick={event => {
							document
								.getElementById("start-button")!
								.removeChild(document.getElementById("start-button")!.childNodes[1]);
							document.getElementById("start-button")!.classList.remove("btn-disabled");
						}}
					>
						Stop
					</button>
					<button
						class="btn"
						id="start-button"
						onclick={event => {
							event.currentTarget.classList.add("btn-disabled");
							const childElement = document.createElement("span");
							childElement.classList.add("loading", "loading-spinner");
							event.currentTarget.appendChild(childElement);
							socket.send("Hello from frontend!");
						}}
					>
						Start
					</button>
				</div>
			</div>
		</div>
	);
};

export default WebSocketComponent;
