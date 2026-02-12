#!/usr/bin/env python3
"""
Generate a standard darts target as a polygon file.

File format:
  Line 1: N (number of polygons/beds)
  Then 2N lines follow in pairs:
    - score (integer: how many points hitting that bed is worth)
    - P x1 y1 x2 y2 ... xP yP  (number of vertices, then CCW coordinates in mm)

Usage:
  python gen_target.py [subdivisions] [output_file]
    subdivisions: number of arc subdivisions per 18-degree sector edge (default 8)
    output_file:  path to write (default "target.txt")
"""

import math
import sys

# --------------- Dartboard dimensions (mm) ---------------
INNER_BULL_RADIUS = 12.7 / 2    # 6.35 mm
OUTER_BULL_RADIUS = 31.8 / 2    # 15.9 mm
TREBLE_INNER_RADIUS = 107.0 - 8.0  # 99 mm
TREBLE_OUTER_RADIUS = 107.0        # 107 mm
DOUBLE_INNER_RADIUS = 170.0 - 8.0  # 162 mm
DOUBLE_OUTER_RADIUS = 170.0        # 170 mm

# Standard dartboard number order (clockwise from top)
NUMBERS = [20, 1, 18, 4, 13, 6, 10, 15, 2, 17,
           3, 19, 7, 16, 8, 11, 14, 9, 12, 5]

SECTOR_ANGLE = math.radians(18)  # each sector spans 18 degrees

# Dartboard colors
COLOR_RED = "#DC143C"
COLOR_GREEN = "#228B22"
COLOR_BLACK = "#000000"
COLOR_CREAM = "#F5F5DC"

def arc_points(radius, angle_start, angle_end, subdivisions):
    """Generate points along an arc from angle_start to angle_end (CCW)."""
    pts = []
    for i in range(subdivisions + 1):
        t = i / subdivisions
        a = angle_start + t * (angle_end - angle_start)
        pts.append((radius * math.cos(a), radius * math.sin(a)))
    return pts


def ring_sector_polygon(r_inner, r_outer, angle_start, angle_end, subdivisions):
    """
    Build a CCW polygon for an annular sector (ring segment).
    Inner arc goes CCW from angle_start to angle_end,
    Outer arc goes CCW from angle_end back to angle_start (reversed outer).
    Together they form a CCW loop.
    """
    inner_arc = arc_points(r_inner, angle_start, angle_end, subdivisions)
    outer_arc = arc_points(r_outer, angle_start, angle_end, subdivisions)
    # CCW: inner arc forward, then outer arc backward
    return inner_arc + list(reversed(outer_arc))


def disc_polygon(radius, subdivisions):
    """Approximate a full disc as a CCW polygon."""
    n = max(subdivisions, 32)
    pts = []
    for i in range(n):
        a = 2 * math.pi * i / n
        pts.append((radius * math.cos(a), radius * math.sin(a)))
    return pts


def generate_target(subdivisions=8):
    """
    Return a list of (score, polygon) where polygon is a list of (x, y) tuples
    in CCW order.  All coordinates in mm, origin at board centre.

    The board coordinate system:
      - 0° points right (+x)
      - 90° points up (+y)
      - The "20" sector is centred at the top (90°).

    Sector i is centred at:  90° - i * 18°   (going clockwise from 20)
    so its boundaries are:  centre ± 9°
    """
    beds = []

    # --- Bullseyes (full discs / annulus) ---
    # Inner bull (Double Bull) = 50 points
    beds.append((50, COLOR_RED, disc_polygon(INNER_BULL_RADIUS, subdivisions * 20)))
    # Outer bull (Single Bull) = 25 points
    for i, number in enumerate(NUMBERS):
        center_angle = math.radians(90) - i * SECTOR_ANGLE
        a_start = center_angle - SECTOR_ANGLE / 2
        a_end = center_angle + SECTOR_ANGLE / 2
        poly = ring_sector_polygon(
            INNER_BULL_RADIUS, OUTER_BULL_RADIUS,
            a_start, a_end, subdivisions
        )
        beds.append((25, COLOR_GREEN, poly))

    # --- Numbered sectors ---
    for i, number in enumerate(NUMBERS):
        center_angle = math.radians(90) - i * SECTOR_ANGLE
        a_start = center_angle - SECTOR_ANGLE / 2
        a_end = center_angle + SECTOR_ANGLE / 2

        # Alternate colors for sectors
        sector_color = COLOR_BLACK if i % 2 == 0 else COLOR_CREAM
        special_color = COLOR_RED if i % 2 == 0 else COLOR_GREEN

        # Inner single  (outer_bull_radius .. treble_inner_radius)
        beds.append((
            number,
            sector_color,
            ring_sector_polygon(OUTER_BULL_RADIUS, TREBLE_INNER_RADIUS,
                                a_start, a_end, subdivisions)
        ))

        # Treble ring  (treble_inner .. treble_outer)  => 3x
        beds.append((
            3 * number,
            special_color,
            ring_sector_polygon(TREBLE_INNER_RADIUS, TREBLE_OUTER_RADIUS,
                                a_start, a_end, subdivisions)
        ))

        # Outer single  (treble_outer .. double_inner)
        beds.append((
            number,
            sector_color,
            ring_sector_polygon(TREBLE_OUTER_RADIUS, DOUBLE_INNER_RADIUS,
                                a_start, a_end, subdivisions)
        ))

    
        # Double ring  (double_inner .. double_outer)  => 2x
        beds.append((
            2 * number,
            special_color,
            ring_sector_polygon(DOUBLE_INNER_RADIUS, DOUBLE_OUTER_RADIUS,
                                a_start, a_end, subdivisions)
        ))

    return beds


def write_target(beds, filename):
    with open(filename, "w") as f:
        f.write(f"{len(beds)}\n")
        for score, color, poly in beds:
            f.write(f"{score} {len(poly)} {color}\n")
            coords = " ".join(f"{x:.6f} {y:.6f}" for x, y in poly)
            f.write(f"{coords}\n")


def main():
    subdivisions = int(sys.argv[1]) if len(sys.argv) > 1 else 8
    output = sys.argv[2] if len(sys.argv) > 2 else "target.out"
    beds = generate_target(subdivisions)
    write_target(beds, output)
    print(f"Wrote {len(beds)} beds to {output}  (subdivisions={subdivisions})")


if __name__ == "__main__":
    main()
