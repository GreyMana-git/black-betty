import { StatusResponse, StatusResponseHistoryStat } from "./StatusResponse";

export type HistoryProperty = ("temperature" | "output" | "heater" | "health");

export interface HistoryGraphAxisInfo {
    minValue: number;
    maxValue: number;
    maxItems: number;
    gridStep: number;
    unit: string;
}

interface HistoryGridInfo {
    x: { "x": number, "label": string }[];
    xposy1: number;
    xposy2: number;
    y: { "y": number, "label": string }[];
    yposx1: number;
    yposx2: number;
}

export class HistoryGraph {
    private element : HTMLCanvasElement;
    private color: { "r": number, "g": number, "b": number };
    private property: HistoryProperty;
    private axisInfo: HistoryGraphAxisInfo;
    
    constructor(id: string, property: HistoryProperty, color: string, axisInfo: HistoryGraphAxisInfo) {
        this.element = <HTMLCanvasElement> window.document.getElementById(id);
        this.property = property;
        this.axisInfo = { ...axisInfo };
        color = color.trim().replace("#", "");
        this.color = { "r": parseInt(color.substr(0, 2), 16), "g": parseInt(color.substr(2, 2), 16), "b": parseInt(color.substr(4, 2), 16)};
    }

    public getColor(opacity: number = 1) {
        if (opacity === 1) {
            return "rgb(" + [this.color.r, this.color.g, this.color.b].join(",") + ")";
        }

        return "rgba(" + [this.color.r, this.color.g, this.color.b, opacity].join(",") + ")";
    }

    public resize(): void {
        const { width, height } = this.element.getBoundingClientRect();
        this.element.width = width;
        this.element.height = height;

        const checkBox = this.element.getBoundingClientRect();
        if (width != checkBox.width || height != checkBox.height) {
            this.element.width += (width - checkBox.width);
            this.element.height = (height - checkBox.height);
        }
    }

    private line(canvas: CanvasRenderingContext2D, x1 : number, y1 : number, x2 : number, y2 : number) {
        canvas.beginPath();
        canvas.moveTo(x1, y1);
        canvas.lineTo(x2, y2);
        canvas.stroke();
    }


    update(data: StatusResponse) {
        // Check for valid canvas
        const canvas = this.element?.getContext("2d");
        if (canvas == null) {
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
            "fullHeight": this.element.height,
            "minValue": this.axisInfo.minValue,
            "maxValue": this.axisInfo.maxValue,
            "count": this.axisInfo.maxItems
        }
        
        let items: StatusResponseHistoryStat[] = [];
        data.history.slice(0, bounding.count).forEach((item) => {
            const stat: StatusResponseHistoryStat = item[this.property];
            bounding.minValue = Math.min(bounding.minValue, stat.min);
            bounding.maxValue = Math.max(bounding.maxValue, stat.max);
            items.push(stat);
        });
        
        // Create locale helper functions for drawing into the canvas
        function px(x: number) { 
            const calculated = (bounding.count - x - 1) / (bounding.count - 1) * bounding.width;
            return bounding.left + Math.floor(Math.max(0, Math.min(bounding.width, calculated))) + 0.5;
        }
            
        function py(y: number) {
            const calculated = bounding.height - ((y - bounding.minValue) / (bounding.maxValue - bounding.minValue) * bounding.height);
            return Math.floor(Math.max(0, Math.min(bounding.height, calculated))) + 0.5;
        }
        
        // Calculate grid layout
        let gridlayout : HistoryGridInfo = { "x": [], xposy1: bounding.height, xposy2: 0, "y": [], yposx1: bounding.left, yposx2:bounding.fullWidth };
        for (let grid = 0; grid < bounding.count; grid++) {
            gridlayout.x.push({ "x": px(grid), "label": (-grid * data.window / 1000.0).toFixed(1) + "s" });
        }

        for (let grid = Math.floor(bounding.minValue / this.axisInfo.gridStep) * this.axisInfo.gridStep; grid < bounding.maxValue; grid += this.axisInfo.gridStep) {
            gridlayout.y.push({"y": py(grid), "label": grid + this.axisInfo.unit });
        }

        // Draw gridlines
        canvas.clearRect(0, 0, this.element.width, this.element.height);
        canvas.lineWidth = 1;
        canvas.strokeStyle = "rgba(32, 32, 32, 0.10)";
        gridlayout.x.forEach((grid) => this.line(canvas, grid.x, gridlayout.xposy1, grid.x, gridlayout.xposy2));
        gridlayout.y.forEach((grid) => this.line(canvas, gridlayout.yposx1, grid.y, gridlayout.yposx2, grid.y));

        // Draw axis
        canvas.font = "16px sans-serif";
        canvas.strokeStyle = canvas.fillStyle = "rgb(32, 32, 32)";
        this.line(canvas, px(0), py(0), px(bounding.count - 1), py(0));
        this.line(canvas, px(bounding.count - 1), py(bounding.minValue), px(bounding.count - 1), py(bounding.maxValue));
        canvas.textAlign = "center";
        gridlayout.x.forEach((grid) => canvas.fillText(grid.label, grid.x, gridlayout.xposy1 + 16));
        canvas.textAlign = "right";
        gridlayout.y.forEach((grid) => canvas.fillText(grid.label, gridlayout.yposx1 - 4, grid.y));

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
            let x = px(index);
            let y = py(item.current);
            canvas.beginPath();
            canvas.arc(x, y, 3, 0, Math.PI * 2);
            canvas.fill();
            canvas.fillText(item.current.toFixed(2) + this.axisInfo.unit, x, y - 10);
        });
    }
}
