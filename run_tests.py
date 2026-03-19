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
    
    if not os.path.exists(expected_file):
        return False, f"Missing {test_name}.expected file"

    with open(expected_file, 'r') as f:
        expected_output = f.read().strip()

    asm_file = os.path.join(TEST_DIR, f"{test_name}.s")
    obj_file = os.path.join(TEST_DIR, f"{test_name}.o")
    exe_file = os.path.join(TEST_DIR, f"{test_name}")

    try:
        # 1. Run the compiler (we no longer try to capture the terminal output)
        subprocess.run([COMPILER_CMD, source_file], check=True, stderr=subprocess.PIPE)

        # --- NEW: Move the file your compiler created into the tests directory ---
        if os.path.exists(COMPILER_DEFAULT_OUTPUT):
            # Move and rename 'out.s' (or whatever it is) to 'tests/math.s'
            os.rename(COMPILER_DEFAULT_OUTPUT, asm_file)
        
        # Safety Check
        if not os.path.exists(asm_file) or os.path.getsize(asm_file) == 0:
            return False, f"Could not find the generated assembly. Did it create '{COMPILER_DEFAULT_OUTPUT}'?"

        # 2. Assemble 
        subprocess.run(["as", "--32", asm_file, "-o", obj_file], check=True, stderr=subprocess.PIPE)

        # 3. Link 
        subprocess.run(["ld", "-m", "elf_i386", obj_file, "-o", exe_file], check=True, stderr=subprocess.PIPE)

        # --- NEW SAFETY CHECK ---
        if not os.path.exists(exe_file) or os.path.getsize(exe_file) == 0:
            return False, "Executable file was not created properly."

        # 4. Run executable
        result = subprocess.run([f"./{exe_file}"], capture_output=True, text=True, timeout=2)
        actual_output = result.stdout.strip()

        # 5. Compare
        if actual_output == expected_output:
            return True, ""
        else:
            return False, f"Expected:\n'{expected_output}'\nGot:\n'{actual_output}'"

    except subprocess.CalledProcessError as e:
        return False, f"Build step failed: {e.stderr.decode().strip()}"
    except subprocess.TimeoutExpired:
        return False, "Execution timed out (Infinite loop in assembly?)"
    except OSError as e:
        return False, f"OS Error (File might be corrupted): {str(e)}"
    finally:
        # Cleanup
        for f in [asm_file, obj_file, exe_file]:
            if os.path.exists(f):
                os.remove(f)

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
    print(f"🏁 Test Run Complete: {passed} Passed, {failed} Failed.")
    print("="*40)

if __name__ == "__main__":
    main()