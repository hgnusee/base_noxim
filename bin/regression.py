#!/usr/bin/env python3

import os
import re
import subprocess
import sys
import argparse

# ANSI color codes
class Colors:
    YELLOW = '\033[93m'
    RESET = '\033[0m'

# Configuration
NOXIM_PATH = "/home/hw/noxim_base/base_noxim/bin/noxim"
TRAFFIC_FILE = "/home/hw/noxim_base/base_noxim/bin/o2m.txt"
CONFIG_FILE = "/home/hw/noxim_base/base_noxim/config_examples/2_o2mTraffic.yaml"
LOG_FILE = "dbg_multicast.txt"

def parse_args():
    parser = argparse.ArgumentParser(description='Noxim Regression Testing Tool')
    parser.add_argument('-sim', type=int, default=5000, help='Simulation cycles')
    parser.add_argument('-vc', type=int, default=1, help='Number of virtual channels')
    parser.add_argument('-compute_delay', type=int, default=10, help='Computation delay')
    parser.add_argument('-help', action='store_true', help='Show this help message')
    args = parser.parse_args()
    
    if args.help:
        print_help()
        sys.exit(0)
        
    return args

def print_help():
    print("""
Noxim Regression Testing Tool
-----------------------------
This script automates testing of different traffic patterns in the o2m.txt file.

Usage:
  ./regression.py [options]

Options:
  -sim N             Set simulation cycles (default: 5000)
  -vc N              Set number of virtual channels (default: 1)
  -compute_delay N   Set computation delay (default: 10)
  -help              Show this help message

Operation:
  The script will:
  1. Find all traffic patterns in o2m.txt
  2. For each pattern:
     - Uncomment the pattern
     - Run noxim with the specified parameters
     - Write output to dbg_multicast.txt
     - Wait for user to press Enter before moving to next pattern
    """)

