# C++ GNR/GSR Computation

Compute **General Necessary Reasons (GNR)** or **General Sufficient Reasons (GSR)** from an NNF log file. Intended to be used on general reason log files produced by CREX.

## Compilation (use O3 flag for best results)

```bash
g++ -std=c++17 -O3 gnr.cpp -o gnr
g++ -std=c++17 -O3 gsr.cpp -o gsr
```

## Usage

```bash
./gnr <input_nnf>
./gsr <input_nnf>
```

Results are written to the current directory in `gnrs_<filename>` or `gsrs_<filename>`.

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

Each output line after the header is one reason: pairs of `variable_num state_bitmask` (same encoding as leaf node lines in the input).
