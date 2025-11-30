import argparse
import subprocess
import re
import matplotlib.pyplot as plt


def run_sim(sim_path, circuit_file, num_vectors):
    cmd = [sim_path, "-f", circuit_file, "-m", "1", "-n", str(num_vectors)]
    result = subprocess.run(cmd, capture_output=True, text=True)
    output = result.stdout

    match = re.search(r"(\d+)\s+total fault nets found", output)
    if match:
        faults_found = int(match.group(1))
        print(f"[+] {num_vectors} random tests -> {faults_found} detected faults")
        return faults_found
    else:
        print(f"[!] Warning: Cannot parse result for {num_vectors} tests")
        print(output)
        return 0


def main():
    # ---------- Argument Parser ----------
    parser = argparse.ArgumentParser(
        description="Run Deductive Fault Simulator and plot fault coverage curve."
    )

    parser.add_argument(
        "-f", "--file",
        default="../files/s27.txt",
        help="Circuit file path (default: ../files/s27.txt)"
    )
    parser.add_argument(
        "-p", "--sim_path",
        default="./sim2",
        help="Path to simulator executable (default: ./sim2)"
    )
    parser.add_argument(
        "-m", "--max_vectors",
        type=int,
        default=50,
        help="Maximum number of random test vectors to apply (default: 50)"
    )
    parser.add_argument(
        "-s", "--step",
        type=int,
        default=1,
        help="Step size of vector count (default: 1)"
    )
    parser.add_argument(
        "-t", "--total_faults",
        type=int,
        default=None,
        help="Total number of possible faults (if known). If not set, "
             "it will be determined automatically using max_vectors run."
    )
    parser.add_argument(
        "-o", "--output",
        default=None,
        help="Save plot to file (e.g., coverage.png). If not set, show interactively."
    )

    args = parser.parse_args()

    sim_path = args.sim_path
    circuit_file = args.file
    max_vectors = args.max_vectors
    step = args.step
    total_faults = args.total_faults
    output_file = args.output

    # ---------- Main Logic ----------
    num_vectors_list = list(range(1, max_vectors + 1, step))
    detected_faults = []

    # if total_faults isn't defined，run the max_vectors to get the total faults
    if total_faults is None:
        print("=== Establishing total fault count ===")
        total_faults = run_sim(sim_path, circuit_file, max_vectors)
        if total_faults == 0:
            print("[Error] Could not determine total faults. Check simulator output.")
            return
    else:
        print(f"[Info] Using user-provided total_faults = {total_faults}")

    print("\n=== Running coverage tests ===")
    for n in num_vectors_list:
        faults = run_sim(sim_path, circuit_file, n)
        detected_faults.append(faults)

    coverage = [f / total_faults * 100 for f in detected_faults]

    # ---------- Plot ----------
    plt.figure(figsize=(8, 5))
    plt.plot(num_vectors_list, coverage, marker='o')
    plt.title("Fault Coverage vs Number of Random Test Vectors")
    plt.xlabel("Number of Random Test Vectors")
    plt.ylabel("Fault Coverage (%)")
    plt.grid(True)
    plt.tight_layout()

    if output_file:
        plt.savefig(output_file, dpi=300)
        print(f"[+] Plot saved to {output_file}")
    else:
        plt.show()

    # ---------- Coverage Summary ----------
    cov75 = next((n for n, c in zip(num_vectors_list, coverage) if c >= 75), None)
    cov90 = next((n for n, c in zip(num_vectors_list, coverage) if c >= 90), None)
    print("\n--- Coverage Summary ---")
    print(f"Total possible faults: {total_faults}")
    print(f"Vectors for >75%% coverage: {cov75}")
    print(f"Vectors for >90%% coverage: {cov90}")


if __name__ == "__main__":
    main()
