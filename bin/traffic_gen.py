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

def generate_onerow_traffic(mesh_dim_x, mesh_dim_y, total_vol=100):
    """
    Generate a "onerow" traffic pattern for a mesh NoC where each row has its own
    independent traffic flow from leftmost to rightmost PE.
    
    Args:
        mesh_dim_x: Number of nodes in X dimension
        mesh_dim_y: Number of nodes in Y dimension
        total_vol: Data volume to transfer between nodes (default: 100)
        
    Returns:
        String containing the traffic table in Noxim format
    """
    # Generate the traffic table header
    traffic_table = "% OneRow Traffic Pattern for {}x{} Mesh NoC\n".format(mesh_dim_x, mesh_dim_y)
    traffic_table += "% taskID layerID [src] [dst] src_minVol src_totalVol dst_minVol dst_totalVol [waitID] [nextID] waitOP\n"
    
    task_id = 0
    
    # For each row, create a traffic pattern from left to right
    for y in range(mesh_dim_y):
        # Create tasks for this row
        for i in range(mesh_dim_x - 1):  # For each link in the row
            src = y * mesh_dim_x + i
            dst = y * mesh_dim_x + i + 1
            
            # First task in the row has no waitID dependency
            wait_id = -1 if i == 0 else task_id - 1
            
            # Last task in the row has no nextID
            next_id = -1 if i == mesh_dim_x - 2 else task_id + 1
            
            # First node in the row is a source
            src_min_vol = -1 if i == 0 else 1
            
            # Last destination node in the row is a sink
            dst_min_vol = -1 if i == mesh_dim_x - 2 else 1
            
            traffic_table += "{} 1 [{}] [{}] {} {} {} {} [{}] [{}] 0\n".format(
                task_id, src, dst, src_min_vol, total_vol, dst_min_vol, total_vol, 
                wait_id, next_id)
            
            task_id += 1
    
    return traffic_table

