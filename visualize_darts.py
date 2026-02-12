#!/usr/bin/env python3
"""
Darts Target Visualization Tool
Displays a dartboard with optimal aim points for different game states.
"""

import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np
import argparse


def parse_results(filename):
    """Parse the results file and extract state, expected throws, and aim coordinates."""
    results = []
    avg_distance = None
    with open(filename, 'r') as f:
        for line_num, line in enumerate(f, 1):
            if line.startswith('Average distance from mean:'):
                try:
                    # Extract average distance
                    avg_dist_start = line.find('Average distance from mean:') + 27
                    avg_distance = float(line[avg_dist_start:].strip())
                except Exception as e:
                    print(f"Warning: Could not parse average distance on line {line_num}")
            elif line.startswith('State:'):
                try:
                    # Use regex or careful parsing to extract values
                    # Format: State: X, Expected throws to finish: Y, Best aim: (a, b)
                    
                    # Extract state
                    state_start = line.find('State: ') + 7
                    state_end = line.find(',', state_start)
                    state = int(line[state_start:state_end].strip())
                    
                    # Extract expected throws
                    expected_start = line.find('Expected throws to finish: ') + 27
                    expected_end = line.find(',', expected_start)
                    expected = float(line[expected_start:expected_end].strip())
                    
                    # Extract aim coordinates
                    aim_start = line.find('Best aim: (') + 11
                    aim_end = line.find(')', aim_start)
                    aim_str = line[aim_start:aim_end].strip()
                    
                    # Split coordinates by comma
                    coords = [c.strip() for c in aim_str.split(',')]
                    if len(coords) != 2:
                        print(f"Warning: Line {line_num} has invalid coordinate format: {aim_str}")
                        continue
                    x, y = map(float, coords)
                    
                    results.append({
                        'state': state,
                        'expected_throws': expected,
                        'aim': (x, y)
                    })
                except Exception as e:
                    print(f"Warning: Could not parse line {line_num}: {line.strip()}")
                    print(f"  Error: {e}")
                    continue
    return results, avg_distance


def draw_dartboard(ax):
    """Draw a standard dartboard layout."""
    # Dartboard parameters (standard measurements in mm)
    # Bull's eye: 12.7mm diameter (inner bull)
    # Outer bull: 31.8mm diameter
    # Triple ring: 99mm to 107mm from center
    # Double ring: 162mm to 170mm from center
    
    # Standard dartboard numbers in clockwise order starting from top
    dartboard_numbers = [20, 1, 18, 4, 13, 6, 10, 15, 2, 17, 3, 19, 7, 16, 8, 11, 14, 9, 12, 5]
    
    # Colors for segments (alternating)
    segment_colors_dark = ['#000000', '#1a1a1a']  # Black shades
    segment_colors_light = ['#f5f5dc', '#fffacd']  # Beige/cream shades
    ring_colors = ['#d62828', '#2d6a4f']  # Red and green for double/triple
    
    # Draw 20 sectors with alternating colors
    for i in range(20):
        angle_start = 99 - i * 18  # Start from top (90 degrees) with 9 degree offset
        angle_end = angle_start - 18
        
        # Determine if this sector should be dark or light
        is_dark = i % 2 == 0
        
        # Double ring (outer edge of dartboard)
        wedge = patches.Wedge((0, 0), 170, angle_end, angle_start,
                             width=8,
                             facecolor=ring_colors[i % 2],
                             edgecolor='black', linewidth=0.5)
        ax.add_patch(wedge)
        
        # Segments between double and triple
        wedge = patches.Wedge((0, 0), 162, angle_end, angle_start,
                             width=55,
                             facecolor=segment_colors_dark[i % 2] if is_dark else segment_colors_light[i % 2],
                             edgecolor='black', linewidth=0.5)
        ax.add_patch(wedge)
        
        # Triple ring
        wedge = patches.Wedge((0, 0), 107, angle_end, angle_start,
                             width=8,
                             facecolor=ring_colors[i % 2],
                             edgecolor='black', linewidth=0.5)
        ax.add_patch(wedge)
        
        # Inner segments (between triple and bull)
        wedge = patches.Wedge((0, 0), 99, angle_end, angle_start,
                             width=83.1,
                             facecolor=segment_colors_dark[i % 2] if is_dark else segment_colors_light[i % 2],
                             edgecolor='black', linewidth=0.5)
        ax.add_patch(wedge)
        
        # Add numbers around the dartboard
        angle_mid = np.radians(angle_start - 9)  # Middle of segment
        text_radius = 188
        x = text_radius * np.cos(angle_mid)
        y = text_radius * np.sin(angle_mid)
        ax.text(x, y, str(dartboard_numbers[i]), 
               ha='center', va='center', fontsize=12, fontweight='bold',
               color='white', bbox=dict(boxstyle='circle', facecolor='black', edgecolor='none', pad=0.3))
    
    # Outer bull (green)
    outer_bull = plt.Circle((0, 0), 15.9, facecolor='#2d6a4f', zorder=10, edgecolor='black', linewidth=1)
    ax.add_patch(outer_bull)
    
    # Bull's eye (red)
    bulls_eye = plt.Circle((0, 0), 6.35, facecolor='#d62828', zorder=11, edgecolor='black', linewidth=1)
    ax.add_patch(bulls_eye)
    
    # Outer black ring
    outer_ring = plt.Circle((0, 0), 170, facecolor='none', edgecolor='black', linewidth=3, zorder=12)
    ax.add_patch(outer_ring)
    
    # Labels
    ax.set_xlim(-210, 210)
    ax.set_ylim(-210, 210)
    ax.set_aspect('equal')
    ax.grid(False)
    ax.set_facecolor('#2a2a2a')
    ax.set_xlabel('X (mm)', color='white', fontsize=10)
    ax.set_ylabel('Y (mm)', color='white', fontsize=10)
    ax.tick_params(colors='white')


