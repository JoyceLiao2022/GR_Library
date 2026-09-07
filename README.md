# General Reasons Library

A reusable C++ library for computing General Sufficient Reasons (GSRs)
and General Necessary Reasons (GNRs) from NNF log files, with command-line
tools built on top of the same public API.

## Build 

### Requirements

- A C++17-compatible compiler
- CMake 3.16 or newer

Clone the repository and run the following commands from the repository root:

```bash
cmake -S . -B build
cmake --build build
```

### Release Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Usage (from the repository root)

```bash
./build/gnr <input_nnf>
./build/gsr <input_nnf>
```

Results are written to the current directory in `gnrs_<filename>` or `gsrs_<filename>`.

## Clean Rebuild

```bash
rm -rf build
cmake -S . -B build
cmake --build build
```

## Batch Testing Usage

```bash
./build/gnr <nnf_prefix> <n>
./build/gsr <nnf_prefix> <n>
```
Example: To run batch computation for 100 files on a banknote dataset model with 100 trees and depth 4, uncomment the batch processing lines in main(), locate the logs directory in CREX and run `./<gnr/gsr> <path/to/logs>/banknote_100_4_gr 99`

## Format example

**Input** (`toy.log`):

```
nnf 3 2 2
2 4
L 0 1
L 1 2
O 0 1
```

**GNR output** (`gnrs_toy.log`):

```
total 1
min_length 2
max_length 2
avg_length 2.00
0 1 1 2 
```

**GSR output** (`gsrs_toy.log`):

```
total 2
min_length 1
max_length 1
avg_length 1.00
0 1 
1 2 
```


**Input** (`tester1.log`):

```
nnf 3 2 2
2 2
L 0 1
L 1 2
A 0 1
```

**GNR output** (`gnrs_tester1.log`):

```
total 2
min_length 1
max_length 1
avg_length 1.00
0 1 
1 2 
```

**GSR output** (`gsrs_tester1.log`):

```
total 1
min_length 2
max_length 2
avg_length 2.00
0 1 1 2 
```


**Input** (`tester2.log`):

```
nnf 5 3 4
3 2 4
L 0 5
L 1 2
O 0 1
L 2 14
A 2 3
```

**GNR output** (`gnrs_tester2.log`):

```
total 2
min_length 1
max_length 2
avg_length 1.50
2 14 
0 5 1 2 
```

**GSR output** (`gsrs_tester2.log`):

```
total 2
min_length 2
max_length 2
avg_length 2.00
0 5 2 14 
1 2 2 14 
```

Each output line after the header is one reason: pairs of `variable_num state_bitmask` (same encoding as leaf node lines in the input).