def generate_convolution_traffic(mesh_dim_x, mesh_dim_y, 
                               input_dim_x, input_dim_y, 
                               kernel_size=3, stride=1, 
                               in_channels=1, out_channels=1,
                               padding=0):
    """
    Generate a traffic pattern mimicking a CNN convolution layer with dynamic data volumes.
    
    Args:
        mesh_dim_x, mesh_dim_y: NoC mesh dimensions
        input_dim_x, input_dim_y: Input feature map dimensions
        kernel_size: Size of the square convolution kernel
        stride: Convolution stride
        in_channels: Number of input channels
        out_channels: Number of output channels
        padding: Input padding
        
    Returns:
        String containing the traffic table in Noxim format
    """
    # 1. Parameter validation
    if stride <= 0:
        raise ValueError("Stride must be greater than 0")
        
    if kernel_size <= 0:
        raise ValueError("Kernel size must be greater than 0")
    
    # 2. Calculate output dimensions
    output_dim_x = (input_dim_x - kernel_size + 2 * padding) // stride + 1
    output_dim_y = (input_dim_y - kernel_size + 2 * padding) // stride + 1
    
    if output_dim_x <= 0 or output_dim_y <= 0:
        raise ValueError(f"Invalid output dimensions: {output_dim_x}x{output_dim_y}. " +
                         "Try reducing kernel size, adding padding, or increasing input dimensions.")
    
    # 3. PE role assignments - improved for small meshes
    # Calculate how many PEs we need for each role
    total_pes = mesh_dim_x * mesh_dim_y
    min_input_pes = min(input_dim_x * input_dim_y, total_pes // 4)
    min_output_pes = min(output_dim_x * output_dim_y, total_pes // 4)
    min_compute_pes = min(kernel_size * kernel_size, total_pes // 2)
    
    # Ensure we have at least one PE for each role
    if total_pes < min_input_pes + min_output_pes + min_compute_pes:
        raise ValueError(f"Mesh size {mesh_dim_x}x{mesh_dim_y} too small for this convolution configuration")
    
    # a. Input region - top rows
    input_pes = []
    input_pe_rows = max(1, min_input_pes // mesh_dim_x + (1 if min_input_pes % mesh_dim_x > 0 else 0))
    for y in range(input_pe_rows):
        for x in range(mesh_dim_x):
            if len(input_pes) < min_input_pes:
                input_pes.append(y * mesh_dim_x + x)
    
    # b. Output region - bottom rows
    output_pes = []
    output_pe_rows = max(1, min_output_pes // mesh_dim_x + (1 if min_output_pes % mesh_dim_x > 0 else 0))
    for y in range(mesh_dim_y - output_pe_rows, mesh_dim_y):
        for x in range(mesh_dim_x):
            if len(output_pes) < min_output_pes:
                output_pes.append(y * mesh_dim_x + x)
    
    # c. Computation PEs - middle section
    compute_pes = []
    for y in range(input_pe_rows, mesh_dim_y - output_pe_rows):
        for x in range(mesh_dim_x):
            pe_id = y * mesh_dim_x + x
            if pe_id not in input_pes and pe_id not in output_pes and len(compute_pes) < min_compute_pes:
                compute_pes.append(pe_id)
    
    # Ensure we have at least one compute PE
    if not compute_pes:
        # If all PEs are assigned to input/output, reassign some
        if len(input_pes) > 1:
            compute_pes.append(input_pes.pop())
        elif len(output_pes) > 1:
            compute_pes.append(output_pes.pop())
        else:
            raise ValueError("Cannot allocate compute PEs - mesh too small")
    
    # 4. Generate traffic table
    traffic_table = "% Convolution Traffic Pattern for {}x{} Mesh NoC\n".format(mesh_dim_x, mesh_dim_y)
    traffic_table += "% Input: {}x{}x{}, Kernel: {}x{}, Stride: {}, Padding: {}\n".format(
        input_dim_x, input_dim_y, in_channels, kernel_size, kernel_size, stride, padding)
    traffic_table += "% PE Allocation: {} input PEs, {} compute PEs, {} output PEs\n".format(
        len(input_pes), len(compute_pes), len(output_pes))
    traffic_table += "% taskID layerID [src] [dst] src_minVol src_totalVol dst_minVol dst_totalVol [waitID] [nextID] waitOP\n"
    
    task_id = 0
    wait_id_map = {}  # To track dependencies
    
    # 5. Phase 1: Input data distribution tasks
    for out_y in range(output_dim_y):
        for out_x in range(output_dim_x):
            # For each output position, identify input window
            window_tasks = []
            output_pe_idx = (out_y * output_dim_x + out_x) % len(output_pes)
            output_pe = output_pes[output_pe_idx]
            
            # Select compute PE for this output
            compute_pe_idx = (out_y * output_dim_x + out_x) % len(compute_pes)
            compute_pe = compute_pes[compute_pe_idx]
            
            # Calculate data volume for input window - one unit per pixel
            window_data_count = 0
            
            # Process each position in the kernel
            for ky in range(kernel_size):
                for kx in range(kernel_size):
                    in_y = out_y * stride + ky - padding
                    in_x = out_x * stride + kx - padding
                    
                    # Skip padding areas
                    if 0 <= in_y < input_dim_y and 0 <= in_x < input_dim_x:
                        window_data_count += 1
                        
                        # Map input location to input PE
                        input_pe_idx = (in_y * input_dim_x + in_x) % len(input_pes)
                        input_pe = input_pes[input_pe_idx]
                        
                        # First input has no dependency, others depend on previous
                        wait_id = -1 if task_id == 0 else task_id - 1
                        next_id = task_id + 1  # Point to next task
                        
                        # First input is a source (-1 minVol)
                        src_min_vol = -1 if task_id == 0 else 1
                        
                        # Each input pixel is one data unit
                        data_vol = 1 * in_channels
                        
                        traffic_table += "{} 1 [{}] [{}] {} {} {} {} [{}] [{}] 0\n".format(
                            task_id, input_pe, compute_pe, 
                            src_min_vol, data_vol, 1, data_vol,
                            wait_id, next_id)
                        
                        window_tasks.append(task_id)
                        wait_id_map[compute_pe] = task_id  # Save last task ID for this window
                        task_id += 1
            
            # If no valid input positions (could happen with padding), skip this output
            if window_data_count == 0:
                continue
                
            # 6. Phase 2: Computation and result collection
            # Computation task waits for all input window tasks
            wait_id = wait_id_map.get(compute_pe, -1)
            
            # Set appropriate next_id
            is_last_task = (out_y == output_dim_y - 1 and out_x == output_dim_x - 1)
            next_id = -1 if is_last_task else task_id + 1
            
            # Last output is a sink (-1 minVol)
            dst_min_vol = -1 if is_last_task else 1
            
            # Computation requires all window inputs
            src_min_vol = window_data_count
            
            # Output data volume is based on feature map size
            output_data_vol = 1 * out_channels
            
            traffic_table += "{} 1 [{}] [{}] {} {} {} {} [{}] [{}] 0\n".format(
                task_id, compute_pe, output_pe,
                src_min_vol, output_data_vol, dst_min_vol, output_data_vol,
                wait_id, next_id)
            
            task_id += 1
            
    return traffic_table

def prompt_convolution_traffic():
    """
    Prompt user for all parameters required for convolution traffic generation.
    Returns the generated traffic table.
    """
    # Get base mesh parameters from command line args
    mesh_dim_x = int(sys.argv[2])
    mesh_dim_y = int(sys.argv[3])
    
    # Prompt for required convolution parameters
    print("\n--- Convolution Traffic Configuration ---")
    
    # Input Feature Map dimensions
    ifm_width = int(input("Input feature map width: "))
    ifm_height = int(input("Input feature map height: "))
    ifm_channels = int(input("Input feature map channels [1]: ") or "1")
    
    # Convolution parameters
    kernel_size = int(input("Kernel size [3]: ") or "3")
    stride = int(input("Stride [1]: ") or "1")
    padding = int(input("Padding [0]: ") or "0")
    
    # Output channels
    out_channels = int(input("Output channels [1]: ") or "1")
    
    print(f"\nGenerating convolution traffic for {mesh_dim_x}x{mesh_dim_y} mesh...")
    print(f"IFM: {ifm_width}x{ifm_height}x{ifm_channels}, Kernel: {kernel_size}x{kernel_size}, Stride: {stride}, Padding: {padding}")
    
    return generate_convolution_traffic(
        mesh_dim_x, mesh_dim_y,
        ifm_width, ifm_height,
        kernel_size, stride,
        ifm_channels, out_channels,
        padding
    )

def save_to_file(content, filename):
    """Save content to a file"""
    with open(filename, 'w') as f:
        f.write(content)
    print(f"Traffic pattern written to {filename}")

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 3:
        print("Usage: python traffic_gen.py [traffic_type] <mesh_dim_x> <mesh_dim_y> [data_volume] [output_file]")
        print("Example: python traffic_gen.py zigzag 4 4 100 zigzag_4x4.txt")
        print("Supported traffic types: zigzag, onerow, conv")
        sys.exit(1)
    
    # Check for traffic_type parameter
    if sys.argv[1] in ["zigzag", "onerow", "conv"]:
        traffic_type = sys.argv[1]
        arg_offset = 1  # Adjust argument positions
    else:
        traffic_type = "zigzag"  # Default
        arg_offset = 0
    
    # Get mesh dimensions
    mesh_dim_x = int(sys.argv[1 + arg_offset])
    mesh_dim_y = int(sys.argv[2 + arg_offset])
    
    # Get optional parameters
    total_vol = 100  # Default data volume
    if len(sys.argv) > 3 + arg_offset:
        total_vol = int(sys.argv[3 + arg_offset])
    
    output_file = f"{traffic_type}_{mesh_dim_x}x{mesh_dim_y}.txt"
    if len(sys.argv) > 4 + arg_offset:
        output_file = sys.argv[4 + arg_offset]
    
    # Generate traffic based on selected type
    if traffic_type == "zigzag":
        traffic_table = generate_zigzag_traffic(mesh_dim_x, mesh_dim_y, total_vol)
    elif traffic_type == "onerow":
        traffic_table = generate_onerow_traffic(mesh_dim_x, mesh_dim_y, total_vol)
    elif traffic_type == "conv":
        traffic_table = prompt_convolution_traffic()

    
    save_to_file(traffic_table, output_file)
    
   # Print validation info
    print("\nValidating traffic table...")
    lines = traffic_table.strip().split('\n')
    data_lines = [line for line in lines if not line.startswith('%')]
    
    if traffic_type == "onerow":
        print(f"Generated {len(data_lines)} tasks for {mesh_dim_y} rows")
        for y in range(mesh_dim_y):
            row_start = y * (mesh_dim_x - 1)
            row_end = row_start + mesh_dim_x - 2
            
            if row_end < len(data_lines):
                start_line = data_lines[row_start]
                end_line = data_lines[row_end]
                print(f"Row {y} first task: {start_line}")
                print(f"Row {y} last task: {end_line}")
                
                # Correct parsing of waitID and nextID values
                start_fields = start_line.split()
                end_fields = end_line.split()
                
                # waitID is the 9th field (index 8), nextID is the 10th field (index 9)
                start_waitID = start_fields[8] 
                end_nextID = end_fields[9]
                
                print(f"✓ Row {y}: First task waitID = [-1], Last task nextID = [-1]" 
                      if start_waitID == "[-1]" and end_nextID == "[-1]" 
                      else f"✗ Row {y}: Check waitID/nextID settings")
    else:  # zigzag
        last_line = data_lines[-1]
        print(f"Last task: {last_line}")
        
        # Parse the last line to check nextID value (10th field, index 9)
        last_fields = last_line.split()
        has_next_id_minus_one = last_fields[9] == "[-1]"
        
        if has_next_id_minus_one:
            print("✓ Last task correctly has nextID = [-1]")
        else:
            print("✗ Last task does NOT have nextID = [-1]")