def visualize_state(results, state_num, avg_distance=None):
    """Visualize a specific state's optimal aim point."""
    fig, ax = plt.subplots(figsize=(10, 10), facecolor='#1a1a1a')
    
    # Draw dartboard
    draw_dartboard(ax)
    
    # Find the state
    state_data = None
    for r in results:
        if r['state'] == state_num:
            state_data = r
            break
    
    if state_data is None:
        print(f"State {state_num} not found in results.")
        return
    
    # Plot the optimal aim point
    x, y = state_data['aim']
    
    # Draw average distance circle around the aim point if provided
    if avg_distance is not None:
        avg_circle = plt.Circle((x, y), avg_distance, facecolor='none', 
                               edgecolor='#00ffff', linewidth=2, linestyle='--', 
                               alpha=0.6, zorder=13, label=f'Avg. Distance: {avg_distance:.1f}mm')
        ax.add_patch(avg_circle)
    
    # Outer glow
    ax.plot(x, y, 'o', color='#ffff00', markersize=25, alpha=0.3, zorder=14)
    ax.plot(x, y, 'o', color='#ffff00', markersize=20, alpha=0.5, zorder=15)
    # Main marker
    ax.plot(x, y, 'o', color='#ffff00', markersize=15, markeredgecolor='#ff0000', 
            markeredgewidth=2, label='Optimal Aim', zorder=16)
    ax.plot(x, y, '+', color='#ff0000', markersize=25, markeredgewidth=3, zorder=17)
    
    # Add crosshairs
    ax.axhline(y=y, color='#ffff00', linestyle='--', alpha=0.5, linewidth=1.5, zorder=13)
    ax.axvline(x=x, color='#ffff00', linestyle='--', alpha=0.5, linewidth=1.5, zorder=13)
    
    title = (f'State: {state_data["state"]} | '
             f'Expected Throws: {state_data["expected_throws"]:.4f}\n'
             f'Optimal Aim: ({x:.2f}, {y:.2f})')
    ax.set_title(title, fontsize=14, fontweight='bold', color='white', pad=20)
    ax.legend(loc='upper right', facecolor='#2a2a2a', edgecolor='white', 
             labelcolor='white', fontsize=10)
    
    plt.tight_layout()
    plt.show()


def visualize_all_states(results, avg_distance=None):
    """Visualize all states' optimal aim points on one dartboard."""
    fig, ax = plt.subplots(figsize=(12, 12), facecolor='#1a1a1a')
    
    # Draw dartboard
    draw_dartboard(ax)
    
    # Create color map for states
    colors = plt.cm.plasma(np.linspace(0, 1, len(results)))
    
    # Plot all aim points
    for idx, r in enumerate(results):
        if r['state'] == 0:  # Skip state 0 (finished)
            continue
        x, y = r['aim']
        
        # Draw average distance circle around each aim point if provided
        if avg_distance is not None:
            avg_circle = plt.Circle((x, y), avg_distance, facecolor='none', 
                                   edgecolor='#00ffff', linewidth=1, linestyle='--', 
                                   alpha=0.2, zorder=12)
            ax.add_patch(avg_circle)
        
        ax.plot(x, y, 'o', markersize=6, alpha=0.8, color=colors[idx], 
               markeredgecolor='white', markeredgewidth=0.5, zorder=13)
    
    ax.set_title('All Optimal Aim Points', fontsize=14, fontweight='bold', 
                color='white', pad=20)
    
    # Add colorbar for state numbers
    sm = plt.cm.ScalarMappable(cmap=plt.cm.plasma, 
                               norm=plt.Normalize(vmin=0, vmax=len(results)))
    sm.set_array([])
    cbar = plt.colorbar(sm, ax=ax, label='State Number')
    cbar.ax.yaxis.label.set_color('white')
    cbar.ax.tick_params(colors='white')
    
    plt.tight_layout()
    plt.show()


