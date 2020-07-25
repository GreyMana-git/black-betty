
interface CommonStatusResponse {
    id: string;
    token: number;
    temperature: {
        current: number;
        target: number;
        low: number;
        high: number;
    }
    pid: {
        kp: number;
        ki: number;
        kd: number;
        input: number;
        output: number;
        setpoint: number;
    },
    heater: {
        mode: ("off" | "low" | "high");
        active: boolean;
    }
    window: number;
}

interface RawStatusResponse extends CommonStatusResponse {
    history: {
        // 4-pair value of current, min, max, avg
        temperature: number[];
        // 4-pair value of current, min, max, avg
        output: number[];
        // 4-pair value of current, min, max, avg
        heater: number[];
        // 4-pair value of current, min, max, avg
        health: number[];
        // 1-pair value
        samples: number[];
        // 1-pair value
        sequence: number[];
    }
}

export interface StatusResponseHistoryStat {
    current: number
    min: number
    max: number
    average: number
}

export interface StatusResponseHistoryItem {
    sequence: number
    samples: number
    temperature: StatusResponseHistoryStat
    output: StatusResponseHistoryStat
    heater: StatusResponseHistoryStat
    health: StatusResponseHistoryStat
}

export interface StatusResponse extends CommonStatusResponse {
    history: StatusResponseHistoryItem[]
}

export function getApiUri(path: string): string {
    if (window.origin === "http://localhost:8080") {
        return "http://esp-grey" + path;
    }

    return window.origin + path;
}

function getStat(index: number, array: number[]): StatusResponseHistoryStat {
    const offset = index * 4;
    return { "current": array[offset], "min": array[offset + 1], "max": array[offset + 2], "average": array[offset + 3] };
}

export async function getStatusData(merge: StatusResponse | null): Promise<StatusResponse> {
    const response = await window.fetch(getApiUri("/status"), { "method": "GET" });
    const source = <RawStatusResponse>await response.json();

    const data: StatusResponse = {
        "id": source.id,
        "token": source.token,
        "temperature": source.temperature,
        "pid": source.pid,
        "heater": source.heater,
        "window": source.window || 1000,
        "history": []
    };

    // Convert source data to expanded object
    const count = source.history.samples.length;
    for (let index = 0; index < count; index++) {
        data.history.push({
            "sequence": source.history.sequence[index],
            "samples": source.history.samples[index],
            "temperature": getStat(index, source.history.temperature),
            "output": getStat(index, source.history.output),
            "heater": getStat(index, source.history.heater),
            "health": getStat(index, source.history.health)
        });
    }

    if (merge != null) {
        const sequences = data.history.map((item) => item.sequence);
        data.history.push(...merge.history.filter((item) => sequences.indexOf(item.sequence) === -1));
    }

    return data;
}

interface CommandResult {
    success: boolean;
    message: string;
}

export async function executeCommand(data: StatusResponse, command: string): Promise<CommandResult> {
    const content = "TOKEN " + data.token + " " + command;

    const response = await window.fetch(getApiUri("/command"), { "method": "POST", body: content });
    return <CommandResult>await response.json();
}
