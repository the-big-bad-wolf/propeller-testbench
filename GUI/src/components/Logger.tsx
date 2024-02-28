import { createSignal, onCleanup, onMount } from "solid-js";
import { Chart, Title, Tooltip, Legend, Colors } from "chart.js";
import { Line } from "solid-chartjs";

interface Measurement {
	time: number;
	force: number;
}

interface WebSocketData {
	force_measurements: Measurement[];
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
	const allData: WebSocketData[] = [];
	const socket = new WebSocket("ws://192.168.137.30:81");
	const [ref, setRef] = createSignal<HTMLCanvasElement | null>(null);
	const [sliderValue, setSliderValue] = createSignal(0);
	const [benchmarkDuration, setBenchmarkDuration] = createSignal(0);

	socket.onerror = event => {
		console.error("WebSocket error observed:", event);
		socket.close();
	};

	socket.onopen = () => {
		console.log("WebSocket connection opened");
	};

	socket.onmessage = event => {
		if (event.data === "Benchmark finished") {
			document
				.getElementById("start-button")!
				.removeChild(document.getElementById("start-button")!.childNodes[1]);
			document.getElementById("start-button")!.classList.remove("btn-disabled");
			downloadCSV(allData);
			return;
		}
		const data: WebSocketData = JSON.parse(event.data);
		allData.push(data);
		if (chartData.labels.length > 10) {
			chartData.labels.shift();
			chartData.datasets[0].data.shift();
		}
		chartData.labels.push(
			...data.force_measurements.map(measurement => measurement.time.toString())
		);
		chartData.datasets[0].data.push(
			...data.force_measurements.map(measurement => measurement.force)
		);
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
			<div class="w-1/3 mx-4">
				<h1 class="text-2xl ml-1 mb-2 text-center">Control Panel</h1>
				<div>
					<div class="flex justify-between">
						<h2 class="mb-2">Motor Speed</h2>
						<label>{sliderValue()}</label>
					</div>
					<input
						type="range"
						min={-127}
						max={127}
						value={sliderValue()}
						step={1}
						class="range range-primary w-full"
						onInput={event => setSliderValue(Number(event.currentTarget.value))}
					/>
				</div>
				<div class="mt-5 flex-col text-center">
					<label class="block mb-2">Benchmark duration (seconds)</label>
					<input
						type="number"
						value={benchmarkDuration()}
						step={1}
						class="input input-bordered w-1/2"
						onChange={event => {
							const inputValue = event.target.value;
							setBenchmarkDuration(Math.round(Number(inputValue)));
						}}
					/>
				</div>
				<div class="flex w-full justify-evenly mt-10">
					<button
						class="btn btn-error"
						onclick={event => {
							const DataToSend = { command: "stop" };
							socket.send(JSON.stringify(DataToSend));
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
							const dataToSend = {
								motor1_speed: sliderValue(),
								motor2_speed: sliderValue(),
								command: "start",
								benchmark_duration: benchmarkDuration(),
								timestamp: new Date().toISOString(),
							};
							socket.send(JSON.stringify(dataToSend));
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

const downloadCSV = (data: WebSocketData[]) => {
	const csvContent = "data:text/csv;charset=utf-8," + convertToCSV(data);
	const encodedUri = encodeURI(csvContent);
	const link = document.createElement("a");
	link.setAttribute("href", encodedUri);
	link.setAttribute("download", "data.csv");
	document.body.appendChild(link);
	link.click();
};

const convertToCSV = (data: WebSocketData[]) => {
	const headers = Object.keys(data[0]).join(",");
	const rows = data.map(obj => Object.values(obj).join(","));
	return [headers, ...rows].join("\n");
};
