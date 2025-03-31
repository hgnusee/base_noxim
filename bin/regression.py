#!/usr/bin/env python3

import os
import re
import subprocess
import sys
import argparse
import yaml
import tempfile
from pathlib import Path

# Configuration
NOXIM_PATH = "/home/hw/noxim_base/base_noxim/bin/noxim"
CONFIG_FILE = "/home/hw/noxim_base/base_noxim/config_examples/2_o2mTraffic.yaml"
TEST_CASES_FILE = "/home/hw/noxim_base/base_noxim/bin/traffic_test_cases.txt"
TEMP_TRAFFIC_FILE = "/home/hw/noxim_base/base_noxim/bin/current_traffic.txt"
RESULTS_DIR = "/home/hw/noxim_base/base_noxim/bin/test_results"

# Default parameters
DEFAULT_SIM_CYCLES = 5000
DEFAULT_VC = 1
DEFAULT_COMPUTE_DELAY = 10

def parse_args():
    parser = argparse.ArgumentParser(description='Noxim Regression Testing Tool')
    parser.add_argument('-sim', type=int, default=None, help='Override simulation cycles')
    parser.add_argument('-vc', type=int, default=None, help='Override number of virtual channels')
    parser.add_argument('-compute_delay', type=int, default=None, help='Override computation delay')
    parser.add_argument('--test', type=str, help='Run a specific test by name')
    parser.add_argument('--list', action='store_true', help='List available tests')
    parser.add_argument('--auto', action='store_true', help='Run all tests without prompting')
    parser.add_argument('--import', dest='import_file', type=str, help='Import a test from o2m.txt with given name')
    args = parser.parse_args()
    return args

class TestCasesParser:
    def __init__(self, test_cases_file):
        self.test_cases_file = test_cases_file
        
    def list_test_cases(self):
        """List all available test cases with their metadata"""
        test_cases = []
        
        with open(self.test_cases_file, 'r') as f:
            content = f.read()
            
        # Find all test cases
        pattern = r'### BEGIN_TEST ###(.*?)### END_TEST ###'
        matches = re.findall(pattern, content, re.DOTALL)
        
        for content in matches:
            # Extract test case name
            name = "unnamed_test"
            name_match = re.search(r'# Test Case: (.*)', content)
            if name_match:
                name = name_match.group(1).strip()
                
            # Extract description if present
            description = "No description"
            description_match = re.search(r'# Description: (.*)', content)
            if description_match:
                description = description_match.group(1).strip()
                
            # Extract parameters if present
            parameters = {
                'sim': DEFAULT_SIM_CYCLES,
                'vc': DEFAULT_VC,
                'compute_delay': DEFAULT_COMPUTE_DELAY
            }
            parameters_match = re.search(r'# Parameters: (.*)', content)
            if parameters_match:
                params_str = parameters_match.group(1).strip()
                for param in params_str.split():
                    key, value = param.split('=', 1)
                    parameters[key] = int(value)
                    
            test_cases.append({
                'name': name,
                'description': description,
                'parameters': parameters
            })
            
        return test_cases
    
    def extract_test_case(self, test_name):
        """Extract a specific test case by name"""
        test_cases = self.list_test_cases()
        
        # Find the test case with matching name
        for test in test_cases:
            if test['name'] == test_name:
                # Now extract the actual traffic rows for this test
                with open(self.test_cases_file, 'r') as f:
                    content = f.read()
                
                # Find all test blocks
                pattern = r'### BEGIN_TEST ###(.*?)### END_TEST ###'
                matches = re.findall(pattern, content, re.DOTALL)
                
                # Find the matching block
                for block in matches:
                    # Check if this block matches the test name
                    name_match = re.search(r'# Test Case: (.*)', block)
                    if name_match and name_match.group(1).strip() == test_name:
                        # Extract traffic rows (non-comment lines)
                        traffic_rows = []
                        for line in block.split('\n'):
                            line = line.strip()
                            if not line or line.startswith('#'):
                                continue
                            traffic_rows.append(line)
                        
                        return traffic_rows, test['parameters']
                
                # If we get here, something went wrong
                return [], test['parameters']
        
        # Test not found
        return [], {}
    
    def import_test_from_file(self, source_file, test_name, description=""):
        """Import a test from an existing file and add it to the test cases file"""
        # Read the source file
        with open(source_file, 'r') as f:
            content = f.readlines()
            
        # Extract uncommented traffic rows
        traffic_rows = []
        for line in content:
            line = line.strip()
            # Skip empty lines, comments or comments with the % prefix
            if not line or line.startswith('%'):
                continue
                
            # Check if this looks like a traffic row (basic check)
            parts = line.split()
            if len(parts) >= 10 and '[' in line and ']' in line:
                traffic_rows.append(line)
                
        if not traffic_rows:
            print("No uncommented traffic rows found in the source file.")
            return False
            
        # Create the test case entry
        test_case = f"""
### BEGIN_TEST ###
# Test Case: {test_name}
# Description: {description if description else "Imported from " + source_file}
# Parameters: sim={DEFAULT_SIM_CYCLES} vc={DEFAULT_VC} compute_delay={DEFAULT_COMPUTE_DELAY}

{os.linesep.join(traffic_rows)}

### END_TEST ###
"""
        
        # Append to the test cases file
        with open(self.test_cases_file, 'a') as f:
            f.write(test_case)
            
        print(f"Test '{test_name}' imported successfully with {len(traffic_rows)} traffic rows.")
        return True

