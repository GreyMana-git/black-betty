import "./src/StatusResponse";
import { getStatusData, StatusResponse, executeCommand } from "./src/StatusResponse";
import { HistoryGraph } from "./src/HistoryGraph";

interface AppState {
    data: StatusResponse | null
    graph: {
        temperature: HistoryGraph;
        output: HistoryGraph;
        heater: HistoryGraph;
        health: HistoryGraph;
    }
}

var state: AppState | null = null;

function text(id: string, text: string | number | boolean) {
    if (typeof text === "number") {
        text = text.toString();
    } else if (typeof text === "boolean") {
        text = text.toString();
    }
    
    let element = window.document.getElementById(id);
    if (element instanceof HTMLInputElement) {
        element.value = text;
    } else if (element instanceof HTMLElement) {
        element.textContent = text;
    }
}

async function update(): Promise<void> {
    if (state == null) {
        return;
    }

    state.data = await getStatusData(state.data);
    const data = state.data;

    // Global text updates
    text("overview-temperature", data.temperature.current.toFixed(3));
    text("overview-mode", data.heater.mode);
    text("overview-toggleEnable", data.heater.mode === "off" ? "Enable" : "Disable");
    text("temperature-current", data.temperature.current);
    text("temperature-target", data.temperature.target);
    text("temperature-low", data.temperature.low.toFixed(1));
    text("temperature-high", data.temperature.high.toFixed(1));
    text("pid-kp", data.pid.kp.toFixed(3));
    text("pid-ki", data.pid.ki.toFixed(3));
    text("pid-kd", data.pid.kd.toFixed(3));
    text("pid-input", data.pid.input.toFixed(3));
    text("pid-output", data.pid.output.toFixed(3));
    text("pid-setpoint", data.pid.setpoint.toFixed(3));
    text("heater-mode", data.heater.mode);
    text("heater-active", data.heater.active);
    
    state.data = await getStatusData(data);
    state.graph.temperature.update(data);
    state.graph.output.update(data);
    state.graph.heater.update(data);
    state.graph.health.update(data);

    window.setTimeout(() => { update(); }, 1000);
}

/** Execute the main function on DOM loaded */
window.document.addEventListener("DOMContentLoaded", () => {
    // Initialize state object
    const temperature = new HistoryGraph("graph-temperature", "temperature", "#ff0000", { minValue: 30, "maxValue": 150.0, maxItems: 15, gridStep: 10, unit: "\u2103" });
    const output = new HistoryGraph("graph-output", "output", "#0000ff", { minValue: -260.0, "maxValue": 260.0, maxItems: 15, gridStep: 50, unit: "" });
    const heater = new HistoryGraph("graph-heater", "heater", "#00ff00", { minValue: 0.0, "maxValue": 1.0, maxItems: 15, gridStep: 0.25, unit: "" });
    const health = new HistoryGraph("graph-health", "health", "#ffa000", { minValue: 0, "maxValue": 50.0, maxItems: 15, gridStep: 10, unit: "ms" });
    state = {
        "data": null,
        "graph": { "temperature": temperature, "output": output, "heater": heater, "health": health }
    };

    window.document.getElementById("setting-open")?.addEventListener("click", () => {
        const element  : HTMLElement | null = window.document.getElementById("setting-dialog");
        if (element == null || state == null || state.data == null) {
            return;
        }

        // Update setting dialog
        const data = state?.data;
        text("setting-temperature-low", data.temperature.low);
        text("setting-temperature-high", data.temperature.high);
        text("setting-pid-kp", data.pid.kp);
        text("setting-pid-ki", data.pid.ki);
        text("setting-pid-kd", data.pid.kd);
        
        element.style.display = "block";
    });
    
    window.document.getElementById("setting-close")?.addEventListener("click", () => {
        const element  : HTMLElement | null = window.document.getElementById("setting-dialog");
        if (element != null) {
            element.style.display = "none";
        }
    });

    window.document.getElementById("setting-apply-temperature")?.addEventListener("click", async () => {
        let data = state?.data;
        if (data == null) {
            return;
        }
        
        let low = parseFloat((<HTMLInputElement>window.document.getElementById("setting-temperature-low"))?.value || "");
        let high= parseFloat((<HTMLInputElement>window.document.getElementById("setting-temperature-high"))?.value || "");
        
        let result = await executeCommand(data, "SET heater.low " + low);
        result = await executeCommand(data, "SET heater.high " + high);

        return;
    });

    window.document.getElementById("setting-apply-pid")?.addEventListener("click", async () => {
        let data = state?.data;
        if (data == null) {
            return;
        }
        
        let kp = parseFloat((<HTMLInputElement>window.document.getElementById("setting-pid-kp"))?.value || "");
        let ki = parseFloat((<HTMLInputElement>window.document.getElementById("setting-pid-ki"))?.value || "");
        let kd = parseFloat((<HTMLInputElement>window.document.getElementById("setting-pid-kd"))?.value || "");
        
        let result = await executeCommand(data, "SET pid " + kp + " " + ki + " " + kd);
        return;
    });

    
    window.document.getElementById("setting-save")?.addEventListener("click", async () => {
        let data = state?.data;
        if (data == null) {
            return;
        }
        let result = await executeCommand(data, "SAVE");
        return;
    });

    window.document.getElementById("setting-restart")?.addEventListener("click", async () => {
        let data = state?.data;
        if (data == null) {
            return;
        }
        let result = await executeCommand(data, "RESTART");
        return;
    });

    window.document.getElementById("overview-toggleEnable")?.addEventListener("click", async () => {
        let data = state?.data;
        if (data == null) {
            return;
        }

        if (data.heater.mode === "off") {
            await executeCommand(data, "SET heater.enabled true");
        } else {
            await executeCommand(data, "SET heater.enabled false");
        }
    });

    update();    
});
