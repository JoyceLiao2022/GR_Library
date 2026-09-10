# General Reasons Library

A reusable C++ library for computing General Sufficient Reasons (GSRs)
and General Necessary Reasons (GNRs) from NNF log files, with command-line
tools and Python bindings built on top of the same public API.

## Build 

### Requirements

- A C++17-compatible compiler
  - Windows: Visual Studio 2022 / MSVC
  - macOS: Apple Clang (Xcode Command Line Tools)
  - Linux: GCC or Clang
- CMake 3.16 or newer
- Python 3 
- Git
- Internet access during the initial CMake configuration

#### Windows

The recommended Windows toolchain is Visual Studio 2022/MSVC
with native Windows Python.

Clone the repository and run the following commands from the repository root:

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

#### macOS / Linux

Clone the repository and run the following commands from the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Usage (from the repository root)

#### Windows

```bash
./build/Release/gnr.exe <input_nnf>
./build/Release/gsr.exe <input_nnf>
```

Results are written to the current directory in `gnrs_<filename>` or `gsrs_<filename>`.

#### macOS / Linux

```bash
./build/gnr <input_nnf>
./build/gsr <input_nnf>
```

## Clean Rebuild

### Windows

```bash
rm -rf build
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### macOS / Linux

```bash
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```


## C++ API

The library exposes a small public C++ API through:

```cpp
#include <gr/gr.h>
```

The two main functions are:

```cpp
gr::ReasonSet gr::compute_gsrs_from_file(const std::string& filepath);
gr::ReasonSet gr::compute_gnrs_from_file(const std::string& filepath);
```

Both functions take the path to an NNF log file and return a `gr::ReasonSet`.

A `ReasonSet` is a collection of reasons, where each reason contains one or more `Assignment` objects. Each assignment exposes:

```cpp
assignment.variable
assignment.states
```

* `variable` is the variable index.
* `states` is the state bitmask represented as a decimal string.

### Example

```cpp
#include <gr/gr.h>

#include <iostream>

int main()
{
    auto gsrs = gr::compute_gsrs_from_file("tests/data/toy.log");
    auto gnrs = gr::compute_gnrs_from_file("tests/data/toy.log");

    std::cout << "GSR count: " << gsrs.size() << '\n';
    std::cout << "GNR count: " << gnrs.size() << '\n';

    for (const auto& reason : gsrs)
    {
        for (const auto& assignment : reason)
        {
            std::cout << assignment.variable
                      << " "
                      << assignment.states
                      << " ";
        }

        std::cout << '\n';
    }

    return 0;
}
```

For `tests/data/toy.log`, the expected reason counts are:

```text
GSR count: 2
GNR count: 1
```

See `examples/example.cpp` for a complete C++ example.

---

## Python API

The project also provides Python bindings through `pybind11`.

The Python extension module is named:

```python
gr_python
```

The Python API exposes the same two computations as the C++ API:

```python
gr_python.compute_gsrs_from_file(filepath)
gr_python.compute_gnrs_from_file(filepath)
```

Both functions take the path to an NNF log file and return a nested Python list with the conceptual structure:

```text
list[
    list[
        Assignment
    ]
]
```

Each `Assignment` object exposes:

```python
assignment.variable
assignment.states
```

* `variable` is the variable index.
* `states` is the state bitmask represented as a decimal string.

### Example

```python
import gr_python

gsrs = gr_python.compute_gsrs_from_file("tests/data/toy.log")
gnrs = gr_python.compute_gnrs_from_file("tests/data/toy.log")

print("GSR count:", len(gsrs))
print("GNR count:", len(gnrs))

print("GSRs:")

for reason in gsrs:
    print([(assignment.variable, assignment.states)
           for assignment in reason])

print("GNRs:")

for reason in gnrs:
    print([(assignment.variable, assignment.states)
           for assignment in reason])
```

For `tests/data/toy.log`, the expected reason counts are:

```text
GSR count: 2
GNR count: 1
```

### Running the Python Example

The Python extension is currently built inside the CMake build directory rather than installed as a Python package.

From the repository root, run the example using the appropriate command for your platform.

#### Windows

When using the Visual Studio Release build:

```bash
PYTHONPATH=build/Release py examples/example.py
```

#### macOS

```bash
PYTHONPATH=build python3 examples/example.py
```

#### Linux

```bash
PYTHONPATH=build python3 examples/example.py
```

See `examples/example.py` for a complete Python example.


## Testing

After building the project, tests can be run either through CTest or individually as smoke tests.

### Automated Tests with CTest

CTest runs the tests registered by the CMake build.

#### Windows

From the repository root using Git Bash:

```bash
ctest --test-dir build -C Release --output-on-failure
```

#### macOS / Linux

From the repository root:

```bash
ctest --test-dir build --output-on-failure
```

A successful run should end with output similar to:

```text
100% tests passed, 0 tests failed
```

If a test fails, `--output-on-failure` prints the corresponding error output.

---

### C++ API Smoke Test

The C++ smoke test calls the public C++ API directly using `tests/data/toy.log`.

#### Windows

```bash
./build/Release/api_smoke.exe tests/data/toy.log
```

#### macOS / Linux

```bash
./build/api_smoke tests/data/toy.log
```

For `toy.log`, the expected result is:

```text
GSR count: 2
GNR count: 1
```

If the smoke test uses assertions rather than printing the counts directly, it should complete successfully with no failed checks.

---

### Python API Smoke Test

The Python smoke test verifies the Python bindings against the known expected results stored under `tests/baseline/`.

Because the Python extension is built inside the CMake build directory rather than installed as a package, the build directory must be added to `PYTHONPATH`.

#### Windows

From the repository root using Git Bash:

```bash
PYTHONPATH="$PWD/build/Release" py tests/python_smoke.py
```

#### macOS

```bash
PYTHONPATH="$PWD/build" python3 tests/python_smoke.py
```

#### Linux

```bash
PYTHONPATH="$PWD/build" python3 tests/python_smoke.py
```

The Python smoke test checks the available test inputs, including `toy.log`, against their saved baseline results.

A successful run should end with output similar to:

```text
Testing tests/data/toy.log...
  passed

All Python API tests passed.
```

If additional tester files are included, each should also report:

```text
passed
```

For `toy.log`, the expected result is equivalent to:

```text
GSR count: 2
GNR count: 1
```

with GSRs:

```text
[(0, "1")]
[(1, "2")]
```

and GNR:

```text
[(0, "1"), (1, "2")]
```

---

### Test Data

Test inputs and their known expected outputs are stored under:

```text
tests/
├── data/
└── baseline/
```

The smoke tests verify that the C++ and Python APIs continue to reproduce these expected results.


## Input Format

The GSR and GNR APIs take an NNF log file as input.

For example, `toy.log`:

```text
nnf 3 2 2
2 4
L 0 1
L 1 2
O 0 1
```

Leaf-node lines use the form:

```text
L <variable_num> <state_bitmask>
```

where:

* `variable_num` identifies the variable;
* `state_bitmask` encodes the corresponding states.

The same format is accepted by:

```cpp
gr::compute_gsrs_from_file(...)
gr::compute_gnrs_from_file(...)
```

and:

```python
gr_python.compute_gsrs_from_file(...)
gr_python.compute_gnrs_from_file(...)
```

Additional test inputs and expected results are available under the `tests/` directory.
