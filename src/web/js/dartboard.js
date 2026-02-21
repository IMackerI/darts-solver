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

/**
 * Attempt to reproduce the 'turbo' colormap from matplotlib.
 * Input: t in [0, 1] (0 = best / low throws, 1 = worst / high throws).
 * Output: [r, g, b] each 0-255.
 */
function turboColor(t) {
    // Piecewise linear approximation of turbo
    // 0.00 → dark blue   (48, 18, 59)
    // 0.15 → blue        (69, 91, 205)
    // 0.30 → cyan        (42, 176, 227)
    // 0.45 → green       (86, 230, 99)
    // 0.60 → yellow      (207, 234, 49)
    // 0.75 → orange      (252, 163, 17)
    // 0.90 → red         (229, 56, 19)
    // 1.00 → dark red    (122, 4, 3)
    const stops = [
        [0.00,  48, 18, 59],
        [0.15,  69, 91,205],
        [0.30,  42,176,227],
        [0.45,  86,230, 99],
        [0.60, 207,234, 49],
        [0.75, 252,163, 17],
        [0.90, 229, 56, 19],
        [1.00, 122,  4,  3],
    ];
    const tc = Math.max(0, Math.min(1, t));
    for (let i = 0; i < stops.length - 1; i++) {
        if (tc <= stops[i + 1][0]) {
            const f = (tc - stops[i][0]) / (stops[i + 1][0] - stops[i][0]);
            return [
                Math.round(stops[i][1] + f * (stops[i + 1][1] - stops[i][1])),
                Math.round(stops[i][2] + f * (stops[i + 1][2] - stops[i][2])),
                Math.round(stops[i][3] + f * (stops[i + 1][3] - stops[i][3])),
            ];
        }
    }
    return [122, 4, 3];
}

