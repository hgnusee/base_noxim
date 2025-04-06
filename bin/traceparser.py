#!/usr/bin/env python3

import sys
import datetime
from pathlib import Path

class TraceConverter:
    def __init__(self, input_file, output_file):
        self.input_file = input_file
        self.output_file = output_file
        self.comments = []
        self.tasks = []
    
    def convert(self):
        self.parse_trace_file()
        self.write_traffic_table()
        print(f"Conversion complete: {self.output_file}")
        
    def parse_trace_file(self):
        comment_count = 0
        
        with open(self.input_file, 'r') as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                    
                # Handle comments
                if line.startswith('#'):
                    if comment_count < 3:
                        # Convert # to % for traffic table comments
                        self.comments.append("%" + line[1:])
                        comment_count += 1
                    continue
                
                # Process task line (tab-separated)
                fields = line.split('\t')
                if len(fields) >= 5:
                    task_id = fields[0].strip()
                    src_pe = fields[1].strip()
                    dst_pe = fields[2].strip()
                    size = fields[3].strip()
                    wait_ids = fields[4].strip()
                    
                    # Convert to traffic table format
                    self.tasks.append(self.format_task(task_id, src_pe, dst_pe, size, wait_ids))
    
    def format_task(self, task_id, src_pe, dst_pe, size, wait_ids):
        # Format waitIDs
        if wait_ids == "None":
            wait_id_str = "[-1]"
        else:
            wait_ids_list = [id.strip() for id in wait_ids.split(',')]
            wait_id_str = f"[{' '.join(wait_ids_list)}]"
        
        # Determine if self-compute task (src == dst)
        is_self_compute = (src_pe == dst_pe)
        
        # Determine if task has dependencies
        has_dependencies = (wait_ids != "None")
        
        # Create traffic entry based on task type
        if is_self_compute:
            # Self-compute pattern: -1 size -1 size
            entry = f"{task_id} 1 [{src_pe}] [{dst_pe}] -1 {size} -1 {size} {wait_id_str} [-1] 0"
        else:
            # Standard task pattern: -1 size size size
            entry = f"{task_id} 1 [{src_pe}] [{dst_pe}] -1 {size} {size} {size} {wait_id_str} [-1] 0"
            # Add wait flag for tasks with dependencies
            if has_dependencies:
                entry += " w"
                
        return entry
    
    def write_traffic_table(self):
        with open(self.output_file, 'w') as f:
            # Write original comments
            for comment in self.comments:
                f.write(comment + "\n")
                
            # Always add timestamp comment
            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            f.write(f"% Traffic table converted on {timestamp}\n")
            
            # Write all tasks
            for task in self.tasks:
                f.write(task + "\n")

def main():
    if len(sys.argv) < 3:
        print("Usage: python trace_converter.py <input_trace_file> <output_traffic_file>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    if not Path(input_file).exists():
        print(f"Error: Input file {input_file} not found.")
        sys.exit(1)
    
    converter = TraceConverter(input_file, output_file)
    converter.convert()

if __name__ == "__main__":
    main()