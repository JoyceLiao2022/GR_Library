#ifndef COMMON_NNF_H
#define COMMON_NNF_H

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <memory>
#include <string>
#include <sstream>
#include <fstream>
#include <functional>
#include <queue>
#include <chrono>
#include <numeric>
#include <stdexcept>
#include <iomanip>

// Optimized hardware functions
#define POPCOUNT(x) __builtin_popcountll(x)
#define CTZ(x) __builtin_ctzll(x)

// Global configuration variables 
inline int num_variables = 0;
inline int V_WORDS = 0; // number of 64-bit masks needed for storing unique variables
inline int S_WORDS = 0; // number of 64-bit masks needed for storing individual variable states
inline std::vector<int> global_cardinalities;

inline std::chrono::high_resolution_clock::time_point time_limit;

// Throws if the current time has passed the one-minute timeout threshold
inline void check_timeout() {
    if (std::chrono::high_resolution_clock::now() > time_limit) {
        throw std::runtime_error("TIMEOUT");
    }
}

// Convert a bitmask array to decimal state string
inline std::string bitmask_to_string(const uint64_t* mask, int num_bits) {
    if (num_bits <= 0) return "0";

    std::vector<uint32_t> digits;
    digits.push_back(0);

    for (int bit_idx = num_bits - 1; bit_idx >= 0; --bit_idx) {
        uint32_t carry = 0;
        for (size_t i = 0; i < digits.size(); ++i) {
            uint64_t cur = (uint64_t)digits[i] * 2ULL + carry;
            digits[i] = (uint32_t)(cur % 1000000000ULL);
            carry = (uint32_t)(cur / 1000000000ULL);
        }
        if (carry) digits.push_back(carry);

        if ((mask[bit_idx / 64] >> (bit_idx % 64)) & 1ULL) {
            carry = 1;
            for (size_t i = 0; i < digits.size() && carry; ++i) {
                uint64_t cur = (uint64_t)digits[i] + carry;
                digits[i] = (uint32_t)(cur % 1000000000ULL);
                carry = (uint32_t)(cur / 1000000000ULL);
            }
            if (carry) digits.push_back(carry);
        }
    }

    while (digits.size() > 1 && digits.back() == 0) digits.pop_back();
    if (digits.empty()) return "0";

    std::ostringstream oss;
    oss << digits.back();
    for (int i = (int)digits.size() - 2; i >= 0; --i) {
        oss << std::setw(9) << std::setfill('0') << digits[i];
    }
    return oss.str();
}

// Parse a decimal state string from a log file into a bitmask array
inline void set_bitmask_from_string(const std::string& str, uint64_t* mask_out, int W) {
    for (int i = 0; i < W; ++i) mask_out[i] = 0;
    std::vector<uint32_t> b;
    for (int i = (int)str.length(); i > 0; i -= 9) {
        int len = std::min(i, 9);
        b.push_back(std::stoul(str.substr(i - len, len)));
    }
    int bit_idx = 0;
    while (!b.empty()) {
        uint32_t rem = 0;
        for (int i = (int)b.size() - 1; i >= 0; --i) {
            uint64_t cur = (uint64_t)rem * 1000000000ULL + b[i];
            b[i] = cur / 2;
            rem = cur % 2;
        }
        if (rem && (bit_idx / 64) < W) {
            mask_out[bit_idx / 64] |= (1ULL << (bit_idx % 64));
        }
        bit_idx++;
        while (!b.empty() && b.back() == 0) b.pop_back();
    }
}

enum NodeType { LEAF, OR_NODE, AND_NODE, TRUE_NODE, FALSE_NODE };

struct Node {
    NodeType type;
    int node_id;
    int var_id;
    std::vector<uint64_t> states;
    
    std::vector<std::shared_ptr<Node>> children;
    
    // Tracking variables for GSR only
    std::vector<uint64_t> in_vars;
    std::vector<uint64_t> out_vars;
    std::vector<uint64_t> V_mask; 
    
    int in_degree = 0; 
    int cache_hits_remaining = 0;
    int processed_parents = 0;

