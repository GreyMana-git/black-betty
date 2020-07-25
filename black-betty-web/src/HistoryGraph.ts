import { StatusResponse, StatusResponseHistoryStat } from "./Status";

export type HistoryProperty = ("temperature" | "output" | "heater" | "health");

export interface HistoryGraphAxisInfo {
    min: number;
    max: number;
    yStep: number;
    unit: string;
}

interface HistoryGridInfo {
    x: { "x": number, "label": string }[];
    xy1: number;
    xy2: number;
    y: { "y": number, "label": string }[];
    yx1: number;
    yx2: number;
}

export class HistoryGraph {
    private element : HTMLCanvasElement;
    private color: { "r": number, "g": number, "b": number };
    private property: HistoryProperty;
    private axisInfo: HistoryGraphAxisInfo;
    
    constructor(property: HistoryProperty, color: string, axisInfo: HistoryGraphAxisInfo) {
        this.element = <HTMLCanvasElement> window.document.getElementById("graph-" + property);
        this.property = property;
        this.axisInfo = { ...axisInfo };
        const colorvalue = parseInt(color, 16);
        this.color = { "r": (colorvalue >> 16) & 0xff, "g": (colorvalue >> 8) & 0xff, "b": colorvalue & 0xff };
    }

    public getColor(opacity: number = 1) {
        return "rgba(" + [this.color.r, this.color.g, this.color.b, opacity].join(",") + ")";
    }

    public resize(): void {
        const { width, height } = this.element.getBoundingClientRect();
        this.element.width = width;
        this.element.height = height;

        const box = this.element.getBoundingClientRect();
        if (width != box.width || height != box.height) {
            this.element.width += (width - box.width);
            this.element.height = (height - box.height);
        }
    }

    private line(canvas: CanvasRenderingContext2D, x1 : number, y1 : number, x2 : number, y2 : number) {
        canvas.beginPath();
        canvas.moveTo(x1, y1);
        canvas.lineTo(x2, y2);
        canvas.stroke();
    }

    public update(status: StatusResponse | null, maxItems: number, xStep: number) {
        // Check for valid canvas
        const canvas = this.element?.getContext("2d");
        if (canvas == null || status == null) {
            return;
        }

        // Resize canvas
        this.resize();
        
        // Calculate new limits
        const bounding = {
            "left": 48,
            "bottom": 20,
            "width": this.element.width - 48,
            "fullWidth": this.element.width,
            "height": this.element.height - 20,
            "min": this.axisInfo.min,
            "max": this.axisInfo.max,
        }
        
        let items: StatusResponseHistoryStat[] = [];
        status.history.slice(0, maxItems).forEach((item) => {
            const stat: StatusResponseHistoryStat = item[this.property];
            bounding.min = Math.min(bounding.min, stat.min);
            bounding.max = Math.max(bounding.max, stat.max);
            items.push(stat);
        });
        
        // Create locale helper functions for drawing into the canvas
        function px(x: number) { 
            const calculated = (maxItems - x - 1) / (maxItems - 1) * bounding.width;
            return bounding.left + Math.floor(Math.max(0, Math.min(bounding.width, calculated))) + 0.5;
        }
            
        function py(y: number) {
            const calculated = bounding.height - ((y - bounding.min) / (bounding.max - bounding.min) * bounding.height);
            return Math.floor(Math.max(0, Math.min(bounding.height, calculated))) + 0.5;
        }
        
        // Calculate grid layout
        let layout : HistoryGridInfo = { "x": [], xy1: bounding.height, xy2: 0, "y": [], yx1: bounding.left, yx2:bounding.fullWidth };
        for (let grid = 0; grid < maxItems; grid++) {
            layout.x.push({ "x": px(grid), "label": (-grid * status.window / 1000.0).toFixed(1) + "s" });
        }

        for (let grid = Math.floor(bounding.min / this.axisInfo.yStep) * this.axisInfo.yStep; grid < bounding.max; grid += this.axisInfo.yStep) {
            layout.y.push({"y": py(grid), "label": grid + this.axisInfo.unit });
        }

        // Draw gridlines
        canvas.clearRect(0, 0, this.element.width, this.element.height);
        canvas.lineWidth = 1;
        canvas.strokeStyle = "rgba(32, 32, 32, 0.10)";
        layout.x.forEach((info, index) => { if (index % xStep === 0) { this.line(canvas, info.x, layout.xy1, info.x, layout.xy2); }});
        layout.y.forEach((info) => this.line(canvas, layout.yx1, info.y, layout.yx2, info.y));

        // Draw axis
        canvas.font = "16px sans-serif";
        canvas.strokeStyle = canvas.fillStyle = "rgb(32, 32, 32)";
        this.line(canvas, px(0), py(0), px(maxItems - 1), py(0));
        this.line(canvas, px(maxItems - 1), py(bounding.min), px(maxItems - 1), py(bounding.max));
        canvas.textAlign = "center";
        layout.x.forEach((info, index) => { if (index % xStep === 0) { canvas.fillText(info.label, info.x, layout.xy1 + 16); }});
        canvas.textAlign = "right";
        layout.y.forEach((info) => canvas.fillText(info.label, layout.yx1 - 4, info.y));

        // Fill the min/max area
        const first = items[0];
        canvas.fillStyle = this.getColor(0.15);
        canvas.beginPath();
        canvas.moveTo(px(0), py(first.max));
        items.forEach((item, index) => { canvas.lineTo(px(index), py(item.max)); });
        items.slice().reverse().forEach((item, index) => { canvas.lineTo(px(items.length - index - 1), py(item.min)); });
        canvas.fill();

        // Draw average line
        canvas.strokeStyle = this.getColor(0.5);
        canvas.beginPath();
        canvas.moveTo(px(0), py(first.average));
        items.forEach((item, index) => { canvas.lineTo(px(index), py(item.average)); });
        canvas.stroke();
        
        // Draw current line
        canvas.strokeStyle = this.getColor();
        canvas.lineWidth = 2;
        canvas.beginPath();
        canvas.moveTo(px(0), py(first.current));
        items.forEach((item, index) => { canvas.lineTo(px(index), py(item.current)); });
        canvas.stroke();

        // Draw labels
        canvas.font = "12px sans-serif";
        canvas.textAlign = "center";
        canvas.strokeStyle = canvas.fillStyle = "rgb(32, 32, 32)";
        items.forEach((item, index) => {
            if (index % xStep === 0) {
                const x = px(index);
                const y = py(item.current);
                canvas.beginPath();
                canvas.arc(x, y, 3, 0, Math.PI * 2);
                canvas.fill();
                canvas.fillText(item.current.toFixed(2) + this.axisInfo.unit, x, y - 10);
            }
        });
    }
}