def interactive_visualizer(results, avg_distance=None):
    """Interactive visualizer that allows cycling through states."""
    current_state = [1]  # Use list to allow modification in nested function
    
    fig, ax = plt.subplots(figsize=(10, 10), facecolor='#1a1a1a')
    
    def update_plot(state_num):
        ax.clear()
        draw_dartboard(ax)
        
        # Find the state
        state_data = None
        for r in results:
            if r['state'] == state_num:
                state_data = r
                break
        
        if state_data is None:
            return
        
        # Plot the optimal aim point
        x, y = state_data['aim']
        
        # Draw average distance circle around the aim point if provided
        if avg_distance is not None:
            avg_circle = plt.Circle((x, y), avg_distance, facecolor='none', 
                                   edgecolor='#00ffff', linewidth=2, linestyle='--', 
                                   alpha=0.6, zorder=13, label=f'Avg. Distance: {avg_distance:.1f}mm')
            ax.add_patch(avg_circle)
        
        # Outer glow
        ax.plot(x, y, 'o', color='#ffff00', markersize=25, alpha=0.3, zorder=14)
        ax.plot(x, y, 'o', color='#ffff00', markersize=20, alpha=0.5, zorder=15)
        # Main marker
        ax.plot(x, y, 'o', color='#ffff00', markersize=15, markeredgecolor='#ff0000', 
                markeredgewidth=2, label='Optimal Aim', zorder=16)
        ax.plot(x, y, '+', color='#ff0000', markersize=25, markeredgewidth=3, zorder=17)
        
        # Add crosshairs
        ax.axhline(y=y, color='#ffff00', linestyle='--', alpha=0.5, linewidth=1.5, zorder=13)
        ax.axvline(x=x, color='#ffff00', linestyle='--', alpha=0.5, linewidth=1.5, zorder=13)
        
        title = (f'State: {state_data["state"]} | '
                 f'Expected Throws: {state_data["expected_throws"]:.4f}\n'
                 f'Optimal Aim: ({x:.2f}, {y:.2f})\n'
                 f'[Use arrow keys to navigate, ESC to exit]')
        ax.set_title(title, fontsize=14, fontweight='bold', color='white', pad=20)
        ax.legend(loc='upper right', facecolor='#2a2a2a', edgecolor='white', 
                 labelcolor='white', fontsize=10)
        
        fig.canvas.draw()
    
    def on_key(event):
        max_state = max(r['state'] for r in results)
        
        if event.key == 'right' or event.key == 'up':
            current_state[0] = min(current_state[0] + 1, max_state)
            update_plot(current_state[0])
        elif event.key == 'left' or event.key == 'down':
            current_state[0] = max(1, current_state[0] - 1)
            update_plot(current_state[0])
        elif event.key == 'escape':
            plt.close()
    
    fig.canvas.mpl_connect('key_press_event', on_key)
    update_plot(current_state[0])
    
    plt.tight_layout()
    plt.show()


def main():
    parser = argparse.ArgumentParser(description='Visualize darts optimal aim points')
    parser.add_argument('--file', '-f', default='results.out', 
                       help='Results file to read (default: results.out)')
    parser.add_argument('--state', '-s', type=int, 
                       help='Show specific state number')
    parser.add_argument('--all', '-a', action='store_true', 
                       help='Show all states on one dartboard')
    parser.add_argument('--interactive', '-i', action='store_true', 
                       help='Interactive mode (use arrow keys to navigate)')
    
    args = parser.parse_args()
    
    # Parse results
    try:
        results, avg_distance = parse_results(args.file)
        print(f"Loaded {len(results)} states from {args.file}")
        if avg_distance is not None:
            print(f"Average distance from mean: {avg_distance:.2f}mm")
    except FileNotFoundError:
        print(f"Error: File '{args.file}' not found.")
        return
    except Exception as e:
        print(f"Error parsing file: {e}")
        return
    
    # Choose visualization mode
    if args.all:
        visualize_all_states(results, avg_distance)
    elif args.interactive:
        interactive_visualizer(results, avg_distance)
    elif args.state is not None:
        visualize_state(results, args.state, avg_distance)
    else:
        # Default to interactive mode
        print("Starting interactive mode (use --help for options)")
        interactive_visualizer(results, avg_distance)


if __name__ == '__main__':
    main()
