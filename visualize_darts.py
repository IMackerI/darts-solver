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
    """Parse the results file and extract state, expected throws, aim coordinates, and heat maps."""
    results = []
    avg_distance = None
    
    with open(filename, 'r') as f:
        lines = f.readlines()
    
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        
        if line.startswith('Average distance from mean:'):
            try:
                avg_dist_start = line.find('Average distance from mean:') + 27
                avg_distance = float(line[avg_dist_start:].strip())
            except Exception as e:
                print(f"Warning: Could not parse average distance on line {i+1}")
            i += 1
            
        elif line.startswith('State:'):
            try:
                # Parse state number
                state = int(line.split(':')[1].strip())
                i += 1
                
                # Parse expected throws and aim point
                line = lines[i].strip()
                expected_start = line.find('Expected throws to finish: ') + 27
                expected_end = line.find(',', expected_start)
                expected = float(line[expected_start:expected_end].strip())
                
                aim_start = line.find('Best aim: (') + 11
                aim_end = line.find(')', aim_start)
                aim_str = line[aim_start:aim_end].strip()
                coords = [c.strip() for c in aim_str.split(',')]
                x, y = map(float, coords)
                i += 1
                
                # Parse heat map if present
                heat_map = None
                heat_map_extent = None
                if i < len(lines) and lines[i].strip().startswith('Heat map for state'):
                    i += 1  # Skip the "Heat map for state" line
                    
                    # Check for heat map extent
                    if i < len(lines) and lines[i].strip().startswith('Heat map extent:'):
                        extent_line = lines[i].strip()
                        extent_parts = extent_line.split(':')[1].strip().split()
                        if len(extent_parts) == 4:
                            min_x, min_y, max_x, max_y = map(float, extent_parts)
                            heat_map_extent = (min_x, min_y, max_x, max_y)
                        i += 1
                    
                    heat_map = []
                    while i < len(lines) and lines[i].strip() and not lines[i].strip().startswith('State:'):
                        row = [float(val) for val in lines[i].strip().split()]
                        if row:  # Only add non-empty rows
                            heat_map.append(row)
                        i += 1
                    if not heat_map:
                        heat_map = None
                else:
                    i += 1
                
                results.append({
                    'state': state,
                    'expected_throws': expected,
                    'aim': (x, y),
                    'heat_map': heat_map,
                    'heat_map_extent': heat_map_extent
                })
            except Exception as e:
                print(f"Warning: Could not parse state on line {i+1}: {e}")
                i += 1
        else:
            i += 1
    
    return results, avg_distance


