import { getApiUri } from "./Status";
import { AppUI } from "./AppUI";

interface CommandResult {
    success: boolean;
    message: string;
}

export async function execute(app: AppUI, command: string): Promise<void> {
    const content = "TOKEN " + app.getToken() + " " + command;

    try {
        const response = await window.fetch(getApiUri("/command"), { "method": "POST", body: content });
        const {success, message } = <CommandResult>await response.json();
        app.notify(success ? "Successfully executed command" : "Error executing command", message);

    } catch (ex) {

    }
}