class NoximTester:
    def __init__(self, args):
        self.args = args
        self.parser = TestCasesParser(TEST_CASES_FILE)
        Path(RESULTS_DIR).mkdir(exist_ok=True)
    
    def list_available_tests(self):
        """List all available test cases"""
        test_cases = self.parser.list_test_cases()
        
        if not test_cases:
            print("No test cases found. Import tests using --import option.")
            return
            
        print(f"\nAvailable test cases ({len(test_cases)}):")
        for idx, test in enumerate(test_cases, 1):
            param_str = f"sim={test['parameters']['sim']} vc={test['parameters']['vc']} compute_delay={test['parameters']['compute_delay']}"
            print(f"  {idx}. {test['name']}: {test['description']} ({param_str})")
    
    def update_config_file(self, traffic_file):
        """Create a test-specific config file pointing to the traffic file"""
        try:
            test_config_file = "/home/hw/noxim_base/base_noxim/bin/test_config.yaml"
            
            # Read the original config
            with open(CONFIG_FILE, 'r') as f:
                config = yaml.safe_load(f)
                
            # Update the traffic_table path
            config['traffic_table_filename'] = traffic_file
            
            # Write to the test-specific config file
            with open(test_config_file, 'w') as f:
                yaml.dump(config, f)
                
            # Return the path to the new config file
            return test_config_file
        except Exception as e:
            print(f"Error creating test config file: {e}")
            return None

    def run_test(self, test_name):
        """Run a specific test case"""
        # Extract the test case
        traffic_rows, parameters = self.parser.extract_test_case(test_name)
        
        if not traffic_rows:
            print(f"Test case '{test_name}' not found.")
            return False
        
        # Write traffic to the temporary file
        with open(TEMP_TRAFFIC_FILE, 'w') as f:
            for row in traffic_rows:
                f.write(row + '\n')
                
        print(f"\nPrepared traffic file with {len(traffic_rows)} traffic rows.")
        
        # Create test-specific config file
        test_config_file = self.update_config_file(TEMP_TRAFFIC_FILE)
        if not test_config_file:
            print("Failed to create test config file.")
            return False
        
        # Set up simulation parameters - command line args override test parameters
        sim = self.args.sim or parameters.get('sim', DEFAULT_SIM_CYCLES)
        vc = self.args.vc or parameters.get('vc', DEFAULT_VC)
        compute_delay = self.args.compute_delay or parameters.get('compute_delay', DEFAULT_COMPUTE_DELAY)
        
        # Run noxim
        result_file = os.path.join(RESULTS_DIR, f"{test_name}.log")
        
        cmd = [
            NOXIM_PATH, 
            "-config", test_config_file,  # Use test-specific config
            "-sim", str(sim),
            "-vc", str(vc),
            "-compute_delay", str(compute_delay)
        ]
        
        print(f"\nExecuting: {' '.join(cmd)}")
        print(f"Output will be written to {result_file}")
        
        try:
            # Run the process with output redirected to file
            with open(result_file, 'w') as log_file:
                process = subprocess.Popen(
                    cmd, 
                    stdout=log_file,
                    stderr=subprocess.STDOUT
                )
                return_code = process.wait()
                
            if return_code != 0:
                print(f"\033[91mFAIL: Noxim exited with return code {return_code}\033[0m")
                return False
                
            print(f"Test '{test_name}' completed. Results saved to {result_file}")
            self.analyze_results(result_file, int(sim))
            
            return True
        except Exception as e:
            print(f"Error running Noxim: {e}")
            return False
    
    def analyze_results(self, result_file, sim_cycles):
        """Analyze test results for potential issues"""
        try:
            test_name = os.path.basename(result_file).replace('.log', '')
            print(f"\nAnalyzing results for test: {test_name}")
            
            with open(result_file, 'r') as f:
                lines = f.readlines()
            
            # Find the LAST line with completion message
            completion_index = -1
            for i, line in enumerate(lines):
                if "Noxim simulation completed" in line:
                    completion_index = i
            # No break statement, so we find the last occurrence
            
            if completion_index == -1:
                print("\n\033[91mWARNING: Simulation did not complete normally!\033[0m")
                return
                
            # Print the 5 lines before completion message
            print(f"\nLast few simulation lines from {result_file}:")
            start_index = max(0, completion_index - 5)
            for i in range(start_index, completion_index):
                print(f"  {lines[i].strip()}")
                
            # Check for cycle counts near max sim time (sim_cycles - 1)
            max_cycle_threshold = sim_cycles - 1
            for i in range(max(0, completion_index - 10), completion_index):
                line = lines[i].strip()
                parts = line.split()
                if parts and parts[0].isdigit():
                    cycle = int(parts[0])
                    if cycle >= max_cycle_threshold:
                        print(f"\n\033[93mWARNING: Simulation reached maximum cycle count ({cycle}/{sim_cycles})!\033[0m")
                        print("\033[93mTraffic pattern may not have completed properly.\033[0m")
                        print("\033[93mConsider increasing simulation time with -sim parameter.\033[0m")
                        break
                    
        except Exception as e:
            print(f"Error analyzing results: {e}")
    
    def run_all_tests(self):
        """Run all available test cases"""
        test_cases = self.parser.list_test_cases()
        
        if not test_cases:
            print("No test cases found. Import tests using --import option.")
            return
        
        print(f"Running {len(test_cases)} tests...")
        
        successful = 0
        failed = 0
        
        for test in test_cases:
            test_name = test['name']
            print(f"\n{'='*50}")
            print(f"Running test: {test_name}")
            print(f"Description: {test['description']}")
            print(f"{'='*50}")
            
            if self.run_test(test_name):
                successful += 1
            else:
                failed += 1
                
            if not self.args.auto:
                input("\nPress Enter to continue to the next test...")
        
        print(f"\nAll tests completed. Success: {successful}, Failed: {failed}")
    
    def import_test(self, source_file, test_name):
        """Import a test from a source file"""
        description = input("Enter a description for this test case: ")
        return self.parser.import_test_from_file(source_file, test_name, description)

def setup_test_cases_file():
    """Create the test cases file if it doesn't exist"""
    if not os.path.exists(TEST_CASES_FILE):
        with open(TEST_CASES_FILE, 'w') as f:
            f.write("""# Noxim Traffic Test Cases
# This file contains all traffic test cases for Noxim regression testing
# Format:
# ### BEGIN_TEST ###
# # Test Case: test_name
# # Description: Test description
# # Parameters: sim=5000 vc=1 compute_delay=10
# 
# traffic rows here...
# 
# ### END_TEST ###
""")
        print(f"Created new test cases file at {TEST_CASES_FILE}")

if __name__ == "__main__":
    # Create test cases file if it doesn't exist
    setup_test_cases_file()
    
    args = parse_args()
    tester = NoximTester(args)
    
    if args.list:
        tester.list_available_tests()
    elif args.import_file:
        source = "/home/hw/noxim_base/base_noxim/bin/o2m.txt" if args.import_file == "o2m" else args.import_file
        test_name = input("Enter a name for this test case: ")
        tester.import_test(source, test_name)
    elif args.test:
        tester.run_test(args.test)
    else:
        tester.run_all_tests()