#!/usr/bin/env python3

import os
import re
import sys
import json
import time
import argparse
import subprocess
import numpy as np
import pandas as pd
from pathlib import Path
from datetime import datetime
from itertools import product
from concurrent.futures import ProcessPoolExecutor, as_completed

class NoxExplorer:
    def __init__(self, config_file=None, base_dir=None, noxim_path=None):
        """Initialize the NoC parameter explorer with timestamped directories"""
        # Default paths
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        if base_dir:
            self.base_dir = os.path.join(base_dir, f"run_{timestamp}")
        else:
            self.base_dir = os.path.abspath(f"results/run_{timestamp}")
        
        self.noxim_path = noxim_path if noxim_path else "./noxim"
        self.config_yaml = "../config_examples/0_llm.yaml"
        
        # Default parameters if no config file provided
        self.parameters = {
            "vc": [1, 2, 4],
            "flit": [256],
            "buffer": [4, 8],
            "routing": ["XY"],
            "sel": ["RANDOM", "NOP"],
            "sim": ["6000"]
        }
        
        # Updated regex patterns for metrics extraction including the new throughput metrics
        self.global_metrics_patterns = {
            "traffic_mode": r"% Traffic mode: (\w+)",
            "flit_size": r"% Flit size: (\d+) bits",
            "received_packets": r"% Total received packets: (\d+)",
            "received_flits": r"% Total received flits: (\d+)",
            "flits_ratio": r"% Received/Ideal flits Ratio: ([\d\.]+)",
            "wireless_util": r"% Average wireless utilization: ([\d\.]+)",
            "avg_delay": r"% Global average delay \(cycles\): ([\d\.]+)",
            "max_delay": r"% Max delay \(cycles\): (\d+)",
            "network_throughput": r"% Network throughput \(flits/cycle\): ([\d\.]+)",
            "network_throughput_gbps": r"% \tNetwork throughput \(Gbps\): ([\d\.]+)",
            "network_throughput_GBps": r"% \tNetwork throughput \(GB/s\): ([\d\.]+)",
            "ip_throughput": r"% Average IP throughput \(flits/cycle/IP\): ([\d\.]+)",
            "ip_throughput_gbps": r"% \tAverage IP throughput \(Gbps\): ([\d\.]+)",
            "ip_throughput_GBps": r"% \tAverage IP throughput \(GB/s\): ([\d\.]+)",
            "total_energy": r"% Total energy \(J\): ([\d\.e\-]+)",
            "dynamic_energy": r"% \s+Dynamic energy \(J\): ([\d\.e\-]+)",
            "static_energy": r"% \s+Static energy \(J\): ([\d\.e\-]+)"
        }
        
        self.stall_patterns = {
            "total_stalls": r"% Total stalls: (\d+)",
            "pe_router_stalls": r"% PE→Router stalls: (\d+) \(([\d\.]+)%\)",
            "router_router_stalls": r"% Router→Router stalls: (\d+) \(([\d\.]+)%\)",
            "buffer_full_stalls": r"% Buffer full stalls: (\d+) \(([\d\.]+)%\)",
            "vc_busy_stalls": r"% VC busy stalls: (\d+) \(([\d\.]+)%\)",
            "reservation_stalls": r"% Reservation stalls: (\d+) \(([\d\.]+)%\)",
            "already_reserved_stalls": r"% Already reserved stalls: (\d+) \(([\d\.]+)%\)"
        }
        
        self.stall_direction_patterns = {
            "north_pe_router": r"% \s+North\s+(\d+) \(([\d\.]+)%\)",
            "east_pe_router": r"% \s+East\s+(\d+) \(([\d\.]+)%\)",
            "south_pe_router": r"% \s+South\s+(\d+) \(([\d\.]+)%\)",
            "west_pe_router": r"% \s+West\s+(\d+) \(([\d\.]+)%\)",
            "local_pe_router": r"% \s+Local\s+(\d+) \(([\d\.]+)%\)",
            "hub_pe_router": r"% \s+Hub\s+(\d+) \(([\d\.]+)%\)"
        }
        
        # Load parameters from config file if provided
        if config_file:
            self._load_config(config_file)
            
        # Setup directories
        self.setup_directories()
        
        # Derived attributes
        self.parameter_combinations = []
        self.results = {
            "param_combinations": [],
            "global_metrics": {},
            "stall_statistics": {},
            "heatmaps": {},
            "execution_times": {},
            "errors": {}
        }
        
        print(f"NoxExplorer initialized. Results will be stored in {self.base_dir}")
    
    def _load_config(self, config_file):
        """Load parameters from config file"""
        try:
            with open(config_file, 'r') as f:
                config = json.load(f)
            
            # Update parameters from config
            if 'parameters' in config:
                self.parameters = config['parameters']
                
            # Update paths if specified
            if 'noxim_path' in config:
                self.noxim_path = config['noxim_path']
            if 'config_yaml' in config:
                self.config_yaml = config['config_yaml']
                
            print(f"Loaded configuration from {config_file}")
        except Exception as e:
            print(f"Error loading config file: {e}")
            sys.exit(1)
    
    def setup_directories(self):
        """Create required directory structure with timestamped base directory"""
        directories = [
            self.base_dir,
            os.path.join(self.base_dir, "logs"),
            os.path.join(self.base_dir, "extracted"),
            os.path.join(self.base_dir, "analysis"),
            os.path.join(self.base_dir, "summary")
        ]
        
        for directory in directories:
            os.makedirs(directory, exist_ok=True)
                
        print(f"Directory structure created at {self.base_dir}")
    
    def generate_parameter_combinations(self):
        """Generate all parameter combinations to test"""
        # Extract parameter names and values
        param_names = list(self.parameters.keys())
        param_values = list(self.parameters.values())
        
        # Generate all combinations
        combinations = list(product(*param_values))
        
        # Create parameter dictionaries for each combination
        self.parameter_combinations = [
            dict(zip(param_names, combo)) for combo in combinations
        ]
        
        print(f"Generated {len(self.parameter_combinations)} parameter combinations")
        return self.parameter_combinations
    
    def get_param_identifier(self, params):
        """Generate a unique identifier for a parameter combination"""
        parts = []
        for key, value in params.items():
            parts.append(f"{key}{value}")
        return "_".join(parts)
    
    def run_simulations(self, parallel=True, max_workers=4):
        """Run all simulations with the specified parameters"""
        print(f"Running {len(self.parameter_combinations)} simulations with parallel={parallel}")
        
        if parallel:
            return self.run_simulations_parallel(max_workers)
        else:
            return self.run_simulations_serial()
    
    def run_simulations_parallel(self, max_workers):
        """Run simulations in parallel using a process pool"""
        start_time = time.time()
        successful = 0
        failed = 0
        
        # Adjust workers if we have fewer simulations than workers
        actual_workers = min(max_workers, len(self.parameter_combinations))
        
        print(f"Starting parallel execution with {actual_workers} workers")
        
        with ProcessPoolExecutor(max_workers=actual_workers) as executor:
            # Submit all simulation tasks
            future_to_params = {
                executor.submit(self._run_single_simulation_worker, params): params 
                for params in self.parameter_combinations
            }
            
            # Process results as they complete
            for i, future in enumerate(as_completed(future_to_params)):
                params = future_to_params[future]
                param_id = self.get_param_identifier(params)
                
                try:
                    # Get results from worker process
                    result = future.result()
                    if result:
                        success, log_path, execution_time, global_metrics, stall_statistics, heatmap_data = result
                        
                        if success:
                            # Update results in the main process
                            self.results["param_combinations"].append(param_id)
                            self.results["global_metrics"][param_id] = global_metrics
                            self.results["stall_statistics"][param_id] = stall_statistics
                            self.results["heatmaps"][param_id] = heatmap_data
                            self.results["execution_times"][param_id] = execution_time
                            
                            successful += 1
                            print(f"[{i+1}/{len(self.parameter_combinations)}] "
                                f"Completed simulation for {param_id} in {execution_time:.2f}s")
                        else:
                            failed += 1
                            self.results["errors"][param_id] = "Simulation validation failed"
                            print(f"[{i+1}/{len(self.parameter_combinations)}] "
                                f"Simulation FAILED for {param_id} in {execution_time:.2f}s")
                    else:
                        failed += 1
                        self.results["errors"][param_id] = "Worker returned no result"
                        print(f"[{i+1}/{len(self.parameter_combinations)}] "
                            f"Simulation FAILED for {param_id}")
                            
                except Exception as e:
                    failed += 1
                    self.results["errors"][param_id] = str(e)
                    print(f"[{i+1}/{len(self.parameter_combinations)}] "
                        f"Error in simulation for {param_id}: {e}")
            
            total_time = time.time() - start_time
            print(f"\nSimulations completed: {successful} successful, {failed} failed")
            print(f"Total execution time: {total_time:.2f}s")
            
            return successful, failed

    def _run_single_simulation_worker(self, params):
        """Worker function that runs in a separate process"""
        try:
            param_id = self.get_param_identifier(params)
            log_path = os.path.join(self.base_dir, "logs", f"{param_id}.txt")
            
            # Build command line arguments
            cmd = [self.noxim_path, "-config", self.config_yaml]
            
            for key, value in params.items():
                cmd.extend([f"-{key}", str(value)])
            
            # Run the simulation
            start_time = time.time()
            try:
                with open(log_path, 'w') as log_file:
                    process = subprocess.run(
                        cmd,
                        stdout=log_file,
                        stderr=subprocess.STDOUT,
                        check=True,
                        text=True
                    )
            except subprocess.CalledProcessError as e:
                error_message = f"Command failed with exit code {e.returncode}: {e}"
                print(f"Simulation process error: {error_message}")
                execution_time = time.time() - start_time
                return False, log_path, execution_time, None, None, None
            
            execution_time = time.time() - start_time
            
            # Check if simulation was successful
            with open(log_path, 'r') as f:
                log_content = f.read()
                    
            success, error_message = self.check_simulation_success(log_content)
            
            if not success:
                print(f"Simulation validation failed: {error_message}")
                return False, log_path, execution_time, None, None, None
            
            # Extract metrics
            global_metrics = self.extract_global_metrics(log_content)
            stall_statistics = self.extract_stall_statistics(log_content)
            heatmap_data = self.extract_heatmap(log_content)
            
            # Save extracted data to JSON file
            extracted_file = os.path.join(self.base_dir, "extracted", f"{param_id}.json")
            with open(extracted_file, 'w') as f:
                json.dump({
                    "parameters": params,
                    "global_metrics": global_metrics,
                    "stall_statistics": stall_statistics,
                    "execution_time": execution_time
                }, f, indent=2)
            
            # Return all the data back to the main process
            return True, log_path, execution_time, global_metrics, stall_statistics, heatmap_data
        
        except Exception as e:
            print(f"Worker exception: {e}")
            return None
    
    # Enhanced error reporting in run_single_simulation

    def run_single_simulation(self, params):
        """Run a single Noxim simulation with the specified parameters"""
        param_id = self.get_param_identifier(params)
        log_path = os.path.join(self.base_dir, "logs", f"{param_id}.txt")
        
        # Build command line arguments
        cmd = [self.noxim_path, "-config", self.config_yaml]
        
        # Add parameter-specific arguments
        for key, value in params.items():
            cmd.extend([f"-{key}", str(value)])
        
        # Run the simulation
        start_time = time.time()
        try:
            with open(log_path, 'w') as log_file:
                process = subprocess.run(
                    cmd,
                    stdout=log_file,
                    stderr=subprocess.STDOUT,
                    check=True,
                    text=True
                )
        except subprocess.CalledProcessError as e:
            error_message = f"Command failed with exit code {e.returncode}: {e}"
            print(f"Simulation process error: {error_message}")
            execution_time = time.time() - start_time
            self.results["errors"][param_id] = error_message
            return False, log_path, execution_time
        
        execution_time = time.time() - start_time
        
        # Store results
        self.results["execution_times"][param_id] = execution_time
        
        # Check if simulation was successful
        with open(log_path, 'r') as f:
            log_content = f.read()
                
        success, error_message = self.check_simulation_success(log_content)
        
        if not success:
            self.results["errors"][param_id] = error_message
            print(f"Simulation validation failed: {error_message}")
            return False, log_path, execution_time
        
        # Extract metrics
        self.extract_metrics(log_path, params)
        
        return True, log_path, execution_time
    
    def run_simulations_serial(self):
        """Run simulations serially one after another"""
        start_time = time.time()
        successful = 0
        failed = 0
        
        print("Starting serial execution")
        
        for i, params in enumerate(self.parameter_combinations):
            param_id = self.get_param_identifier(params)
            print(f"[{i+1}/{len(self.parameter_combinations)}] Running simulation for {param_id}")
            
            try:
                success, log_path, execution_time = self.run_single_simulation(params)
                
                if success:
                    successful += 1
                    print(f"Completed simulation in {execution_time:.2f}s")
                else:
                    failed += 1
                    print(f"Simulation FAILED in {execution_time:.2f}s")
            
            except Exception as e:
                failed += 1
                self.results["errors"][param_id] = str(e)
                print(f"Error in simulation: {e}")
        
        total_time = time.time() - start_time
        print(f"\nSimulations completed: {successful} successful, {failed} failed")
        print(f"Total execution time: {total_time:.2f}s")
        
        return successful, failed

    def extract_metrics(self, log_file, params):
        """Extract all metrics from a simulation log file"""
        param_id = self.get_param_identifier(params)
        
        # Store parameter combination
        if param_id not in self.results["param_combinations"]:
            self.results["param_combinations"].append(param_id)
        
        # Read log file
        with open(log_file, 'r') as f:
            log_content = f.read()
        
        # Extract metrics
        global_metrics = self.extract_global_metrics(log_content)
        stall_statistics = self.extract_stall_statistics(log_content)
        heatmap_data = self.extract_heatmap(log_content)
        
        # Store the results
        self.results["global_metrics"][param_id] = global_metrics
        self.results["stall_statistics"][param_id] = stall_statistics
        self.results["heatmaps"][param_id] = heatmap_data
        
        # Save extracted data to JSON file
        extracted_file = os.path.join(self.base_dir, "extracted", f"{param_id}.json")
        with open(extracted_file, 'w') as f:
            json.dump({
                "parameters": params,
                "global_metrics": global_metrics,
                "stall_statistics": stall_statistics,
                "execution_time": self.results["execution_times"][param_id]
            }, f, indent=2)
        
        return global_metrics, stall_statistics, heatmap_data
    
    def extract_global_metrics(self, log_content):
        """Extract global performance metrics"""
        metrics = {}
        
        # Extract each metric using regex patterns
        for metric_name, pattern in self.global_metrics_patterns.items():
            match = re.search(pattern, log_content)
            if match:
                # Try to convert to float or int if possible
                value = match.group(1)
                try:
                    # Convert to float first, then to int if it's a whole number
                    float_val = float(value)
                    metrics[metric_name] = int(float_val) if float_val.is_integer() else float_val
                except ValueError:
                    metrics[metric_name] = value
            else:
                metrics[metric_name] = None
                # Debug line to identify missing metrics
                # print(f"Warning: Could not extract '{metric_name}' using pattern '{pattern}'")
        
        # Debug line to see all metrics        
        # print(f"Extracted metrics: {metrics}")
                    
        return metrics
    
    def extract_stall_statistics(self, log_content):
        """Extract detailed stall statistics"""
        stall_stats = {}
        
        # Extract general stall statistics
        for stat_name, pattern in self.stall_patterns.items():
            match = re.search(pattern, log_content)
            if match:
                value = int(match.group(1))
                percentage = float(match.group(2)) if len(match.groups()) > 1 else None
                stall_stats[stat_name] = value
                stall_stats[f"{stat_name}_pct"] = percentage
            else:
                stall_stats[stat_name] = None
                stall_stats[f"{stat_name}_pct"] = None
        
        # Extract stall direction statistics
        # We need to find the PE->Router and Router->Router sections separately
        pe_router_section = re.search(r"% Direction\s+PE→Router.*?%\s+Local\s+(\d+) \(([\d\.]+)%\)", log_content, re.DOTALL)
        router_router_section = re.search(r"% Direction.*?Router→Router.*?%\s+Hub\s+(\d+) \(([\d\.]+)%\)", log_content, re.DOTALL)
        
        # Extract direction stats for PE->Router if found
        if pe_router_section:
            pe_router_text = pe_router_section.group(0)
            for direction, pattern in self.stall_direction_patterns.items():
                match = re.search(pattern, pe_router_text)
                if match:
                    value = int(match.group(1))
                    percentage = float(match.group(2))
                    stall_stats[f"{direction}_pe_router"] = value
                    stall_stats[f"{direction}_pe_router_pct"] = percentage
                else:
                    stall_stats[f"{direction}_pe_router"] = None
                    stall_stats[f"{direction}_pe_router_pct"] = None
        
        # Extract direction stats for Router->Router if found
        if router_router_section:
            router_router_text = router_router_section.group(0)
            for direction, pattern in self.stall_direction_patterns.items():
                match = re.search(pattern, router_router_text)
                if match:
                    value = int(match.group(1))
                    percentage = float(match.group(2))
                    stall_stats[f"{direction}_router_router"] = value
                    stall_stats[f"{direction}_router_router_pct"] = percentage
                else:
                    stall_stats[f"{direction}_router_router"] = None
                    stall_stats[f"{direction}_router_router_pct"] = None
                    
        return stall_stats
    
    def extract_heatmap(self, log_content):
        """Extract raw stall heatmap data"""
        # Find heatmap section between markers
        heatmap_pattern = r"% Raw stall counts:\n%(.*?)%\n% === Stall Statistics Summary ==="
        heatmap_match = re.search(heatmap_pattern, log_content, re.DOTALL)
        
        if not heatmap_match:
            return None
            
        raw_data = heatmap_match.group(1).strip().split('\n')
        
        # Parse the data into a numerical matrix
        heatmap_matrix = []
        for line in raw_data:
            # Split by whitespace and convert to integers
            values = [int(x) for x in line.split() if x.isdigit() or (x.replace('-', '').isdigit() and x != '%')]
            if values:
                heatmap_matrix.append(values)
                
        return heatmap_matrix if heatmap_matrix else None
    
    def compile_results(self):
        """Compile results from all simulations with error reporting"""
        print("Compiling results from all simulations...")
        
        # Check if we have any successful results
        if not self.results["global_metrics"]:
            print("No successful simulations to compile results from.")
            return False
                
        # Create summary tables
        summary_path = os.path.join(self.base_dir, "summary", "summary.txt")
        
        with open(summary_path, 'w') as summary_file:
            # Write header
            summary_file.write("# NOXIM SIMULATION RESULTS SUMMARY\n")
            summary_file.write("==============================================\n")
            summary_file.write(f"Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
            summary_file.write(f"Total Simulations: {len(self.parameter_combinations)}\n")
            summary_file.write(f"Parameters Tested: {', '.join(self.parameters.keys())}\n\n")
            
            # Write error summary if any
            if self.results["errors"]:
                summary_file.write("# SIMULATION ERRORS\n")
                summary_file.write("==============================================\n")
                summary_file.write(f"{'Configuration':<30} | {'Error Reason':<50}\n")
                summary_file.write(f"{'-'*30}-|{'-'*50}\n")
                
                for param_id, error_msg in self.results["errors"].items():
                    summary_file.write(f"{param_id:<30} | {error_msg:<50}\n")
                
                summary_file.write("\n")
            
            # Write performance comparison table
            summary_file.write("# PERFORMANCE COMPARISON\n")
            summary_file.write("==============================================\n")
            summary_file.write(f"{'Configuration':<30} | {'Avg Delay':<10} | {'Throughput':<10} | {'Thpt (Gbps)':<10} | {'Thpt (GB/s)':<10} | {'Energy (J)':<10} | {'Exec Time':<10}\n")
            summary_file.write(f"{'-'*30}-|{'-'*12}|{'-'*12}|{'-'*12}|{'-'*12}|{'-'*12}|{'-'*12}\n")
            
            for param_id in self.results["param_combinations"]:
                global_metrics = self.results["global_metrics"][param_id]
                exec_time = self.results["execution_times"].get(param_id, 0)
                
                # Get values with safe defaults for None values
                avg_delay = global_metrics.get("avg_delay", "N/A")
                throughput = global_metrics.get("network_throughput", "N/A")
                throughput_gbps = global_metrics.get("network_throughput_gbps", "N/A")
                throughput_GBps = global_metrics.get("network_throughput_GBps", "N/A")
                energy = global_metrics.get("total_energy", "N/A")
                
                # Format with safe string conversion to handle None values
                summary_file.write(f"{param_id:<30} | {str(avg_delay):<10} | {str(throughput):<10} | " +
                                f"{str(throughput_gbps):<10} | {str(throughput_GBps):<10} | " +
                                f"{str(energy):<10} | {exec_time:<.2f}s\n")
            
            # Write stall statistics summary
            summary_file.write("\n# STALL STATISTICS SUMMARY\n")
            summary_file.write("==============================================\n")
            summary_file.write(f"{'Configuration':<30} | {'Total Stalls':<12} | {'PE→Router':<9} | {'Router→Router':<13} | {'Buffer Full':<11} | {'VC Busy':<8}\n")
            summary_file.write(f"{'-'*30}-|{'-'*14}|{'-'*11}|{'-'*15}|{'-'*13}|{'-'*10}\n")
            
            for param_id in self.results["param_combinations"]:
                stall_stats = self.results["stall_statistics"][param_id]
                
                total_stalls = stall_stats.get("total_stalls", "N/A")
                pe_router_pct = stall_stats.get("pe_router_stalls_pct", "N/A")
                router_router_pct = stall_stats.get("router_router_stalls_pct", "N/A")
                buffer_full_pct = stall_stats.get("buffer_full_stalls_pct", "N/A")
                vc_busy_pct = stall_stats.get("vc_busy_stalls_pct", "N/A")
                
                summary_file.write(f"{param_id:<30} | {total_stalls:<12} | {pe_router_pct:<7}% | {router_router_pct:<11}% | {buffer_full_pct:<9}% | {vc_busy_pct:<6}%\n")
                
            # Calculate and write summary statistics
            summary_file.write("\n# SUMMARY STATISTICS\n")
            summary_file.write("==============================================\n")
            summary_file.write(f"{'Metric':<20} | {'Minimum':<9} | {'Maximum':<9} | {'Average':<9} | {'Best Configuration':<30}\n")
            summary_file.write(f"{'-'*20}-|{'-'*11}|{'-'*11}|{'-'*11}|{'-'*30}\n")
            
            # Calculate statistics for key metrics (expanded with new throughput metrics)
            metrics_to_summarize = {
                "avg_delay": ("min", "Global average delay"),
                "network_throughput": ("max", "Network throughput"),
                "network_throughput_gbps": ("max", "Network thpt (Gbps)"),
                "network_throughput_GBps": ("max", "Network thpt (GB/s)"),
                "ip_throughput_gbps": ("max", "IP thpt (Gbps)"),
                "ip_throughput_GBps": ("max", "IP thpt (GB/s)"),
                "total_energy": ("min", "Total energy"),
                "total_stalls": ("min", "Total stalls")
            }
                    
            for metric_key, (best_type, metric_name) in metrics_to_summarize.items():
                values = []
                configs = []
                
                for param_id in self.results["param_combinations"]:
                    if metric_key in self.results["global_metrics"].get(param_id, {}):
                        values.append(self.results["global_metrics"][param_id][metric_key])
                        configs.append(param_id)
                    elif metric_key in self.results["stall_statistics"].get(param_id, {}):
                        values.append(self.results["stall_statistics"][param_id][metric_key])
                        configs.append(param_id)
                
                if values:
                    min_val = min(values)
                    max_val = max(values)
                    avg_val = sum(values) / len(values)
                    
                    # Determine best configuration
                    if best_type == "min":
                        best_val = min_val
                        best_idx = values.index(best_val)
                    else:
                        best_val = max_val
                        best_idx = values.index(best_val)
                        
                    best_config = configs[best_idx]
                    
                    summary_file.write(f"{metric_name:<20} | {min_val:<9} | {max_val:<9} | {avg_val:<9.2f} | {best_config:<30}\n")
        
        print(f"Results summary written to {summary_path}")
        return True
    
    def generate_csv_reports(self):
        """Generate CSV reports from compiled results"""
        print("Generating CSV reports...")
        
        # Check if we have any successful results
        if not self.results["global_metrics"]:
            print("No successful simulations to generate reports from.")
            return False
            
        # Create main_stats.csv
        main_stats_path = os.path.join(self.base_dir, "summary", "main_stats.csv")
        stall_stats_path = os.path.join(self.base_dir, "summary", "stall_stats.csv")
        stall_direction_path = os.path.join(self.base_dir, "summary", "stall_direction.csv")
        
        # Prepare data for main stats CSV
        main_stats_data = []
        
        for param_id in self.results["param_combinations"]:
            # Extract parameter values from the ID
            params = {}
            for part in param_id.split("_"):
                # Extract parameter name and value (e.g., "vc1" -> "vc": "1")
                for param_name in self.parameters.keys():
                    if part.startswith(param_name):
                        params[param_name] = part[len(param_name):]
            
            # Get metrics
            global_metrics = self.results["global_metrics"][param_id]
            exec_time = self.results["execution_times"].get(param_id, 0)
            
            # Create row
            row = {
                "param_set": param_id,
                "execution_time": exec_time
            }
            
            # Add parameters
            for param_name in self.parameters.keys():
                row[param_name] = params.get(param_name, "")
                
            # Add global metrics
            for metric_name, value in global_metrics.items():
                row[metric_name] = value
                
            main_stats_data.append(row)
            
        # Create DataFrame and save to CSV
        main_stats_df = pd.DataFrame(main_stats_data)
        main_stats_df.to_csv(main_stats_path, index=False)
        
        # Prepare data for stall stats CSV
        stall_stats_data = []
        
        for param_id in self.results["param_combinations"]:
            # Extract parameter values
            params = {}
            for part in param_id.split("_"):
                for param_name in self.parameters.keys():
                    if part.startswith(param_name):
                        params[param_name] = part[len(param_name):]
            
            # Get stall statistics
            stall_stats = self.results["stall_statistics"][param_id]
            
            # Create row
            row = {
                "param_set": param_id
            }
            
            # Add parameters
            for param_name in self.parameters.keys():
                row[param_name] = params.get(param_name, "")
                
            # Add stall statistics (main categories only)
            main_stall_metrics = [
                "total_stalls", "pe_router_stalls", "router_router_stalls",
                "buffer_full_stalls", "vc_busy_stalls", "reservation_stalls",
                "already_reserved_stalls"
            ]
            
            for metric_name in main_stall_metrics:
                row[metric_name] = stall_stats.get(metric_name, None)
                row[f"{metric_name}_pct"] = stall_stats.get(f"{metric_name}_pct", None)
                
            stall_stats_data.append(row)
            
        # Create DataFrame and save to CSV
        stall_stats_df = pd.DataFrame(stall_stats_data)
        stall_stats_df.to_csv(stall_stats_path, index=False)
        
        # Prepare data for stall direction CSV
        stall_direction_data = []
        
        for param_id in self.results["param_combinations"]:
            # Extract parameter values
            params = {}
            for part in param_id.split("_"):
                for param_name in self.parameters.keys():
                    if part.startswith(param_name):
                        params[param_name] = part[len(param_name):]
            
            # Get stall statistics
            stall_stats = self.results["stall_statistics"][param_id]
            
            # Create row
            row = {
                "param_set": param_id
            }
            
            # Add parameters
            for param_name in self.parameters.keys():
                row[param_name] = params.get(param_name, "")
                
            # Add stall direction statistics
            directions = ["north", "east", "south", "west", "local", "hub"]
            types = ["pe_router", "router_router"]
            
            for direction in directions:
                for type_ in types:
                    metric_name = f"{direction}_{type_}"
                    row[metric_name] = stall_stats.get(metric_name, None)
                    row[f"{metric_name}_pct"] = stall_stats.get(f"{metric_name}_pct", None)
                    
            stall_direction_data.append(row)
            
        # Create DataFrame and save to CSV
        stall_direction_df = pd.DataFrame(stall_direction_data)
        stall_direction_df.to_csv(stall_direction_path, index=False)
        
        print(f"CSV reports generated at:\n - {main_stats_path}\n - {stall_stats_path}\n - {stall_direction_path}")
        return True
    
    def generate_html_report(self):
        """Generate HTML report with interactive visualizations"""
        # This is left as a template function as per request
        print("HTML report generation is not implemented in this version")
        return True
    
    def check_simulation_success(self, log_content):
        """Check if a simulation completed successfully with detailed error reason"""
        # Check for simulation completion
        if "Noxim simulation completed." not in log_content:
            return False, "Simulation failed: 'Noxim simulation completed.' not found"
        
        # Check for 100% completion rate
        completion_match = re.search(r"% Completion Rate: \d+/\d+ \(([\d\.]+)%\)", log_content)
        if not completion_match:
            return False, "Simulation failed: Completion rate information not found"
        elif completion_match.group(1) != "100.00":
            return False, f"Simulation failed: Completion rate was {completion_match.group(1)}%, not 100.00%"
                
        return True, "Success"

def main():
    """Main function to parse arguments and run the explorer"""
    parser = argparse.ArgumentParser(description="Noxim parameter exploration tool")
    
    # Add command line arguments
    parser.add_argument('-c', '--config', help='Configuration file for parameter exploration')
    parser.add_argument('-s', '--serial', action='store_true', help='Run simulations serially (default is parallel)')
    parser.add_argument('-w', '--workers', type=int, default=4, help='Number of parallel workers')
    parser.add_argument('-n', '--noxim', help='Path to noxim executable', default='./noxim')
    parser.add_argument('-o', '--output', help='Base output directory for results', default='results')
    
    # Parse arguments
    args = parser.parse_args()
    
    # Create and run the explorer
    explorer = NoxExplorer(
        config_file=args.config, 
        base_dir=args.output,
        noxim_path=args.noxim
    )
    
    explorer.generate_parameter_combinations()
    explorer.run_simulations(parallel=not args.serial, max_workers=args.workers)
    explorer.compile_results()
    explorer.generate_csv_reports()
    
    print(f"\nExploration complete! Results are available at {explorer.base_dir}")

if __name__ == "__main__":
    main()