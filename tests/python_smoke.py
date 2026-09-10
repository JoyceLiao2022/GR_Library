import sys
from pathlib import Path

import gr_python


def normalize_python(reason_set):
    """Convert gr_python ReasonSet into a comparable Python structure."""
    return sorted(
        sorted(
            (assignment.variable, assignment.states)
            for assignment in reason
        )
        for reason in reason_set
    )


def read_baseline(path):
    """
    Read a GSR/GNR baseline output file.

    Expected format:

        total ...
        min_length ...
        max_length ...
        avg_length ...
        0 1
        1 2
        ...

    Each reason line contains:
        variable state variable state ...
    """
    reasons = []

    with open(path, "r") as f:
        lines = f.readlines()

    # Skip the four statistics/header lines
    for line in lines[4:]:
        tokens = line.split()

        if not tokens:
            continue

        if len(tokens) % 2 != 0:
            raise ValueError(
                f"Invalid reason line in {path}: {line.strip()}"
            )

        reason = []

        for i in range(0, len(tokens), 2):
            variable = int(tokens[i])
            states = tokens[i + 1]
            reason.append((variable, states))

        reasons.append(reason)

    return sorted(sorted(reason) for reason in reasons)


def test_case(input_path, gsr_baseline, gnr_baseline):
    print(f"Testing {input_path}...")

    # Compute through Python → pybind11 → C++ API
    gsrs = gr_python.compute_gsrs_from_file(str(input_path))
    gnrs = gr_python.compute_gnrs_from_file(str(input_path))

    actual_gsrs = normalize_python(gsrs)
    actual_gnrs = normalize_python(gnrs)

    expected_gsrs = read_baseline(gsr_baseline)
    expected_gnrs = read_baseline(gnr_baseline)

    assert actual_gsrs == expected_gsrs, (
        f"GSR mismatch for {input_path}\n"
        f"Expected: {expected_gsrs}\n"
        f"Actual:   {actual_gsrs}"
    )

    assert actual_gnrs == expected_gnrs, (
        f"GNR mismatch for {input_path}\n"
        f"Expected: {expected_gnrs}\n"
        f"Actual:   {actual_gnrs}"
    )

    print("  passed")


def main():
    root = Path(__file__).resolve().parent.parent

    data = root / "tests" / "data"
    baseline = root / "tests" / "baseline"

    test_case(
        data / "toy.log",
        baseline / "gsrs_toy.log",
        baseline / "gnrs_toy.log",
    )

    test_case(
        data / "tester1.log",
        baseline / "gsrs_tester1.log",
        baseline / "gnrs_tester1.log",
    )

    test_case(
        data / "tester2.log",
        baseline / "gsrs_tester2.log",
        baseline / "gnrs_tester2.log",
    )

    print("\nAll Python API tests passed.")


if __name__ == "__main__":
    main()