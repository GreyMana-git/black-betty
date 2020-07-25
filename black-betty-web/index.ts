import { AppUI } from "./src/AppUI";

/** Execute the main function on DOM loaded */
window.document.addEventListener("DOMContentLoaded", () => {
    const app = new AppUI();
    app.init();

    let nextTimeout = Date.now();
    async function update(): Promise<void> {
        try {
            await app.update();
        } finally {
            const now = Date.now();
            const delay = nextTimeout - now;
            while (nextTimeout < now) {
                nextTimeout += 1000;
            }

            console.log("Delaying " + delay);
            window.setTimeout(() => update(), Math.max(500, delay));
        }
    }

    window.setInterval(() => app.update(), 1000);
});
