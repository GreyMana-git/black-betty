
interface CommonStatusResponse {
    id: string;
    token: number;
    isDebug: boolean;
    isCountdownMode: boolean;
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

function explode(index: number, array: number[]): StatusResponseHistoryStat {
    const offset = index * 4;
    return { "current": array[offset], "min": array[offset + 1], "max": array[offset + 2], "average": array[offset + 3] };
}

export async function getStatus(oldStatus: StatusResponse | null): Promise<StatusResponse> {
    const response = await window.fetch(getApiUri("/status"), { "method": "GET" });
    const source = <RawStatusResponse>await response.json();

    const status: StatusResponse = {
        "id": source.id,
        "token": source.token,
        "isDebug": source.isDebug,
        "isCountdownMode": source.isCountdownMode,
        "temperature": source.temperature,
        "pid": source.pid,
        "heater": source.heater,
        "window": source.window || 1000,
        "history": []
    };

    // Convert source to expanded object
    const count = source.history.samples.length;
    for (let index = 0; index < count; index++) {
        status.history.push({
            "sequence": source.history.sequence[index],
            "samples": source.history.samples[index],
            "temperature": explode(index, source.history.temperature),
            "output": explode(index, source.history.output),
            "heater": explode(index, source.history.heater),
            "health": explode(index, source.history.health)
        });
    }

    // Merge with old history
    if (oldStatus != null) {
        const sequences = status.history.map((item) => item.sequence);
        status.history.push(...oldStatus.history.filter((item) => sequences.indexOf(item.sequence) === -1));
    }

    // Limit history to 120 entries
    status.history = status.history.slice(0, 120);
    return status
}