class TrafficTester:
    def __init__(self, traffic_file, noxim_path, config_file, args):
        self.traffic_file = traffic_file
        self.noxim_path = noxim_path
        self.config_file = config_file
        self.args = args
        
    def read_file(self):
        """Read the traffic file"""
        with open(self.traffic_file, 'r', encoding='utf-8') as f:
            return f.readlines()
    
    def write_file(self, lines):
        """Write lines back to the traffic file"""
        with open(self.traffic_file, 'w', encoding='utf-8') as f:
            f.writelines(lines)
    
    def ensure_all_commented(self):
        """Ensure all traffic pattern lines start with '%'"""
        lines = self.read_file()
        modified = False
        
        # Pattern to match traffic table rows - with or without % prefix
        pattern = re.compile(r'^(%+\s*)?(\d+\s+\d+\s+\[\S+\]\s+\[\S+\]\s+(?:-?\d+)\s+\d+\s+(?:-?\d+)\s+\d+\s+\[\S+\]\s+\[\S+\]\s+\d+(?:\s+[um])?)$')
        
        for i, line in enumerate(lines):
            match = pattern.match(line.strip())
            if match and not line.strip().startswith('%'):
                lines[i] = '% ' + line
                modified = True
                
        if modified:
            print("Note: Some traffic patterns were not commented. Adding '%' to these lines.")
            self.write_file(lines)
            
        return lines
            
    def find_traffic_blocks(self, lines):
        """Find all traffic table blocks in the file"""
        traffic_blocks = []
        current_block = []
        
        # Pattern to match traffic table rows - more precise matching
        # Matches: taskID layerID [src] [dst] src_minVol src_totalVol dst_minVol dst_totalVol [waitID] [nextID] waitOP [traffic]
        traffic_pattern = re.compile(r'^(%\s*)?\d+\s+\d+\s+\[\S+\]\s+\[\S+\]\s+(?:-?\d+)\s+\d+\s+(?:-?\d+)\s+\d+\s+\[\S+\]\s+\[\S+\]\s+\d+(?:\s+[um])?$')
        
        i = 0
        while i < len(lines):
            line = lines[i].strip()
            
            if not line:  # Skip empty lines
                i += 1
                continue
            
            # Check if this is a traffic table row
            if traffic_pattern.match(line):
                # This is a traffic row - add it to the current block
                current_block.append(i)
            else:
                # This is not a traffic row
                
                # If we were collecting traffic rows, end the block
                if current_block:
                    traffic_blocks.append(current_block)
                    current_block = []
            
            i += 1
        
        # Don't forget the last block
        if current_block:
            traffic_blocks.append(current_block)
        
        # Debug - print what blocks were found
        if traffic_blocks:
            print("\nDetected traffic blocks:")
            for idx, block in enumerate(traffic_blocks):
                print(f"  Block {idx+1}: {len(block)} traffic rows")
                # Print first line of each block for reference
                first_line = lines[block[0]].strip()
                if len(first_line) > 60:
                    first_line = first_line[:57] + "..."
                print(f"    First line: {first_line}")
            print()
        
        return traffic_blocks
    
    def uncomment_block(self, lines, block_indices):
        """Uncomment a traffic block by removing the % character"""
        for i in block_indices:
            lines[i] = re.sub(r'^%+\s*', '', lines[i])
        return lines
    
    def comment_block(self, lines, block_indices):
        """Comment out a traffic block"""
        for i in block_indices:
            if not lines[i].startswith('%'):
                lines[i] = '% ' + lines[i]  # Add % and a space
        return lines
    
    def analyze_log_file(self):
        """Analyze the log file to check for potential failures"""
        try:
            with open(LOG_FILE, 'r', encoding='utf-8') as f:
                lines = f.readlines()
            
            # Find the line that contains "Noxim simulation completed"
            completion_index = None
            for i, line in enumerate(lines):
                if "Noxim simulation completed" in line:
                    completion_index = i
                    break
            
            if completion_index is None:
                print("Warning: Could not find 'Noxim simulation completed' in log file.")
                return
            
            # Print the last 5 lines before completion
            print("\nLast 5 lines before completion:")
            start_idx = max(0, completion_index - 5)
            for i in range(start_idx, completion_index):
                print(f"  {lines[i].strip()}")
            
            # Check the last 10 lines for potential failure (reaching max simulation time)
            start_idx = max(0, completion_index - 10)
            warning_threshold = self.args.sim + 999
            for i in range(start_idx, completion_index):
                # Extract first word which should be a number (cycle count)
                words = lines[i].strip().split()
                if words and words[0].isdigit():
                    cycle = int(words[0])
                    if cycle >= warning_threshold:
                        print(f"{Colors.YELLOW}WARNING: Potential simulation failure detected!")
                        print(f"Simulation reached cycle {cycle}, which is close to or exceeding the max simulation time.")
                        print(f"Check if your traffic pattern completed successfully.{Colors.RESET}")
                        break
                        
        except Exception as e:
            print(f"Error analyzing log file: {e}")
    
    def run_noxim(self):
        """Run Noxim simulator with the current traffic file"""
        print("\n" + "="*50)
        print(f"Running Noxim with traffic file: {self.traffic_file}")
        print("="*50)
        
        # Extract and print the uncommented traffic pattern for reference
        with open(self.traffic_file, 'r', encoding='utf-8') as f:
            content = f.read()
            pattern_lines = [line for line in content.split('\n') 
                            if line.strip() and not line.strip().startswith('%') 
                            and re.match(r'\d+\s+\d+\s+\[', line)]
            
            print("\nCurrent Traffic Pattern:")
            for line in pattern_lines:
                print(f"  {line}")
            print()
        
        try:
            # Run noxim with the traffic table and parameters
            cmd = [
                self.noxim_path, 
                "-config", self.config_file,
                "-sim", str(self.args.sim),
                "-vc", str(self.args.vc),
                "-compute_delay", str(self.args.compute_delay)
            ]
            
            print(f"Executing: {' '.join(cmd)}")
            print(f"Output will be written to {LOG_FILE}")
            
            # Run the process with output redirected to file
            with open(LOG_FILE, 'w', encoding='utf-8') as log_file:
                process = subprocess.Popen(
                    cmd, 
                    stdout=log_file,
                    stderr=subprocess.STDOUT
                )
                return_code = process.wait()
                
            if return_code != 0:
                print(f"Warning: Noxim exited with return code {return_code}")
                return False
                
            print(f"Noxim execution completed. Check {LOG_FILE} for details.")
            
            # Analyze log file after completion
            self.analyze_log_file()
            
        except Exception as e:
            print(f"Error running Noxim: {e}")
            return False
            
        return True
    
    def run_tests(self):
        """Run tests for all traffic blocks"""
        # First, ensure all traffic patterns are initially commented
        lines = self.ensure_all_commented()
        
        # Find all traffic blocks
        traffic_blocks = self.find_traffic_blocks(lines)
        
        if not traffic_blocks:
            print("No traffic patterns found in the file.")
            return
        
        print(f"Found {len(traffic_blocks)} traffic patterns to test.")
        
        # Test each block
        for i, block in enumerate(traffic_blocks):
            print(f"\n\nTesting traffic pattern {i+1}/{len(traffic_blocks)}")
            
            # Show which lines are in this block
            print("\nTraffic rows in this block:")
            for j, idx in enumerate(block[:5]):  # Show up to 5 rows
                print(f"  {lines[idx].strip()}")
            if len(block) > 5:
                print(f"  ... and {len(block) - 5} more rows")
            
            # Uncomment this block
            lines = self.uncomment_block(lines, block)
            
            # Comment out all other blocks to avoid interference
            for other_block in traffic_blocks:
                if other_block != block:
                    lines = self.comment_block(lines, other_block)
            
            # Write the modified file back
            self.write_file(lines)
            
            # Run Noxim
            success = self.run_noxim()
            
            if success:
                print("\nSimulation completed.")
            else:
                print("\nSimulation failed. Continuing with next pattern.")
            
            # Wait for user input
            input("\nPress Enter to continue to the next pattern...")
            
            # Comment out this block again
            lines = self.comment_block(lines, block)
            self.write_file(lines)
        
        print("\nAll traffic patterns tested!")

if __name__ == "__main__":
    # Parse command line arguments
    args = parse_args()
    
    # Check if noxim exists
    if not os.path.exists(NOXIM_PATH):
        print(f"Error: Noxim executable not found at {NOXIM_PATH}")
        print("Please update the NOXIM_PATH variable in the script.")
        sys.exit(1)
    
    # Check if traffic file exists
    if not os.path.exists(TRAFFIC_FILE):
        print(f"Error: Traffic file not found at {TRAFFIC_FILE}")
        print("Please update the TRAFFIC_FILE variable in the script.")
        sys.exit(1)
    
    # Check if config file exists
    if not os.path.exists(CONFIG_FILE):
        print(f"Warning: Config file not found at {CONFIG_FILE}")
        print("Will attempt to run Noxim without a config file.")
    
    # Run the tests
    tester = TrafficTester(TRAFFIC_FILE, NOXIM_PATH, CONFIG_FILE, args)
    tester.run_tests()