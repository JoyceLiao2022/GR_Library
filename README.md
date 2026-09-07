# General Reasons Library

A reusable C++ library for computing General Sufficient Reasons (GSRs)
and General Necessary Reasons (GNRs) from NNF log files, with command-line
tools built on top of the same public API.

## Build 

The project requires a C++17-compatible compiler.

Run all commands from the repository root

### Compile the library sources 

```bash
g++ -std=c++17 -O3 -Iinclude -c src/gsr.cpp -o gsr.o
g++ -std=c++17 -O3 -Iinclude -c src/gnr.cpp -o gnr.o
```

### Create the static library

```bash
ar rcs libgr.a gsr.o gnr.o
```

### Build the CLI exe

```bash
g++ -std=c++17 -O3 -Iinclude tools/gsr_main.cpp -L. -lgr -o gsr
g++ -std=c++17 -O3 -Iinclude tools/gnr_main.cpp -L. -lgr -o gnr
```

### Usage (in the same folder as gsr or gnr executable)

```bash
./gnr <input_nnf>
./gsr <input_nnf>
```

Results are written to the current directory in `gnrs_<filename>` or `gsrs_<filename>`.

## Cleanup

```bash
rm -f gsr.o gnr.o libgr.a gsr gnr gsr.exe gnr.exe
```

## Batch Testing Usage

```bash
./gnr <nnf_prefix> <n>
./gsr <nnf_prefix> <n>
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