def draw_dartboard(ax, show_colors=True):
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
    
    # Gray colors for when heat map is enabled
    gray_color = '#3a3a3a'
    
    # Draw 20 sectors with alternating colors
    for i in range(20):
        angle_start = 99 - i * 18  # Start from top (90 degrees) with 9 degree offset
        angle_end = angle_start - 18
        
        # Determine if this sector should be dark or light
        is_dark = i % 2 == 0
        
        if show_colors:
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
        else:
            # Just show sector outlines in gray
            wedge = patches.Wedge((0, 0), 170, angle_end, angle_start,
                                 width=170,
                                 facecolor=gray_color,
                                 edgecolor='white', linewidth=0.3, alpha=0.3)
            ax.add_patch(wedge)
        
        # Add numbers around the dartboard (with high zorder to be in front of heat map)
        angle_mid = np.radians(angle_start - 9)  # Middle of segment
        text_radius = 188
        x = text_radius * np.cos(angle_mid)
        y = text_radius * np.sin(angle_mid)
        ax.text(x, y, str(dartboard_numbers[i]), 
               ha='center', va='center', fontsize=12, fontweight='bold',
               color='white', bbox=dict(boxstyle='circle', facecolor='black', edgecolor='none', pad=0.3),
               zorder=15)
    
    # Outer bull and bull's eye - always draw these with lower z-order so heat map is on top
    if show_colors:
        # Outer bull (green)
        outer_bull = plt.Circle((0, 0), 15.9, facecolor='#2d6a4f', zorder=3, edgecolor='black', linewidth=1)
        ax.add_patch(outer_bull)
        
        # Bull's eye (red)
        bulls_eye = plt.Circle((0, 0), 6.35, facecolor='#d62828', zorder=3, edgecolor='black', linewidth=1)
        ax.add_patch(bulls_eye)
    else:
        # Gray circles when heat map is on
        outer_bull = plt.Circle((0, 0), 15.9, facecolor=gray_color, zorder=3, edgecolor='white', linewidth=0.5, alpha=0.3)
        ax.add_patch(outer_bull)
        
        bulls_eye = plt.Circle((0, 0), 6.35, facecolor=gray_color, zorder=3, edgecolor='white', linewidth=0.5, alpha=0.3)
        ax.add_patch(bulls_eye)
    
    # Draw sector dividing lines and ring outlines on top of heat map
    for i in range(20):
        angle = np.radians(99 - i * 18)
        x_end = 170 * np.cos(angle)
        y_end = 170 * np.sin(angle)
        ax.plot([0, x_end], [0, y_end], color='black', linewidth=1, zorder=10, alpha=0.8)
    
    # Draw ring outlines (triple, double, outer bull, bull's eye)
    for radius in [107, 99, 162, 170, 15.9, 6.35]:
        circle = plt.Circle((0, 0), radius, facecolor='none', edgecolor='black', 
                           linewidth=1.5 if radius == 170 else 1, zorder=10, alpha=0.8)
        ax.add_patch(circle)
    
    # Labels
    ax.set_xlim(-210, 210)
    ax.set_ylim(-210, 210)
    ax.set_aspect('equal')
    ax.grid(False)
    ax.set_facecolor('#2a2a2a')
    ax.set_xlabel('X (mm)', color='white', fontsize=10)
    ax.set_ylabel('Y (mm)', color='white', fontsize=10)
    ax.tick_params(colors='white')


def draw_heat_map(ax, heat_map, heat_map_extent=None, extent=200, vmax=50, colorbar_ax=None):
    """Draw heat map overlay on the dartboard."""
    if heat_map is None or len(heat_map) == 0:
        return None
    
    # Use provided extent if available
    if heat_map_extent is not None:
        min_x, min_y, max_x, max_y = heat_map_extent
        extent_x = max(abs(min_x), abs(max_x))
        extent_y = max(abs(min_y), abs(max_y))
        extent = max(extent_x, extent_y)
    
    # Convert heat map to numpy array
    heat_array = np.array(heat_map)
    
    # Mask zero values (outside dartboard or invalid areas)
    # Also mask very large values (essentially infinity/impossible states)
    heat_masked = np.ma.masked_where((heat_array == 0) | (heat_array > 1000), heat_array)
    
    # Get actual min from the data
    if heat_masked.count() == 0:  # No valid data
        return None
    
    data_min = heat_masked.min()
    
    # Normalize by subtracting the minimum from all values
    heat_normalized = heat_masked - data_min
    
    # Apply logarithmic transformation for better visualization of small differences
    # Use fixed epsilon for consistent scale starting point
    epsilon = 0.1
    heat_log = np.log10(heat_normalized + epsilon)
    
    # Use fixed scale: epsilon to vmax
    vmax_log = np.log10(vmax + epsilon)
    vmin_log = np.log10(epsilon)
    
    # Clamp the logged data
    heat_clamped = np.clip(heat_log, vmin_log, vmax_log)
    
    # Create custom colormap with more color variation for better distinction
    # 'turbo' has blue->cyan->green->yellow->orange->red progression
    cmap = plt.cm.turbo
    cmap.set_bad(alpha=0)  # Make masked values transparent
    
    # Display heat map with proper extent and normalization
    # The heat map covers from -extent to +extent in both x and y
    im = ax.imshow(heat_clamped, 
                   extent=[-extent, extent, -extent, extent],
                   origin='lower',
                   cmap=cmap,
                   alpha=0.7,
                   zorder=8,
                   vmin=vmin_log,
                   vmax=vmax_log,
                   interpolation='nearest')
    
    # Add colorbar if not updating existing one
    if colorbar_ax is None:
        cbar = plt.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
        cbar.set_label(f'Throws above min ({data_min:.2f})', rotation=270, labelpad=20, color='white')
        cbar.ax.tick_params(colors='white')
        
        # Set custom tick labels to show actual values (relative to minimum)
        tick_positions = cbar.get_ticks()
        tick_labels = [f'{10**pos:.2f}' if pos < 1 else f'{10**pos:.1f}' for pos in tick_positions]
        cbar.set_ticks(tick_positions)
        cbar.set_ticklabels(tick_labels)
        
        return im, cbar
    
    return im, None


