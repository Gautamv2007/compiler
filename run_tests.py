import os
import subprocess
import glob

# --- Configuration ---
COMPILER_CMD = "./gvr.out"  # Make sure this matches your executable!
TEST_DIR = "tests"
COMPILER_DEFAULT_OUTPUT = "a.s"  # Default output file if compiler doesn't specify one

GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
RESET = '\033[0m'

def run_test(source_file):
    test_name = os.path.splitext(os.path.basename(source_file))[0]
    expected_file = os.path.join(TEST_DIR, f"{test_name}.expected")
    # --- NEW: Automatically look for a matching .input file ---
    input_file = os.path.join(TEST_DIR, f"{test_name}.input")
    
    if not os.path.exists(expected_file):
        return False, f"Missing {test_name}.expected file"

    with open(expected_file, 'r') as f:
        expected_output = f.read().strip()

    # Read input data if the file exists, otherwise None
    input_data = None
    if os.path.exists(input_file):
        with open(input_file, 'r') as f:
            input_data = f.read() # Read the whole thing (e.g., "7\n")

    asm_file = os.path.join(TEST_DIR, f"{test_name}.s")
    obj_file = os.path.join(TEST_DIR, f"{test_name}.o")
    exe_file = os.path.join(TEST_DIR, f"{test_name}")

    try:
        # 1. Run the compiler
        subprocess.run([COMPILER_CMD, source_file], check=True, stderr=subprocess.PIPE)

        # Move compiler output (adjust "out.s" to your compiler's output filename)
        if os.path.exists(COMPILER_DEFAULT_OUTPUT):
            os.rename(COMPILER_DEFAULT_OUTPUT, asm_file)
        
        if not os.path.exists(asm_file):
            return False, "Assembly file not generated."

        # 2. Assemble 
        subprocess.run(["as", "--32", asm_file, "-o", obj_file], check=True, stderr=subprocess.PIPE)

        # 3. Link 
        subprocess.run(["ld", "-m", "elf_i386", obj_file, "-o", exe_file], check=True, stderr=subprocess.PIPE)

        # 4. Run executable with Gneralized Input
        # subprocess.run handles 'input_data=None' perfectly (it just uses empty stdin)
        result = subprocess.run(
            [f"./{exe_file}"], 
            input=input_data, 
            capture_output=True, 
            text=True, 
            timeout=3
        )
        
        actual_output = result.stdout.strip()

        # 5. Compare
        if actual_output == expected_output:
            return True, ""
        else:
            return False, f"Expected:\n'{expected_output}'\nGot:\n'{actual_output}'"

    except subprocess.CalledProcessError as e:
        return False, f"Build failed: {e.stderr.decode().strip()}"
    except subprocess.TimeoutExpired:
        return False, "Timed out (Infinite loop?)"
    finally:
        # Cleanup
        for f in [asm_file, obj_file, exe_file]:
            if os.path.exists(f): os.remove(f)

def main():
    print("Starting Compiler Test Suite...\n")
    test_files = glob.glob(os.path.join(TEST_DIR, "*.txt"))
    
    if not test_files:
        print(f"No test files found in '{TEST_DIR}/'.")
        return

    passed, failed = 0, 0

    for source_file in sorted(test_files):
        test_name = os.path.basename(source_file)
        print(f"Testing {test_name:.<30} ", end="", flush=True)
        
        success, message = run_test(source_file)
        
        if success:
            print(f"{GREEN}[ PASS ]{RESET}")
            passed += 1
        else:
            print(f"{RED}[ FAIL ]{RESET}")
            print(f"{YELLOW}   -> {message}{RESET}")
            failed += 1

    print("\n" + "="*40)
    print(f"Test Run Complete: {passed} Passed, {failed} Failed.")
    print("="*40)

if __name__ == "__main__":
    main()