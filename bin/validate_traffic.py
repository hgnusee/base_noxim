#!/usr/bin/env python3

import sys
import re
import os

def validate_traffic_file(filename):
    print(f"Validating traffic file: {filename}")
    
    if not os.path.exists(filename):
        print(f"Error: File '{filename}' does not exist")
        return
    
    with open(filename, 'r') as file:
        lines = file.readlines()
    
    valid_lines = 0
    invalid_lines = 0
    result_lines = []
    
    for line_num, line in enumerate(lines, 1):
        line = line.strip()
        result_line = line
        
        # Skip empty lines or comments
        if not line or line.startswith('%'):
            result_lines.append(line)
            continue
        
        issues = []
        
        # Check basic format using regex pattern matching
        pattern = r'^\s*(\d+)\s+(\d+)\s+\[(.*?)\]\s+\[(.*?)\]\s+(-?\d+)\s+(\d+)\s+(-?\d+)\s+(\d+)\s+\[(.*?)\]\s+\[(.*?)\]\s+(\d+)(?:\s+([umb]))?\s*$'
        match = re.match(pattern, line)
        
        if not match:
            issues.append("Line does not match required format: taskID layerID [src] [dst] src_minVol src_totalVol dst_minVol dst_totalVol [waitID] [nextID] waitOP [traffic_type]")
        else:
            # Extract all fields
            taskID, layerID, src_str, dst_str, src_minVol, src_totalVol, dst_minVol, dst_totalVol, waitID_str, nextID_str, waitOP, traffic_type = match.groups()
            
            # Parse vectors
            src = [int(x) for x in src_str.split() if x]
            dst = [int(x) for x in dst_str.split() if x]
            waitID = [int(x) for x in waitID_str.split() if x]
            nextID = [int(x) for x in nextID_str.split() if x]
            
            # Rule 1: Either src or dst must be size 1, while the other can be >1
            # Update: Allow many-to-many traffic as valid
            if not ((len(src) == 1 and len(dst) >= 1) or (len(src) > 1 and len(dst) == 1) or (len(src) > 1 and len(dst) > 1)):
                issues.append(f"Either src or dst must be size 1, or both can be >1 (many-to-many). Current: src={len(src)}, dst={len(dst)}")
            
            # Rule 2: If waitID contains -1, it must not have more than 1 element
            if -1 in waitID and len(waitID) > 1:
                issues.append("If waitID contains -1, it must not have more than 1 element")
            
            # Rule 3: waitID[0] must be >= -1
            if waitID and waitID[0] < -1:
                issues.append(f"waitID[0] must be >= -1. Current: {waitID[0]}")
            
            # Rule 4: If traffic_type is provided, it must be one of 'u', 'm', or 'b'
            if traffic_type and traffic_type not in ['u', 'm', 'b']:
                issues.append(f"traffic_type must be one of 'u', 'm', or 'b'. Current: {traffic_type}")
            
            # Additional validation: multicast traffic should have src.size() == 1 and dst.size() > 1
            if traffic_type == 'm' and not (len(src) == 1 and len(dst) > 1):
                issues.append(f"Multicast traffic ('m') must have src.size() == 1 and dst.size() > 1. Current: src={len(src)}, dst={len(dst)}")
        
        if issues:
            invalid_lines += 1
            issue_str = ", ".join(issues)
            print(f"Line {line_num}: INVALID - {issue_str}")
            result_lines.append(f"% ERROR: {issue_str}\n% {line}")
        else:
            valid_lines += 1
            result_lines.append(line)
    
    # Write fixed file
    fixed_filename = filename + ".fixed"
    with open(fixed_filename, 'w') as file:
        file.write('\n'.join(result_lines))
    
    print(f"\nSummary:")
    print(f"  Valid lines: {valid_lines}")
    print(f"  Invalid lines: {invalid_lines}")
    print(f"  Total non-comment lines: {valid_lines + invalid_lines}")
    print(f"\nFixed file written to: {fixed_filename}")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python validate_traffic.py <traffic_file>")
        sys.exit(1)
    
    validate_traffic_file(sys.argv[1])