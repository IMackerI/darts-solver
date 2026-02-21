/**
 * Dartboard renderer — draws bed polygons, shots, aim markers, and heatmaps
 * on an HTML Canvas element.
 *
 * Coordinate system:
 *   Dartboard coords are in mm with (0,0) at center.
 *   +x → right, +y → up  (standard math).
 *   Canvas coords: +x → right, +y → down.
 *   We flip y when mapping.
 */

export class DartboardRenderer {
    /** @param {HTMLCanvasElement} canvas */
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.beds = [];       // parsed bed objects
        this.bounds = null;   // {min:{x,y}, max:{x,y}}
        this.padding = 20;    // px

        // Overlays
        this.shots = [];           // [{x,y}] dartboard coords
        this.aimPoint = null;      // {x,y} or null
        this.heatmapImage = null;  // ImageData or null
    }

    /* ---------- coordinate transforms ---------- */

    /** Dartboard mm → canvas px */
    toCanvas(bx, by) {
        const { min, max } = this.bounds;
        const w = this.canvas.width  - 2 * this.padding;
        const h = this.canvas.height - 2 * this.padding;
        const scaleX = w / (max.x - min.x);
        const scaleY = h / (max.y - min.y);
        const scale = Math.min(scaleX, scaleY);
        const cx = this.canvas.width  / 2;
        const cy = this.canvas.height / 2;
        const midX = (min.x + max.x) / 2;
        const midY = (min.y + max.y) / 2;
        return {
            x: cx + (bx - midX) * scale,
            y: cy - (by - midY) * scale,   // flip y
        };
    }

    /** Canvas px → dartboard mm */
    toBoard(cx, cy) {
        const { min, max } = this.bounds;
        const w = this.canvas.width  - 2 * this.padding;
        const h = this.canvas.height - 2 * this.padding;
        const scaleX = w / (max.x - min.x);
        const scaleY = h / (max.y - min.y);
        const scale = Math.min(scaleX, scaleY);
        const midX = (min.x + max.x) / 2;
        const midY = (min.y + max.y) / 2;
        const canCx = this.canvas.width  / 2;
        const canCy = this.canvas.height / 2;
        return {
            x:  (cx - canCx) / scale + midX,
            y: -(cy - canCy) / scale + midY,
        };
    }

    /* ---------- setup ---------- */

    /** Load beds and bounds produced by target.js */
    setBeds(beds, bounds) {
        this.beds = beds;
        this.bounds = bounds;
    }

    /* ---------- drawing primitives ---------- */

    clear() {
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
    }

    drawBeds() {
        const ctx = this.ctx;
        for (const bed of this.beds) {
            ctx.beginPath();
            const v0 = this.toCanvas(bed.vertices[0].x, bed.vertices[0].y);
            ctx.moveTo(v0.x, v0.y);
            for (let i = 1; i < bed.vertices.length; i++) {
                const v = this.toCanvas(bed.vertices[i].x, bed.vertices[i].y);
                ctx.lineTo(v.x, v.y);
            }
            ctx.closePath();
            ctx.fillStyle = bed.color;
            ctx.fill();
            ctx.strokeStyle = '#555';
            ctx.lineWidth = 0.5;
            ctx.stroke();
        }
    }

    drawNumbers() {
        // Standard dartboard number order and positions
        const NUMBERS = [20,1,18,4,13,6,10,15,2,17,3,19,7,16,8,11,14,9,12,5];
        const SECTOR_DEG = 18;
        const labelRadius = 185; // mm, just outside double ring

        const ctx = this.ctx;
        ctx.fillStyle = '#ccc';
        ctx.font = 'bold 13px sans-serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';

        for (let i = 0; i < NUMBERS.length; i++) {
            const angleDeg = 90 - i * SECTOR_DEG;
            const angleRad = angleDeg * Math.PI / 180;
            const bx = labelRadius * Math.cos(angleRad);
            const by = labelRadius * Math.sin(angleRad);
            const p = this.toCanvas(bx, by);
            ctx.fillText(String(NUMBERS[i]), p.x, p.y);
        }
    }

    drawShots(shots) {
        const ctx = this.ctx;
        for (const s of shots) {
            const p = this.toCanvas(s.x, s.y);
            ctx.beginPath();
            ctx.arc(p.x, p.y, 4, 0, Math.PI * 2);
            ctx.fillStyle = 'rgba(255, 255, 100, 0.85)';
            ctx.fill();
            ctx.strokeStyle = '#000';
            ctx.lineWidth = 1;
            ctx.stroke();
        }
    }

    drawCrosshair(bx, by, { color = '#ff4444', size = 12, label = '' } = {}) {
        const p = this.toCanvas(bx, by);
        const ctx = this.ctx;
        ctx.save();
        ctx.strokeStyle = color;
        ctx.lineWidth = 2;

        ctx.beginPath();
        ctx.moveTo(p.x - size, p.y); ctx.lineTo(p.x + size, p.y);
        ctx.moveTo(p.x, p.y - size); ctx.lineTo(p.x, p.y + size);
        ctx.stroke();

        ctx.beginPath();
        ctx.arc(p.x, p.y, size * 0.6, 0, Math.PI * 2);
        ctx.stroke();

        if (label) {
            ctx.fillStyle = color;
            ctx.font = 'bold 12px sans-serif';
            ctx.textAlign = 'left';
            ctx.fillText(label, p.x + size + 4, p.y + 4);
        }
        ctx.restore();
    }

    drawAimCenter() {
        // Faint crosshair at (0,0) showing where the user should aim
        this.drawCrosshair(0, 0, { color: 'rgba(78, 201, 176, 0.5)', size: 16, label: '' });
    }

    /* ---------- heatmap ---------- */

    /**
     * Build and draw a heatmap overlay.
     * @param {number[][]} grid  2D array [row][col] of values
     * @param {{min:{x,y},max:{x,y}}} gridBounds  dartboard bounds of the grid
     * @param {boolean} invertColors  if true, higher = better (green); else lower = better
     */
    drawHeatmap(grid, gridBounds, invertColors = false) {
        if (!grid || grid.length === 0) return;
        const rows = grid.length;
        const cols = grid[0].length;

        // Find value range
        let lo = Infinity, hi = -Infinity;
        for (let r = 0; r < rows; r++) {
            for (let c = 0; c < cols; c++) {
                const v = grid[r][c];
                if (v < 1e8) { // ignore INFINITE_SCORE sentinels
                    if (v < lo) lo = v;
                    if (v > hi) hi = v;
                }
            }
        }
        if (lo === hi) return;

        const ctx = this.ctx;
        for (let r = 0; r < rows; r++) {
            for (let c = 0; c < cols; c++) {
                const v = grid[r][c];
                if (v >= 1e8) continue; // skip unreachable cells

                // Grid cell center in dartboard coords
                const bx = gridBounds.min.x + (c + 0.5) / cols * (gridBounds.max.x - gridBounds.min.x);
                const by = gridBounds.max.y - (r + 0.5) / rows * (gridBounds.max.y - gridBounds.min.y);

                // Map value to 0..1
                let t = (v - lo) / (hi - lo);
                if (invertColors) t = 1 - t;

                // Hue: 240 (blue, good) → 0 (red, bad)
                const hue = (1 - t) * 240;

                const tl = this.toCanvas(
                    gridBounds.min.x + c / cols * (gridBounds.max.x - gridBounds.min.x),
                    gridBounds.max.y - r / rows * (gridBounds.max.y - gridBounds.min.y)
                );
                const br = this.toCanvas(
                    gridBounds.min.x + (c + 1) / cols * (gridBounds.max.x - gridBounds.min.x),
                    gridBounds.max.y - (r + 1) / rows * (gridBounds.max.y - gridBounds.min.y)
                );

                ctx.fillStyle = `hsla(${hue}, 100%, 50%, 0.55)`;
                ctx.fillRect(tl.x, tl.y, br.x - tl.x, br.y - tl.y);
            }
        }
    }

    /* ---------- composite render calls ---------- */

    /** Render board outline + optional overlays. */
    render({ shots = [], aimPoint = null, heatmap = null, heatmapBounds = null, invertColors = false } = {}) {
        this.clear();
        this.drawBeds();
        if (heatmap) {
            this.drawHeatmap(heatmap, heatmapBounds, invertColors);
        }
        this.drawNumbers();
        if (shots.length > 0) this.drawShots(shots);
        if (aimPoint) {
            this.drawCrosshair(aimPoint.x, aimPoint.y, { color: '#ff4444', size: 14, label: 'Aim' });
        }
    }
}
