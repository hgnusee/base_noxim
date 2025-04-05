# Converting Trace Files to Noxim Traffic Table Format

## Overview

This document explains how application trace files are converted to the Noxim traffic table format used by the NoC simulator. Understanding this conversion is essential for users who want to simulate realistic traffic patterns derived from application traces.

## Format Comparison

### Original Trace File Format
```
TaskID  SrcPE  DstPE  Size  Dependencies
```

### Noxim Traffic Table Format
```
taskID layerID [src] [dst] src_minVol src_totalVol dst_minVol dst_totalVol [waitID] [nextID] waitOP traffic_type
```

## Key Conversion Rules

### 1. Basic Field Mapping

- **TaskID**: Directly preserved from trace to traffic table
- **SrcPE/DstPE**: Converted to PE node IDs (divided by 10 in this example)
- **Size**: Maps to volume parameters (src_totalVol and dst_totalVol)
- **Dependencies**: Maps to waitID field in traffic table

### 2. Special Traffic Types

#### Self-Compute Tasks

When source and destination PEs are the same in the trace file, this indicates a self-compute task. The Noxim traffic table maintains the same value in both `[src]` and `[dst]` arrays.

Example from trace file:
```
9 400 400 32 7, 8
```

Converted to traffic table:
```
9 1 [40] [40] -1 32 -1 32 [7 8] [-1] 0
```

In the ProcessingElement implementation, these tasks are identified with the `is_self_compute` flag:

```cpp
// Check for special traffic types
if (src.size() == 1 && dst.size() == 1 && src[0] == dst[0]) {
    TrafficCommunication.is_self_compute = true;
} else {
    TrafficCommunication.is_self_compute = false;
}
```

Self-compute tasks are handled differently - they don't actually transmit packets across the NoC but still require computation within the processing element.

#### T_WAIT Traffic Type

Tasks marked with `w` at the end of the traffic table entry are of type T_WAIT. These tasks wait for reception completion of their dependencies rather than just transmission completion.

Example from traffic table:
```
7 1 [10] [40] -1 48 48 48 [2] [-1] 0 w
```

The `w` indicator causes the task to use reception dependencies checking:

```cpp
if (comm.is_wait_reception) {
    // Check if all waitIDs have their reception completed
    if (!traffic_communication_table->checkReceptionDependencies(comm.waitID)) {
        LOG << "PE " << local_id << " waiting for reception completion of dependencies" << endl;
        return false;  // Dependencies not satisfied
    }
}
```

### 3. Volume Parameters

Four volume parameters control data transmission behavior:

- **src_minVol**: Minimum volume that must be transmitted in one burst
- **src_totalVol**: Total volume to be transmitted from source
- **dst_minVol**: Minimum volume that must be received in one burst
- **dst_totalVol**: Total volume expected by the destination

For regular transmissions, these are typically set to the size value from the trace file. When `-1` is used for min volumes, it indicates special handling:

```
9 1 [40] [40] -1 32 -1 32 [7 8] [-1] 0
```

The `-1` value for src_minVol and dst_minVol indicates this is a reception-dependent task:

```cpp
if (src_minVol == -1 && dst_minVol == -1) {
    TrafficCommunication.is_wait_reception = true;
} else {
    TrafficCommunication.is_wait_reception = false;
}
```

### 4. Dependency Handling

Dependencies in the trace file are converted to waitIDs in the traffic table:

- **Single dependency**: `[dependency_id]`
- **Multiple dependencies**: `[dependency_id1 dependency_id2 ...]`
- **No dependencies**: `[-1]`

Example:
```
// Trace file (multiple dependencies)
9 400 400 32 7, 8

// Traffic table
9 1 [40] [40] -1 32 -1 32 [7 8] [-1] 0
```

The simulator checks whether these dependencies are satisfied before allowing packet transmission:

```cpp
// Regular waitID semantics (check for transmission completion)
auto it_find_waitID = find(comm.waitID.begin(), comm.waitID.end(), currentTaskID);
found_waitID = (it_find_waitID != comm.waitID.end());

if (!found_waitID) {
    return false;  // Original dependency check fails
}
```

### 5. NextID Field

The `nextID` field indicates successor tasks. In this example, all tasks have `[-1]` as nextID, indicating they're terminal tasks with no successors in the execution chain.

## Complete Example Analysis

Let's analyze a full example to illustrate the conversion:

**Original trace entry**:
```
7 100 400 48 2
```

**Converted traffic table entry**:
```
7 1 [10] [40] -1 48 48 48 [2] [-1] 0 w
```

This represents:
- Task 7 with layerID 1
- Source PE at node 10 (converted from 100)
- Destination PE at node 40 (converted from 400)
- src_minVol is -1 (indicating it's waiting for reception)
- Total volume to transmit is 48 units
- dst_minVol and dst_totalVol are both set to 48
- Task depends on task 2's completion (waitID = [2])
- No successor tasks (nextID = [-1])
- waitOP = 0 (indicating synchronization mechanism)
- Type is T_WAIT (indicated by trailing 'w'), meaning this task waits for reception completion of dependencies

## Conclusion

The conversion from trace file to Noxim traffic table expands the simple trace format into a more detailed specification that controls how tasks are executed, dependencies are managed, and data is transmitted across the NoC. The additional parameters in the traffic table provide fine-grained control over communication behavior, enabling realistic simulation of complex application communication patterns.