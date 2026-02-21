/**
 * Parse a target.out file into an array of bed objects for JS rendering.
 *
 * Format (per gen_target.py output):
 *   num_beds
 *   score num_vertices color type
 *   x1 y1 x2 y2 ...          (coordinates may span multiple lines)
 *
 * We tokenise the entire file and walk through tokens, which handles
 * coordinates that wrap across lines.
 */
export function parseTarget(text) {
    const tokens = text.trim().split(/\s+/);
    let i = 0;
    const next = () => tokens[i++];

    const numBeds = parseInt(next(), 10);
    const beds = [];

    for (let b = 0; b < numBeds; b++) {
        const score    = parseInt(next(), 10);
        const numVerts = parseInt(next(), 10);
        const color    = next();          // e.g. "#DC143C"
        const type     = next();          // "single" | "double" | "treble"

        const vertices = [];
        for (let v = 0; v < numVerts; v++) {
            vertices.push({ x: parseFloat(next()), y: parseFloat(next()) });
        }
        beds.push({ score, numVerts, color, type, vertices });
    }
    return beds;
}

/**
 * Compute bounding box of all beds.
 * Returns { min: {x,y}, max: {x,y} }
 */
export function computeBounds(beds) {
    let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
    for (const bed of beds) {
        for (const v of bed.vertices) {
            if (v.x < minX) minX = v.x;
            if (v.y < minY) minY = v.y;
            if (v.x > maxX) maxX = v.x;
            if (v.y > maxY) maxY = v.y;
        }
    }
    return { min: { x: minX, y: minY }, max: { x: maxX, y: maxY } };
}
