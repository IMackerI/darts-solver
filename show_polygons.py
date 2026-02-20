#!/usr/bin/env python3
"""
Visualize polygons from target.out file
"""

import matplotlib.pyplot as plt
from matplotlib.patches import Polygon
import sys


def parse_target_file(filename):
    """Parse the target.out file and extract polygon data"""
    polygons = []
    
    with open(filename, 'r') as f:
        lines = [line.strip() for line in f.readlines() if line.strip()]
    
    idx = 0
    num_polygons = int(lines[idx])
    idx += 1
    
    for _ in range(num_polygons):
        params = lines[idx].split()
        score = int(params[0])
        num_vertices = int(params[1])
        color = params[2]
        poly_type = params[3]
        
        idx += 1

        vertices = []
        for _ in range(num_vertices):
            x, y = map(float, lines[idx].split())
            vertices.append([x, y])
            idx += 1
        
        polygons.append({
            'score': score,
            'vertices': vertices,
            'color': color,
            'type': poly_type
        })
    
    return polygons


def plot_polygons(polygons):
    """Plot the polygons using matplotlib"""
    fig, ax = plt.subplots(figsize=(10, 10))
    
    for poly_data in polygons:
        vertices = poly_data['vertices']
        color = poly_data['color']
        poly_type = poly_data['type']
        score = poly_data['score']
        
        # Create polygon patch
        polygon = Polygon(vertices, alpha=0.6, facecolor=color, 
                         edgecolor='black', linewidth=2)
        ax.add_patch(polygon)
        
        # Calculate center for label
        center_x = sum(v[0] for v in vertices) / len(vertices)
        center_y = sum(v[1] for v in vertices) / len(vertices)
        
        # Add label with score and type
        ax.text(center_x, center_y, f'{score}\n({poly_type})', 
               ha='center', va='center', fontsize=10, 
               bbox=dict(boxstyle='round', facecolor='white', alpha=0.8))
    
    # Set equal aspect ratio and add grid
    ax.set_aspect('equal')
    ax.grid(True, alpha=0.3)
    ax.axhline(y=0, color='k', linewidth=0.5)
    ax.axvline(x=0, color='k', linewidth=0.5)
    
    # Set axis labels
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_title('Darts Target Polygons')
    
    # Auto-scale to fit all polygons
    ax.autoscale()
    
    plt.tight_layout()
    plt.show()


def main():
    filename = sys.argv[1] if len(sys.argv) > 1 else 'target.out'
    
    try:
        polygons = parse_target_file(filename)
        print(f"Loaded {len(polygons)} polygons:")
        for i, poly in enumerate(polygons, 1):
            print(f"  {i}. {poly['color']} {poly['type']} - {poly['score']} points - {len(poly['vertices'])} vertices")
        
        plot_polygons(polygons)
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
