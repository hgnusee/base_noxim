def generate_zigzag_traffic(mesh_dim_x, mesh_dim_y, total_vol=100):
    """
    Generate a zig-zag traffic pattern for a mesh NoC of size mesh_dim_x × mesh_dim_y
    
    Args:
        mesh_dim_x: Number of nodes in X dimension
        mesh_dim_y: Number of nodes in Y dimension
        total_vol: Data volume to transfer between nodes (default: 100)
        
    Returns:
        String containing the traffic table in Noxim format
    """
    # Calculate total number of nodes
    total_nodes = mesh_dim_x * mesh_dim_y
    
    # Generate the zig-zag path
    path = []
    for y in range(mesh_dim_y):
        if y % 2 == 0:  # Even row, left to right
            for x in range(mesh_dim_x):
                path.append(y * mesh_dim_x + x)
        else:  # Odd row, right to left
            for x in range(mesh_dim_x - 1, -1, -1):
                path.append(y * mesh_dim_x + x)
    
    # Generate the traffic table
    traffic_table = "% Zig-Zag Traffic Pattern for {}x{} Mesh NoC\n".format(mesh_dim_x, mesh_dim_y)
    traffic_table += "% taskID layerID [src] [dst] src_minVol src_totalVol dst_minVol dst_totalVol [waitID] [nextID] waitOP\n"
    
    for i in range(len(path) - 1):
        src = path[i]
        dst = path[i + 1]
        task_id = i
        
        # First task has no waitID dependency
        wait_id = -1 if i == 0 else i - 1
        
        # Last task has no nextID - fix here!
        next_id = -1 if i == len(path) - 2 else i + 1
        
        # First node is a source (src_minVol = -1)
        src_min_vol = -1 if i == 0 else 1
        
        # Last destination node is a sink (dst_minVol = -1)
        dst_min_vol = -1 if i == len(path) - 2 else 1
        
        traffic_table += "{} 1 [{}] [{}] {} {} {} {} [{}] [{}] 0\n".format(
            task_id, src, dst, src_min_vol, total_vol, dst_min_vol, total_vol, wait_id, next_id)
    
    return traffic_table

def save_to_file(content, filename):
    """Save content to a file"""
    with open(filename, 'w') as f:
        f.write(content)
    print(f"Traffic pattern written to {filename}")

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 3:
        print("Usage: python generate_zigzag.py <mesh_dim_x> <mesh_dim_y> [data_volume] [output_file]")
        print("Example: python generate_zigzag.py 4 4 100 zigzag_4x4.txt")
        sys.exit(1)
    
    mesh_dim_x = int(sys.argv[1])
    mesh_dim_y = int(sys.argv[2])
    
    total_vol = 100  # Default data volume
    if len(sys.argv) > 3:
        total_vol = int(sys.argv[3])
    
    output_file = f"zigzag_{mesh_dim_x}x{mesh_dim_y}.txt"
    if len(sys.argv) > 4:
        output_file = sys.argv[4]
    
    traffic_table = generate_zigzag_traffic(mesh_dim_x, mesh_dim_y, total_vol)
    save_to_file(traffic_table, output_file)
    
    # Print validation info
    print("\nValidating traffic table...")
    lines = traffic_table.strip().split('\n')
    data_lines = [line for line in lines if not line.startswith('%')]
    last_line = data_lines[-1]
    print(f"Last task: {last_line}")
    if "[-1]" in last_line.split(" "):
        print("✓ Last task correctly has nextID = [-1]")
    else:
        print("✗ Last task does NOT have nextID = [-1]")