    // Construct an NNF node with initialized state and variable-tracking vectors.
    Node(NodeType t, int n_id, int v_id = 0) 
        : type(t), node_id(n_id), var_id(v_id) {
        if (S_WORDS > 0) states.resize(S_WORDS, 0);
        if (V_WORDS > 0) {
            in_vars.resize(V_WORDS, 0);
            out_vars.resize(V_WORDS, 0);
            V_mask.resize(V_WORDS, 0);
        }
    }

    void addChild(std::shared_ptr<Node> child) {
        children.push_back(child);
        child->in_degree++;
    }
};

// Parser class for log files to NNF
class NNFParser {
public:
    // Parses an NNF log stream into a tree rooted at the last node.
    std::shared_ptr<Node> parse(std::istream& is) {
        std::string line;
        int n = 0, v = 0, e = 0;
        
        while (std::getline(is, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string token;
            ss >> token;
            if (token == "nnf") {
                ss >> n >> v >> e; 
                num_variables = v;
                V_WORDS = (num_variables + 63) / 64;
                break;
            }
        }

        global_cardinalities.clear();
        int max_card = 0;
        while (global_cardinalities.size() < v && std::getline(is, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            int c;
            while (ss >> c) {
                global_cardinalities.push_back(c);
                if (c > max_card) max_card = c;
            }
        }
        
        S_WORDS = (max_card + 63) / 64;
        if (S_WORDS == 0) S_WORDS = 1;

        std::vector<std::shared_ptr<Node>> nodes(n);
        for (int node_idx = 0; node_idx < n; ++node_idx) {
            while (std::getline(is, line) && line.empty()) {}
            std::stringstream ss(line);
            char type;
            ss >> type;

            if (type == 'L') {
                int var_idx;
                std::string states_str;
                ss >> var_idx >> states_str;
                nodes[node_idx] = std::make_shared<Node>(LEAF, node_idx, var_idx);
                set_bitmask_from_string(states_str, nodes[node_idx]->states.data(), S_WORDS);
            } 
            else if (type == 'A' || type == 'O') {
                nodes[node_idx] = std::make_shared<Node>(type == 'A' ? AND_NODE : OR_NODE, node_idx);
                int child_idx;
                while (ss >> child_idx) nodes[node_idx]->addChild(nodes[child_idx]);
            } 
            else if (type == 'T') nodes[node_idx] = std::make_shared<Node>(TRUE_NODE, node_idx);
            else if (type == 'F') nodes[node_idx] = std::make_shared<Node>(FALSE_NODE, node_idx);
        }

        nodes.back()->in_degree = 1; 
        return nodes.back(); 
    }
};

// Extract filename from a file path
inline std::string basename_from_path(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

// Result of processing a single NNF file (used by batch testing)
struct BatchProcessResult {
    long long reason_count = 0;
    long long total_length = 0;
    double time_ms = 0.0;
};

// Write reason statistics and reasons pairs to an output file
template<typename ReasonT, typename HasVarFn, typename VarStatesFn>
inline void write_reasons_file(
    const std::string& outpath,
    const std::vector<ReasonT>& reasons,
    HasVarFn has_var,
    VarStatesFn var_states)
{
    std::vector<int> lengths;
    lengths.reserve(reasons.size());
    long long total_len = 0;

    for (const auto& r : reasons) {
        int len = 0;
        for (int v = 0; v < num_variables; ++v) {
            if (has_var(r, v)) len++;
        }
        lengths.push_back(len);
        total_len += len;
    }

    int min_len = reasons.empty() ? 0 : *std::min_element(lengths.begin(), lengths.end());
    int max_len = reasons.empty() ? 0 : *std::max_element(lengths.begin(), lengths.end());
    double avg_len = reasons.empty() ? 0.0 : (double)total_len / (double)reasons.size();

    std::ofstream out(outpath);
    if (!out.is_open()) {
        throw std::runtime_error("Could not open output file: " + outpath);
    }

    out << "total " << reasons.size() << "\n";
    out << "min_length " << min_len << "\n";
    out << "max_length " << max_len << "\n";
    out << std::fixed << std::setprecision(2) << "avg_length " << avg_len << "\n";

    for (const auto& r : reasons) {
        for (int v = 0; v < num_variables; ++v) {
            if (has_var(r, v)) {
                int card = (v < (int)global_cardinalities.size()) ? global_cardinalities[v] : (S_WORDS * 64);
                out << v << " " << bitmask_to_string(var_states(r, v), card) << " ";
            }
        }
        out << "\n";
    }
}

// Run batch processing over prefix0.log through prefixN.log and prints summary stats
inline void run_batch_test(
    const std::string& prefix,
    int n,
    const std::string& label,
    const std::function<BatchProcessResult(const std::string& filename)>& process_file)
{
    long long total_reasons = 0;
    long long total_length = 0;
    double total_time_ms = 0.0;

    int valid_files = 0;
    int successful_runs = 0;
    int timeout_runs = 0;
    int error_runs = 0;

    std::vector<long long> reason_counts;

    std::cout << "Starting " << label << " batch computation for files: "
              << prefix << "0.log to " << prefix << n << ".log\n\n";

    for (int i = 0; i <= n; ++i) {
        std::string filename = prefix + std::to_string(i) + ".log";
        std::ifstream file_stream(filename);

        if (!file_stream.is_open()) {
            std::cerr << "  [!] Warning: Could not open file '" << filename << "'. Skipping." << std::endl;
            continue;
        }
        valid_files++;

        try {
            BatchProcessResult res = process_file(filename);
            total_reasons += res.reason_count;
            total_length += res.total_length;
            total_time_ms += res.time_ms;
            successful_runs++;
            reason_counts.push_back(res.reason_count);

            double current_avg_length = res.reason_count == 0
                ? 0.0
                : (double)res.total_length / (double)res.reason_count;

            std::cout << "  Processed " << filename << " -> " << label << "s: " << res.reason_count
                      << " | Avg Len: " << std::fixed << std::setprecision(2) << current_avg_length
                      << " | Time: " << res.time_ms << " ms" << std::endl;

        } catch (const std::runtime_error& e) {
            if (std::string(e.what()) == "TIMEOUT") {
                std::cout << "  [X] Timeout triggered for " << filename << " (> 1 min)" << std::endl;
                timeout_runs++;
            } else {
                std::cerr << "  [!] Error in " << filename << ": " << e.what() << std::endl;
                error_runs++;
            }
        } catch (const std::exception& e) {
            std::cerr << "  [!] Parse Error in " << filename << ": " << e.what() << std::endl;
            error_runs++;
        }
    }

    std::cout << "\n--- Batch Summary (" << label << ") ---\n";
    if (valid_files > 0) {
        double pass_percent = ((double)successful_runs / valid_files) * 100.0;
        std::cout << "Files targeted               : " << valid_files << "\n";
        std::cout << "Successfully processed       : " << successful_runs << "\n";
        std::cout << "Timed out (> 1 min)          : " << timeout_runs << "\n";
        std::cout << "Failed with errors           : " << error_runs << "\n";
        std::cout << "Pass Rate (Success / Total)  : " << std::fixed << std::setprecision(2) << pass_percent << "%\n";

        if (successful_runs > 0) {
            std::sort(reason_counts.begin(), reason_counts.end());

            long long max_reasons = reason_counts.back();
            double median_reasons = 0.0;
            size_t count_size = reason_counts.size();

            if (count_size % 2 == 0) {
                median_reasons = (reason_counts[count_size / 2 - 1] + reason_counts[count_size / 2]) / 2.0;
            } else {
                median_reasons = reason_counts[count_size / 2];
            }

            double global_avg_length = total_reasons > 0 ? (double)total_length / total_reasons : 0.0;

            std::cout << "--------------------------------\n";
            std::cout << "Average " << label << "s per instance    : " << (double)total_reasons / successful_runs << "\n";
            std::cout << "Median " << label << "s per instance     : " << median_reasons << "\n";
            std::cout << "Max " << label << "s in an instance      : " << max_reasons << "\n";
            std::cout << "Average length of a " << label << "      : " << global_avg_length << " vars\n";
            std::cout << "Average time per instance    : " << total_time_ms / successful_runs << " ms\n";
        }
    } else {
        std::cout << "No valid log files were found to process.\n";
    }
}

#endif // COMMON_NNF_H