export class DartboardRenderer {
    /** @param {HTMLCanvasElement} canvas */
    constructor(canvas) {
        this.canvas = canvas;
        this.ctx = canvas.getContext('2d');
        this.beds = [];       // parsed bed objects
        this.bounds = null;   // {min:{x,y}, max:{x,y}}
        this.padding = 40;    // px – enough room for number labels

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

    /** Load beds and bounds produced by target.js.
     *  Expands bounds so number labels aren't clipped. */
    setBeds(beds, bounds) {
        this.beds = beds;
        // Expand bounds to include number labels (~185 mm radius + text)
        const labelMargin = 200;
        this.bounds = {
            min: { x: Math.min(bounds.min.x, -labelMargin), y: Math.min(bounds.min.y, -labelMargin) },
            max: { x: Math.max(bounds.max.x,  labelMargin), y: Math.max(bounds.max.y,  labelMargin) },
        };
    }

    /* ---------- drawing primitives ---------- */

    clear() {
        this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
    }

    drawBeds(outlineOnly = false) {
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
            if (!outlineOnly) {
                ctx.fillStyle = bed.color;
                ctx.fill();
            }
            ctx.strokeStyle = outlineOnly ? 'rgba(200,200,200,0.45)' : '#555';
            ctx.lineWidth = outlineOnly ? 0.8 : 0.5;
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
     * @param {boolean} invertColors  if true, higher = better (maxPoints); else lower = better (minThrows)
     */
    drawHeatmap(grid, gridBounds, invertColors = false) {
        if (!grid || grid.length === 0) return;
        const rows = grid.length;
        const cols = grid[0].length;

        if (invertColors) {
            // --- maxPoints mode: linear scaling, higher = better ---
            this._drawHeatmapLinear(grid, gridBounds, rows, cols);
        } else {
            // --- minThrows mode: log scaling matching visualize_darts.py ---
            this._drawHeatmapLog(grid, gridBounds, rows, cols);
        }
    }

    /**
     * Max-points heatmap: linear scaling, higher = green, lower = red.
     */
    _drawHeatmapLinear(grid, gridBounds, rows, cols) {
        let lo = Infinity, hi = -Infinity;
        for (let r = 0; r < rows; r++) {
            for (let c = 0; c < cols; c++) {
                const v = grid[r][c];
                if (v < 1e8) {
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
                if (v >= 1e8) continue;

                let t = (v - lo) / (hi - lo); // 0 = worst, 1 = best
                t = Math.pow(t, 0.6);

                const hue = t * 120; // 0 (red) → 120 (green)
                const lightness = 35 + t * 20;

                const tl = this.toCanvas(
                    gridBounds.min.x + c / cols * (gridBounds.max.x - gridBounds.min.x),
                    gridBounds.max.y - r / rows * (gridBounds.max.y - gridBounds.min.y)
                );
                const br = this.toCanvas(
                    gridBounds.min.x + (c + 1) / cols * (gridBounds.max.x - gridBounds.min.x),
                    gridBounds.max.y - (r + 1) / rows * (gridBounds.max.y - gridBounds.min.y)
                );

                ctx.fillStyle = `hsla(${hue}, 100%, ${lightness}%, 0.85)`;
                ctx.fillRect(tl.x, tl.y, br.x - tl.x, br.y - tl.y);
            }
        }
    }

    /**
     * Min-throws heatmap: log10 scaling with baseline subtraction.
     * Matches the logic in visualize_darts.py:
     *   - mask values == 0 or > 1000
     *   - subtract minimum (baseline)
     *   - log10(val + epsilon)
     *   - clamp to [log10(epsilon), log10(vmax + epsilon)]
     *   - turbo colormap
     */
    _drawHeatmapLog(grid, gridBounds, rows, cols) {
        const MASK_THRESHOLD = 1000; // matches Python
        const VMAX = 50;             // max "throws above best" to show
        const EPSILON = 0.1;

        // Find minimum valid value (the baseline)
        let dataMin = Infinity;
        for (let r = 0; r < rows; r++) {
            for (let c = 0; c < cols; c++) {
                const v = grid[r][c];
                if (v > 0 && v <= MASK_THRESHOLD && v < dataMin) dataMin = v;
            }
        }
        if (dataMin === Infinity) return; // no valid data

        const vminLog = Math.log10(EPSILON);
        const vmaxLog = Math.log10(VMAX + EPSILON);

        const ctx = this.ctx;
        for (let r = 0; r < rows; r++) {
            for (let c = 0; c < cols; c++) {
                const v = grid[r][c];
                if (v <= 0 || v > MASK_THRESHOLD) continue; // masked

                const normalized = v - dataMin;
                const logged = Math.log10(normalized + EPSILON);
                const clamped = Math.max(vminLog, Math.min(vmaxLog, logged));

                // Map to 0..1
                const t = (clamped - vminLog) / (vmaxLog - vminLog);

                // Turbo-like colormap: blue → cyan → green → yellow → red
                const [rr, gg, bb] = turboColor(t);

                const tl = this.toCanvas(
                    gridBounds.min.x + c / cols * (gridBounds.max.x - gridBounds.min.x),
                    gridBounds.max.y - r / rows * (gridBounds.max.y - gridBounds.min.y)
                );
                const br = this.toCanvas(
                    gridBounds.min.x + (c + 1) / cols * (gridBounds.max.x - gridBounds.min.x),
                    gridBounds.max.y - (r + 1) / rows * (gridBounds.max.y - gridBounds.min.y)
                );

                ctx.fillStyle = `rgba(${rr},${gg},${bb},0.7)`;
                ctx.fillRect(tl.x, tl.y, br.x - tl.x, br.y - tl.y);
            }
        }
    }

    /* ---------- composite render calls ---------- */

    /** Render board outline + optional overlays. */
    render({ shots = [], aimPoint = null, heatmap = null, heatmapBounds = null, invertColors = false } = {}) {
        this.clear();
        const hasHeatmap = heatmap != null;
        if (!hasHeatmap) {
            this.drawBeds(false);           // filled colours
        } else {
            this.drawHeatmap(heatmap, heatmapBounds, invertColors);
            this.drawBeds(true);            // outlines only, on top of heatmap
        }
        this.drawNumbers();
        if (shots.length > 0) this.drawShots(shots);
        if (aimPoint) {
            this.drawCrosshair(aimPoint.x, aimPoint.y, { color: '#ff4444', size: 14, label: 'Aim' });
        }
    }
}
