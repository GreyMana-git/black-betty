import { getStatus, StatusResponse } from "./Status";
import { execute } from "./Command";
import { HistoryGraph } from "./HistoryGraph";


export class AppUI {
    private status: StatusResponse | null;
    private graphs: HistoryGraph[];
    private active: boolean;

    constructor() {
        this.status = null;
        this.graphs = [];
        this.active = true;
    }

    public init(): void {
        this.graphs.push(new HistoryGraph("temperature", "ff0000", { min: 30, max: 150.0, yStep: 10, unit: "\u2103" }));
        this.graphs.push(new HistoryGraph("output", "0000ff", { min: -5.0, max: 200.0, yStep: 50, unit: "" }));
        this.graphs.push(new HistoryGraph("heater", "00ff00", { min: 0.0, max: 1.0, yStep: 0.25, unit: "" }));
        this.graphs.push(new HistoryGraph("health", "ffa000", { min: 0, max: 50.0, yStep: 10, unit: "ms" }));

        // Toggle enable / disable
        this.on("overview-toggleEnable", "click", async () => {
            if (this.status != null) {
                await execute(this, "SET heater.enabled " + (this.status.heater.mode === "off" ? "true" : "false"));
            }
        });

        // Open setting pane
        this.on("setting-open", "click", async () => {
            const element = this.get("setting-panel");
            if (element != null && this.status != null) {
                // Update setting dialog
                const t = this.setText.bind(this);
                t("setting-temperature-low", this.status.temperature.low);
                t("setting-temperature-high", this.status.temperature.high);
                t("setting-pid-kp", this.status.pid.kp);
                t("setting-pid-ki", this.status.pid.ki);
                t("setting-pid-kd", this.status.pid.kd);
                t("setting-toggle-countdown", this.status.isCountdownMode ? "Disable Countdown" : "Enable Countdown");
                element.classList.remove("slide-out");
                element.classList.add("slide-in");
            }
        });

        // Close setting dialog
        this.on("setting-close", "click", () => {
            const element = this.get("setting-panel");
            if (element != null) {
                element.classList.remove("slide-in");
                element.classList.add("slide-out");
            }
        });

        // Setting dialog apply temperature
        this.on("setting-apply-temperature", "click", async () => {
            const low = parseFloat(this.getText("setting-temperature-low"));
            const high= parseFloat(this.getText("setting-temperature-high"));
            await execute(this, "SET heater " + low + " " + high);
        });

        // Setting dialog apply pid
        this.on("setting-apply-pid", "click", async () => {
            if (this.status != null) {
                const kp = parseFloat(this.getText("setting-pid-kp"));
                const ki = parseFloat(this.getText("setting-pid-ki"));
                const kd = parseFloat(this.getText("setting-pid-kd"));
                
                await execute(this, "SET pid " + kp + " " + ki + " " + kd);
            }
        });

        // Setting countdown mode
        this.on("setting-toggle-countdown", "click", async () => {
            if (this.status != null) {
                const mode = !this.status.isCountdownMode;
                await execute(this, "SET countdown_mode " + mode.toString());
                this.setText("setting-toggle-countdown", mode ? "Disable Countdown" : "Enable Countdown");
            }
        });

        // Save values to eeprom
        this.on("setting-save", "click", async () => {
            await execute(this, "SAVE");
        });

        // Restart
        this.on("setting-restart", "click", async () => {
            let result = await execute(this, "RESTART");
        });

        // Update active flag
        window.addEventListener("blur", () => { this.active = false; });
        window.addEventListener("focus", () => { this.active = true; });

        // Hide notification dialog
        const notify = this.get("notify-panel");
        if (notify != null) {
            notify.style.display = "none";
        }
    }

    public async update(): Promise<void> {
        // Do not update when inactive (blurred)
        if (!this.active) {
            return;
        }
        
        this.status = await getStatus(this.status);
        if (this.status == null) {
            return;
        }

        // Update all text controls
        const { temperature: temp, pid, heater } = this.status;
        const t = this.setText.bind(this);
        t("overview-temperature", temp.current.toFixed(3));
        t("overview-mode", heater.mode);
        t("overview-toggleEnable", heater.mode === "off" ? "Enable" : "Disable");
        t("temperature-current", temp.current);
        t("temperature-target", temp.target);
        t("temperature-low", temp.low.toFixed(1));
        t("temperature-high", temp.high.toFixed(1));
        t("pid-kp", pid.kp.toFixed(3));
        t("pid-ki", pid.ki.toFixed(3));
        t("pid-kd", pid.kd.toFixed(3));
        t("pid-input", pid.input.toFixed(3));
        t("pid-output", pid.output.toFixed(3));
        t("pid-setpoint", pid.setpoint.toFixed(3));
        t("heater-mode", heater.mode);
        t("heater-active", heater.active);

        this.graphs.forEach((graph) => graph.update(this.status, 15, 1));
    }

    public getToken(): number {
        return this.status?.token || 0;
    }

    public notify(header: string, message: string) {
        const element = this.get("notify-panel");
        if (element != null) {
            element.style.display = "none";
            element.classList.remove("fade-out");
            this.setText("notify-header", header);
            this.setText("notify-message", message);
            window.setTimeout(() => {
                element.classList.add("fade-out");
                element.style.display = "block";
            }, 5);
        }
    }

    private get(id: string): HTMLElement | null {
        return window.document.getElementById(id);
    }

    private setText(id: string, text: string | number | boolean): void {
        if (typeof text === "number") {
            text = text.toString();
        } else if (typeof text === "boolean") {
            text = text.toString();
        }
        
        const element = this.get(id);
        if (element instanceof HTMLInputElement) {
            element.value = text;
        } else if (element instanceof HTMLElement) {
            element.textContent = text;
        }
    }

    private getText(id: string): string {
        const element = this.get(id);
        if (element instanceof HTMLInputElement) {
            return element.value || "";
        } else if (element instanceof HTMLElement) {
            return element.textContent || "";
        }

        return "";
    }

    private on(id: string, event: string, callback: () => Promise<void> | void) {
        this.get(id)?.addEventListener(event, callback);
    }
}