# Noxim - Network-on-Chip Simulator (fastsim Branch)

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)

## Table of Contents
- [Introduction](#introduction)
- [Key Features](#key-features)
- [System Requirements](#system-requirements)
- [Environment Setup](#environment-setup)
- [Quick Start Guide](#quick-start-guide)
- [Detailed User Guide](#detailed-user-guide)
  - [Configuration Files](#configuration-files)
  - [Traffic Pattern Design](#traffic-pattern-design)
  - [Command-Line Options](#command-line-options)
  - [Performance Tuning](#performance-tuning)
  - [Understanding Output Statistics](#understanding-output-statistics)
- [Advanced Usage](#advanced-usage)
  - [Traffic Generation Tools](#traffic-generation-tools)
  - [Parameter Space Exploration](#parameter-space-exploration)
  - [Regression Testing](#regression-testing)
- [Citation](#citation)
- [Support](#support)

---

## Introduction

**Noxim** is a cycle-accurate Network-on-Chip (NoC) simulator developed at the University of Catania. The **fastsim branch** is an optimized version specifically designed for simulating complex data-parallel workloads such as neural network inference, machine learning accelerators, and high-performance computing applications.

### Purpose

This simulator enables researchers and engineers to:

- **Evaluate NoC architectures** for various topologies (Mesh, Butterfly, Baseline, Omega)
- **Analyze communication patterns** with complex task dependencies and data flows
- **Optimize NoC parameters** including buffer sizes, routing algorithms, and virtual channels
- **Measure performance metrics** such as latency, throughput, energy consumption, and network utilization
- **Simulate realistic workloads** with byte-based traffic, compute delays, and multi-stage pipelines
- **Design accelerator architectures** for AI/ML workloads with one-to-many, many-to-one, and many-to-many communication patterns

### What Makes fastsim Special?

The fastsim branch includes several enhancements over the standard Noxim:

- **Traffic Communication Tables**: Define complex PE-to-PE communication with task dependencies
- **Byte-based Traffic Mode**: Specify traffic in bytes rather than flits for realistic workload modeling
- **Compute Delay Modeling**: Simulate processing delays at each PE
- **Multicast Support**: Efficient one-to-many data distribution
- **Performance Optimizations**: Significantly faster simulation for large-scale designs
- **Advanced Statistics**: Detailed breakdowns of stalls, throughput, timing, and energy
- **CSV Export**: Easy integration with analysis tools and scripts

---

## Key Features

### Network Topologies
- **Mesh** (2D grid with XY routing support)
- **Butterfly** (Delta network topology)
- **Baseline** (Delta network topology)
- **Omega** (Delta network topology)

### Routing Algorithms
- **XY**: Dimension-ordered routing
- **WEST_FIRST**: Turn model routing
- **NORTH_LAST**: Turn model routing
- **NEGATIVE_FIRST**: Turn model routing
- **ODD_EVEN**: Deadlock-free adaptive routing
- **DYAD**: Adaptive routing with threshold
- **TABLE_BASED**: Custom routing tables
- **DELTA**: For delta network topologies

### Selection Strategies
- **RANDOM**: Random output port selection
- **BUFFER_LEVEL**: Select port with most free buffer space
- **NOP**: No output port selection (used in some configurations)

### Traffic Patterns
- **TRAFFIC_RANDOM**: Random uniform distribution
- **TRAFFIC_TRANSPOSE1/2**: Matrix transpose patterns
- **TRAFFIC_HOTSPOT**: Hotspot-based traffic
- **TRAFFIC_BIT_REVERSAL**: Bit-reversal permutation
- **TRAFFIC_SHUFFLE**: Shuffle permutation
- **TRAFFIC_BUTTERFLY**: Butterfly permutation
- **TRAFFIC_COMMUNICATION_TABLE**: Custom task-based traffic (recommended for complex workloads)

---

## System Requirements

### Hardware Requirements
- **Processor**: x86_64 architecture (optimized with `-march=native`)
- **Memory**: Minimum 4GB RAM (8GB+ recommended for large simulations)
- **Disk Space**: ~2GB for source code, libraries, and build artifacts

### Software Requirements
- **Operating System**: Linux (Ubuntu 18.04+, Debian, CentOS) or macOS
- **Compiler**: g++ 5.0+ with C++11 support
- **Build Tools**: make, pkg-config
- **Libraries**:
  - SystemC 2.3.1 (required for simulation engine)
  - yaml-cpp (required for YAML configuration parsing)
- **Python**: Python 3.6+ (for traffic generation and exploration tools)
  - numpy
  - pandas (optional, for xplore.py)

---

## Environment Setup

### Method 1: Automated Setup (Ubuntu)

For Ubuntu-based systems, use the official setup script:

```bash
bash <(wget -qO- --no-check-certificate https://raw.githubusercontent.com/davidepatti/noxim/master/other/setup/ubuntu.sh)
```

### Method 2: Automated Setup (macOS)

For macOS systems:

```bash
/bin/zsh -c "$(curl -fsSL https://raw.githubusercontent.com/davidepatti/noxim/master/other/setup/macos.zsh)"
```

### Method 3: Manual Setup

#### Step 1: Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential git wget tar
sudo apt-get install python3 python3-pip
pip3 install numpy pandas
```

**macOS:**
```bash
brew install gcc make wget cmake
brew install python3
pip3 install numpy pandas
```

#### Step 2: Install SystemC 2.3.1

```bash
cd bin
mkdir -p libs
cd libs

# Download SystemC
wget https://www.accellera.org/images/downloads/standards/systemc/systemc-2.3.1a.tar.gz
tar -xzf systemc-2.3.1a.tar.gz
cd systemc-2.3.1a

# Configure and build
mkdir objdir
cd objdir
../configure --prefix=$(pwd)/../../systemc-2.3.1
make
make install
cd ../../

# Verify installation
ls -la systemc-2.3.1/lib-linux64/  # or lib-macosx64/ on macOS
```

#### Step 3: Install yaml-cpp

```bash
cd libs  # Should be in bin/libs/

# Download yaml-cpp
git clone https://github.com/jbeder/yaml-cpp.git
cd yaml-cpp
mkdir build
cd build

# Build and install
cmake -DCMAKE_INSTALL_PREFIX=../../yaml-cpp ..
make
make install
cd ../../

# Verify installation
ls -la yaml-cpp/lib/
ls -la yaml-cpp/include/
```

#### Step 4: Build Noxim

```bash
cd ..  # Back to bin/ directory
make clean
make
```

If successful, you should see the `noxim` executable in the `bin/` directory.

#### Step 5: Verify Installation

```bash
./noxim -config ../config_examples/default_config.yaml
```

You should see simulation output with statistics at the end.

### Troubleshooting

**Error: "fatal error: systemc.h: No such file or directory"**
- Ensure SystemC is installed in `bin/libs/systemc-2.3.1/`
- Check the Makefile paths match your installation

**Error: "cannot find -lyaml-cpp"**
- Ensure yaml-cpp is installed in `bin/libs/yaml-cpp/`
- Verify the library files exist in `yaml-cpp/lib/`

**Compilation warnings about deprecated features**
- These are generally safe to ignore; the code compiles with `-Wno-deprecated`

---

## Quick Start Guide

### 1. Build the Simulator

```bash
cd bin
make clean && make
```

### 2. Run Your First Simulation

```bash
./noxim -config ../config_examples/default_config.yaml
```

This runs a basic 4×4 mesh network simulation with random traffic.

### 3. Run with Custom Traffic

```bash
./noxim -config ../config_examples/0_llm.yaml
```

This simulates a 10×40 mesh with a complex traffic pattern defined in `o2m.txt`.

### 4. View Results

The simulation output includes:
- Total received packets and flits
- Average and max delay (cycles)
- Network throughput (flits/cycle, GB/s, Gbps)
- Energy consumption breakdown
- Stall statistics

Example output:
```
% Total received packets: 256
% Total received flits: 2048
% Global average delay (cycles): 87.45
% Network throughput (flits/cycle): 0.2048
%   Network throughput (GB/s): 6.553
%   Network throughput (Gbps): 52.428
% Total energy (J): 1.234e-03
```

### 5. Explore Traffic Patterns

Examine the example traffic files:
```bash
less bin/o2m.txt
less bin/new_traffic.txt
```

---

## Detailed User Guide

### Configuration Files

Noxim uses YAML configuration files to define simulation parameters. Configuration files are located in `config_examples/`.

#### Basic Structure

```yaml
# Network Topology
topology: MESH
mesh_dim_x: 8
mesh_dim_y: 8

# Router Configuration
buffer_depth: 8
n_virtual_channels: 2
routing_algorithm: XY
selection_strategy: BUFFER_LEVEL

# Link Configuration
r2r_link_length: 1.0  # Router-to-router link length in mm
r2h_link_length: 2.0  # Router-to-hub link length in mm

# Flit and Packet Configuration
flit_size: 128           # Flit size in bits
min_packet_size: 4       # Minimum packet size in flits
max_packet_size: 8       # Maximum packet size in flits

# PE Configuration
compute_delay: 5         # Compute cycles at each PE

# Simulation Parameters
clock_period_ps: 1000    # Clock period in picoseconds (1ns)
simulation_time: 100000  # Total simulation cycles
reset_time: 1000         # Reset duration in cycles
stats_warm_up_time: 10000  # Cycles before collecting stats

# Traffic Configuration
traffic_distribution: TRAFFIC_COMMUNICATION_TABLE
traffic_table_filename: "my_traffic.txt"
traffic_in_bytes: true   # Traffic specified in bytes

packet_injection_rate: 0.01  # For random traffic
probability_of_retransmission: 0.01

# Output Configuration
verbose_mode: VERBOSE_MEDIUM  # OFF, LOW, MEDIUM, HIGH
detailed: true                # Detailed per-task statistics
show_buffer_stats: true       # Buffer utilization stats
trace_mode: false             # VCD trace generation
trace_filename: ""

# Wireless Configuration (if applicable)
use_winoc: false
use_wirxsleep: false
```

#### Key Parameters Explained

**topology**
- Defines the network topology
- Options: `MESH`, `BUTTERFLY`, `BASELINE`, `OMEGA`
- Mesh is most common for 2D NoC designs

**mesh_dim_x, mesh_dim_y**
- Dimensions of the mesh network
- Total PEs = mesh_dim_x × mesh_dim_y
- Example: 8×8 = 64 PEs

**buffer_depth**
- Number of flits each router buffer can hold
- Larger buffers reduce congestion but increase area/power
- Typical values: 4-16 flits

**n_virtual_channels**
- Number of virtual channels per physical channel
- Increases throughput and can prevent deadlock
- Typical values: 1-4 VCs

**flit_size**
- Size of each flit in bits
- Common values: 32, 64, 128, 256, 512 bits
- Larger flits reduce packet overhead but increase wire area

**compute_delay**
- Processing delay at each PE in clock cycles
- Models computation time before data can be forwarded
- Set to 0 for pure communication simulation

**traffic_in_bytes**
- When `true`, traffic table values are interpreted as bytes
- Simulator automatically converts bytes to flits
- Essential for realistic workload modeling

**simulation_time**
- Total simulation duration in clock cycles
- Should be long enough for all traffic to complete
- Use `-max_volume` option to stop when traffic completes

**stats_warm_up_time**
- Cycles to run before collecting statistics
- Allows network to reach steady state
- Set to 0 for deterministic traffic patterns

### Traffic Pattern Design

The **Traffic Communication Table** is the most powerful feature of the fastsim branch, enabling complex task-based communication patterns.

#### Traffic Table Format

```
taskID layerID [src] [dst] src_minVol src_totalVol dst_minVol dst_totalVol [waitID] [nextID] waitOP [traffic_type]
```

#### Field Descriptions

| Field | Type | Description |
|-------|------|-------------|
| `taskID` | int | Unique identifier for this task |
| `layerID` | int | Layer number (for organizing tasks, e.g., NN layers) |
| `[src]` | int[] | Source PE ID(s). Can be multiple: `[0 4 8]` |
| `[dst]` | int[] | Destination PE ID(s). Can be multiple: `[1 5 9]` |
| `src_minVol` | int | Data units needed to generate one flit at source. `-1` = pure source (no input) |
| `src_totalVol` | int | Total data to transmit from source (in bytes if `traffic_in_bytes: true`) |
| `dst_minVol` | int | Data units needed at dst to start compute. `-1` = pure sink (no compute) |
| `dst_totalVol` | int | Total data to receive at destination |
| `[waitID]` | int[] | Task IDs this task must wait for. `[-1]` = no wait (start at time 0) |
| `[nextID]` | int[] | Task IDs that depend on this task. `[-1]` = no dependents |
| `waitOP` | int | Wait operation: `0`=NOP (no wait), `1`=TRN (wait for transmission), `2`=CMP (wait for compute) |
| `traffic_type` | char | Optional: `u`=unicast (default), `m`=multicast, `b`=broadcast |

#### Understanding Data Volumes

**Source Volumes:**
- `src_minVol = -1`: This PE is a **pure source** (e.g., input layer)
  - Data is available immediately
  - No incoming flits needed
- `src_minVol = N`: This PE must receive **N data units** before sending one flit
  - Acts as a **compression** or **reduction** operation
  - Example: `src_minVol = 4` → accumulate 4 units, send 1 flit

**Destination Volumes:**
- `dst_minVol = -1`: This PE is a **pure sink** (e.g., output layer)
  - Data is consumed, no further processing
- `dst_minVol = N`: This PE must receive **N data units** before starting compute
  - Models **minimum data requirement** for computation
  - Example: Convolution needs full window before computing

#### Traffic Pattern Examples

**Example 1: Simple Pipeline (PE0 → PE1 → PE2)**

```
% Three-stage pipeline
0 1 [0] [1] -1 100 10 100 [-1] [1] 0
1 1 [1] [2] 10 50 5 50 [0] [2] 0
2 1 [2] [3] 5 25 -1 25 [1] [-1] 0
```

Explanation:
- Task 0: PE0 sends 100 bytes to PE1 (source)
  - PE1 needs 10 bytes to start compute
- Task 1: PE1 processes and sends 50 bytes to PE2 (2:1 reduction)
  - PE2 needs 5 bytes to start compute
- Task 2: PE2 processes and outputs 25 bytes (sink)

**Example 2: One-to-Many (Broadcast)**

```
% PE0 broadcasts to PE1, PE2, PE3
0 1 [0] [1 2 3] -1 128 16 128 [-1] [1 2 3] 0 m
1 1 [1] [5] 16 64 -1 64 [0] [-1] 0
2 1 [2] [6] 16 64 -1 64 [0] [-1] 0
3 1 [3] [7] 16 64 -1 64 [0] [-1] 0
```

Explanation:
- Task 0: PE0 multicasts 128 bytes to PE1, PE2, PE3
- Tasks 1-3: Each destination processes independently
- Traffic type `m` enables efficient multicast routing

**Example 3: Many-to-One (Aggregation)**

```
% PE0, PE1, PE2 aggregate to PE3
0 1 [0] [3] -1 64 16 64 [-1] [3] 0
1 1 [1] [3] -1 64 16 64 [-1] [3] 0
2 1 [2] [3] -1 64 16 64 [-1] [3] 0
3 1 [3] [4] 16 96 -1 96 [0 1 2] [-1] 0
```

Explanation:
- Tasks 0-2: Three sources send 64 bytes each to PE3
- Task 3: PE3 waits for all three (waitID `[0 1 2]`)
  - Needs 16 bytes minimum before processing
  - Receives total 192 bytes (3 × 64)
  - Sends 96 bytes to PE4 (2:1 reduction)

**Example 4: Many-to-Many**

```
% PE0, PE1 send to PE2, PE3
0 1 [0 1] [2 3] -1 100 50 100 [-1] [1] 0
1 1 [2 3] [4 5] 50 50 25 50 [0] [2 3] 0
2 1 [4] [6] 25 25 -1 25 [1] [-1] 0
3 1 [5] [7] 25 25 -1 25 [1] [-1] 0
```

Explanation:
- Task 0: PE0 and PE1 each send 100 bytes to both PE2 and PE3
  - Creates 4 communication pairs: (0→2, 0→3, 1→2, 1→3)
- Task 1: PE2 and PE3 process and send to PE4 and PE5
- Tasks 2-3: Final sinks

**Example 5: Compute Delay and Dependencies**

```
% Pipeline with compute wait
0 1 [0] [1] -1 256 1 256 [-1] [1] 0
1 1 [1] [2] 1 128 1 128 [0] [2] 1
2 1 [2] [3] 1 64 -1 64 [1] [-1] 2
```

Explanation:
- Task 1 with `waitOP = 1`: Wait for task 0 **transmission** to complete
- Task 2 with `waitOP = 2`: Wait for task 1 **compute** to complete
  - More realistic dependency modeling

#### Traffic Table Validation Rules

1. **Source/Destination Constraints:**
   - One-to-many: `src.size() == 1 && dst.size() > 1`
   - Many-to-one: `src.size() > 1 && dst.size() == 1`
   - Many-to-many: `src.size() > 1 && dst.size() > 1`

2. **WaitID Rules:**
   - `[-1]` means no dependencies (start at time 0)
   - If waitID contains -1, it must be the only element
   - All waitID values must be ≥ -1

3. **Traffic Type Rules:**
   - Multicast (`m`) requires: `src.size() == 1 && dst.size() > 1`
   - Unicast (`u`) is default

4. **Volume Consistency:**
   - `src_totalVol` should match expected data generation
   - `dst_totalVol` should match expected data reception
   - For many-to-one: `dst_totalVol` should account for all sources

Use the validation tool to check your traffic:
```bash
./validate_traffic.py my_traffic.txt
```

### Command-Line Options

Override YAML configuration with command-line arguments:

```bash
./noxim -config <file.yaml> [options]
```

#### Common Options

| Option | Description | Example |
|--------|-------------|---------|
| `-config` | YAML configuration file | `-config config.yaml` |
| `-sim` | Simulation cycles | `-sim 100000` |
| `-mesh` | Mesh dimensions | `-mesh 8 8` |
| `-buffer` | Buffer depth | `-buffer 8` |
| `-flit_size` | Flit size in bits | `-flit_size 256` |
| `-routing` | Routing algorithm | `-routing XY` |
| `-sel` | Selection strategy | `-sel BUFFER_LEVEL` |
| `-vc` | Virtual channels | `-vc 2` |
| `-traffic` | Traffic distribution | `-traffic TRAFFIC_RANDOM` |
| `-pir` | Packet injection rate | `-pir 0.02` |
| `-verbose` | Verbosity level | `-verbose VERBOSE_MEDIUM` |
| `-detailed` | Detailed stats | `-detailed true` |
| `-traffic_table` | Traffic table file | `-traffic_table traffic.txt` |
| `-clkps` | Clock period (ps) | `-clkps 1000` |

#### Advanced Options

| Option | Description | Example |
|--------|-------------|---------|
| `-traffic_in_bytes` | Enable byte mode | `-traffic_in_bytes true` |
| `-compute_delay` | PE compute cycles | `-compute_delay 10` |
| `-show_buffer_stats` | Buffer stats | `-show_buffer_stats true` |
| `-max_volume` | Stop after volume | `-max_volume 10000` |
| `-warm_up` | Warmup cycles | `-warm_up 1000` |
| `-trace` | Enable VCD trace | `-trace true` |
| `-trace_file` | Trace filename | `-trace_file sim.vcd` |

#### Example Usage

**Run with modified parameters:**
```bash
./noxim -config ../config_examples/0_llm.yaml \
        -sim 200000 \
        -buffer 16 \
        -vc 4 \
        -routing ODD_EVEN \
        -verbose VERBOSE_HIGH
```

**Quick parameter sweep:**
```bash
for buf in 4 8 16; do
    ./noxim -config config.yaml -buffer $buf > results_buf${buf}.txt
done
```

### Performance Tuning

#### Optimizing Simulation Speed

1. **Disable Debug Output**
   - Edit `bin/Makefile`, ensure DEBUG line is commented:
   ```makefile
   # DEBUG := -g -DDEBUG -DCDBG
   ```
   - Rebuild: `make clean && make`
   - This can speed up simulation by 10-50×

2. **Reduce Verbosity**
   ```yaml
   verbose_mode: VERBOSE_OFF
   detailed: false
   ```

3. **Use Appropriate Simulation Time**
   - Calculate based on traffic: `sim_time = traffic_volume / expected_throughput * 1.5`
   - Or use: `max_volume_to_be_drained: <total_flits>`

4. **Compiler Optimizations**
   - Already enabled: `-O3 -march=native -funroll-loops`
   - For aggressive optimization: add `-Ofast` in Makefile

#### Optimizing NoC Performance

1. **Buffer Sizing**
   - Small buffers (4-8): Low area/power, higher latency under contention
   - Large buffers (16-32): Better throughput, higher area/power
   - Rule of thumb: `buffer_depth ≥ average_packet_size × 1.5`

2. **Virtual Channels**
   - More VCs improve throughput and reduce head-of-line blocking
   - Diminishing returns beyond 4 VCs
   - Cost: Increased router complexity

3. **Routing Algorithm Selection**
   - **XY**: Simplest, deadlock-free for mesh, but not adaptive
   - **ODD_EVEN**: Adaptive, better load balancing, slightly more complex
   - **WEST_FIRST/NORTH_LAST**: Good balance of adaptivity and simplicity
   - **TABLE_BASED**: Custom routing, most flexible

4. **Selection Strategy**
   - **RANDOM**: Simple, works well for low-to-medium traffic
   - **BUFFER_LEVEL**: Better for high traffic, adapts to congestion
   - **NOP**: Minimal logic, use when routing is deterministic

5. **Flit Size vs Packet Size**
   - Larger flits reduce serialization delay and router hops
   - But increase channel width and power
   - Balance: 64-256 bits for most applications

#### Traffic Pattern Optimization

1. **Minimize Hotspots**
   - Use network visualization to identify congested links
   - Spread traffic spatially if possible
   - Consider multicast for one-to-many patterns

2. **Pipeline Traffic Carefully**
   - Use `waitID` and `nextID` to enforce correct ordering
   - Set appropriate `dst_minVol` to enable pipelining
   - Monitor PE utilization

3. **Byte-based Traffic**
   - Always use `traffic_in_bytes: true` for realistic workloads
   - Ensures proper flit-to-byte conversion
   - Critical for accurate energy/latency estimates

### Understanding Output Statistics

#### Global Statistics

```
% Total received packets: 1024
% Total received flits: 8192
% Received/Ideal flits Ratio: 0.950
```
- **Received packets/flits**: Successfully delivered data
- **Ratio**: Actual vs ideal (1.0 = perfect, <1.0 = some data not delivered)

#### Latency Metrics

```
% Global average delay (cycles): 125.34
% Max delay (cycles): 456
```
- **Average delay**: Mean packet latency from injection to reception
- **Max delay**: Worst-case latency (important for real-time systems)

#### Throughput Metrics

```
% Network throughput (flits/cycle): 0.0819
%   Network throughput (Gbps): 20.99
%   Network throughput (GB/s): 2.624
% Average IP throughput (flits/cycle/IP): 0.00204
%   Average IP throughput (Gbps): 0.525
%   Average IP throughput (GB/s): 0.0656
```
- **Network throughput**: Aggregate network bandwidth utilization
- **IP throughput**: Per-PE average bandwidth
- Multiple units for convenience (flits/cycle most fundamental)

#### Energy Metrics

```
% Total energy (J): 2.345e-03
%   Dynamic energy (J): 1.890e-03
%   Static energy (J): 4.550e-04
```
- **Total energy**: Sum of dynamic and static
- **Dynamic**: Energy from switching activity (routers, links)
- **Static**: Leakage energy (depends on simulation time)

#### Stall Statistics

```
% Total stalls: 45678
% PE→Router stalls: 12345 (27.0%)
% Router→Router stalls: 28901 (63.3%)
% Buffer full stalls: 23456 (51.4%)
% VC busy stalls: 15432 (33.8%)
```
- **Stalls**: Cycles where flits cannot advance
- **PE→Router**: Backpressure from network to PEs
- **Router→Router**: Network congestion
- **Buffer full**: Need larger buffers or better routing
- **VC busy**: Need more VCs or better VC allocation

#### Per-Task Timing (with detailed: true)

```
Traffic Completion Statistics:
TaskID  Layer  Src→Dst        WaitTime  CompTime  XmitTime  RecvTime  TotalTime
0       1      [0]→[10]       0         50        1200      1250      1250
1       1      [10]→[20]      1250      100       2400      2650      3900
```
- **WaitTime**: Cycles waiting for dependencies
- **CompTime**: Cycles in compute (based on `compute_delay`)
- **XmitTime**: Cycles transmitting data
- **RecvTime**: Cycles receiving data
- **TotalTime**: End-to-end task completion time

---

## Advanced Usage

### Traffic Generation Tools

#### Using traffic_gen.py

Generate common patterns programmatically:

```python
from traffic_gen import (
    generate_zigzag_traffic,
    generate_onerow_traffic,
    generate_convolution_traffic
)

# Generate zig-zag pattern for 10×40 mesh
traffic = generate_zigzag_traffic(mesh_dim_x=10, mesh_dim_y=40, total_vol=256)
print(traffic)

# Save to file
with open('zigzag_traffic.txt', 'w') as f:
    f.write(traffic)
```

**Available Generators:**
- `generate_zigzag_traffic(x, y, vol)`: Serpentine pattern across mesh
- `generate_onerow_traffic(x, y, vol)`: Parallel row-wise data flows
- `generate_convolution_traffic(...)`: Convolution layer communication

#### Validating Traffic Tables

```bash
./validate_traffic.py my_traffic.txt
```

Output:
- Line-by-line validation
- Rule violations highlighted
- Generates `.fixed` file with comments on errors

### Parameter Space Exploration

The `xplore.py` tool automates running multiple simulations with different parameters.

#### Basic Usage

```bash
./xplore.py --config ../config_examples/0_llm.yaml
```

#### Custom Parameter Sweep

Create a JSON config file `exploration_config.json`:

```json
{
    "noxim_path": "./noxim",
    "config_yaml": "../config_examples/0_llm.yaml",
    "parameters": {
        "vc": [1, 2, 4],
        "buffer": [4, 8, 16],
        "flit": [128, 256],
        "routing": ["XY", "ODD_EVEN"],
        "sel": ["RANDOM", "BUFFER_LEVEL"],
        "sim": [100000]
    }
}
```

Run exploration:
```bash
./xplore.py --config exploration_config.json --parallel 4
```

This runs all combinations (3×3×2×2×2×1 = 72 simulations) in parallel.

#### Analyzing Results

Results are saved in timestamped directories:
```
results/run_20250403_143022/
├── logs/                 # Raw simulation outputs
├── extracted/            # Parsed metrics (CSV)
├── analysis/             # Generated plots
└── summary/              # Summary statistics
```

Load results in Python:
```python
import pandas as pd

# Load extracted metrics
df = pd.read_csv('results/run_20250403_143022/extracted/all_results.csv')

# Analyze
best_config = df.loc[df['avg_delay'].idxmin()]
print(f"Best configuration: {best_config}")
```

### Regression Testing

Run test suites to verify simulator behavior:

```bash
# List available tests
./regression.py --list

# Run specific test
./regression.py --test simple_pipeline

# Run all tests
./regression.py --auto
```

Create custom tests in `traffic_test_cases.txt`:

```
### BEGIN_TEST ###
name: my_test
description: Custom test case
mesh: 4 4
vc: 2
buffer: 8
sim: 10000
traffic: |
    0 1 [0] [5] -1 64 -1 64 [-1] [-1] 0
    1 1 [8] [13] -1 64 -1 64 [-1] [-1] 0
expected:
    packets: 2
    avg_delay: < 100
### END_TEST ###
```

---

## Citation

If you use Noxim in your research, please cite:

```bibtex
@article{noxim2016,
  author = {Catania, Vincenzo and Mineo, Andrea and Monteleone, Salvatore and Palesi, Maurizio and Patti, Davide},
  title = {Cycle-Accurate Network on Chip Simulation with Noxim},
  journal = {ACM Transactions on Modeling and Computer Simulation},
  volume = {27},
  number = {1},
  year = {2016},
  pages = {4:1--4:25},
  doi = {10.1145/2953878}
}
```

---

## Support

### Documentation
- **Main Repository**: [https://github.com/davidepatti/noxim](https://github.com/davidepatti/noxim)
- **Tutorial Slides**: `doc/noxim_tutorial.pdf` (if available)

### Getting Help
- **GitHub Issues**: Report bugs or request features
- **Mailing List**: Register at the [Noxim Registration Form](https://docs.google.com/forms/d/e/1FAIpQLSfJnYQZwxC4gr4jUc-nuwuGp0MDBA-0N_TVf8hqV1DIa325Dg/viewform)

### Common Issues

**Q: Simulation is very slow**
- A: Disable debug mode in Makefile and rebuild
- A: Reduce verbosity: `verbose_mode: VERBOSE_OFF`
- A: Use appropriate simulation_time

**Q: Traffic doesn't complete**
- A: Increase simulation_time
- A: Check traffic table for cycles in dependencies
- A: Validate traffic with `validate_traffic.py`

**Q: High energy consumption**
- A: Normal for large simulations; check power.yaml for parameters
- A: Reduce buffer sizes, use lower flit sizes
- A: Optimize routing to reduce hop count

**Q: Stalls are very high**
- A: Increase buffer depth
- A: Add more virtual channels
- A: Use adaptive routing (ODD_EVEN)
- A: Reduce traffic injection rate

---

## License

Noxim is released under the GNU General Public License v2.0. See LICENSE file for details.

---

**Happy Simulating!** 🚀

For questions or contributions, please open an issue on GitHub or contact the development team.