def visualize_state(results, state_num, avg_distance=None, show_heatmap=True, heat_vmax=50):
    """Visualize a specific state's optimal aim point."""
    fig, ax = plt.subplots(figsize=(10, 10), facecolor='#1a1a1a')
    
    # Find the state
    state_data = None
    for r in results:
        if r['state'] == state_num:
            state_data = r
            break
    
    if state_data is None:
        print(f"State {state_num} not found in results.")
        return
    
    # Determine if we should show dartboard colors
    has_heatmap = show_heatmap and state_data.get('heat_map')
    
    # Draw dartboard (disable colors if heat map is shown)
    draw_dartboard(ax, show_colors=not has_heatmap)
    
    # Draw heat map if available and requested
    if show_heatmap and state_data.get('heat_map'):
        draw_heat_map(ax, state_data['heat_map'], state_data.get('heat_map_extent'), vmax=heat_vmax)
    
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


def interactive_visualizer(results, avg_distance=None, show_heatmap=True, heat_vmax=50):
    """Interactive visualizer that allows cycling through states."""
    current_state = [1]  # Use list to allow modification in nested function
    current_colorbar = [None]  # Store colorbar reference
    
    fig, ax = plt.subplots(figsize=(10, 10), facecolor='#1a1a1a')
    
    def update_plot(state_num):
        # Remove existing colorbar if present
        if current_colorbar[0] is not None:
            current_colorbar[0].remove()
            current_colorbar[0] = None
        
        ax.clear()
        
        # Find the state
        state_data = None
        for r in results:
            if r['state'] == state_num:
                state_data = r
                break
        
        if state_data is None:
            return
        
        # Determine if we should show dartboard colors
        has_heatmap = show_heatmap and state_data.get('heat_map')
        
        # Draw dartboard (disable colors if heat map is shown)
        draw_dartboard(ax, show_colors=not has_heatmap)
        
        # Draw heat map if available and requested
        if show_heatmap and state_data.get('heat_map'):
            result = draw_heat_map(ax, state_data['heat_map'], state_data.get('heat_map_extent'), vmax=heat_vmax)
            if result and result[1] is not None:
                current_colorbar[0] = result[1]
        
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
                 f'[Use arrow keys to navigate, H to toggle heat map, ESC to exit]')
        ax.set_title(title, fontsize=14, fontweight='bold', color='white', pad=20)
        ax.legend(loc='upper right', facecolor='#2a2a2a', edgecolor='white', 
                 labelcolor='white', fontsize=10)
        
        fig.canvas.draw()
    
    def on_key(event):
        nonlocal show_heatmap
        max_state = max(r['state'] for r in results)
        
        if event.key == 'right' or event.key == 'up':
            current_state[0] = min(current_state[0] + 1, max_state)
            update_plot(current_state[0])
        elif event.key == 'left' or event.key == 'down':
            current_state[0] = max(1, current_state[0] - 1)
            update_plot(current_state[0])
        elif event.key == 'h':
            show_heatmap = not show_heatmap
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
    parser.add_argument('--no-heatmap', action='store_true',
                       help='Disable heat map overlay')
    parser.add_argument('--heat-max', type=float, default=50,
                       help='Maximum value for heat map color scale (default: 50)')
    
    args = parser.parse_args()
    show_heatmap = not args.no_heatmap
    
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
        interactive_visualizer(results, avg_distance, show_heatmap, args.heat_max)
    elif args.state is not None:
        visualize_state(results, args.state, avg_distance, show_heatmap, args.heat_max)
    else:
        # Default to interactive mode
        print("Starting interactive mode (use --help for options)")
        interactive_visualizer(results, avg_distance, show_heatmap, args.heat_max)


if __name__ == '__main__':
    main()
