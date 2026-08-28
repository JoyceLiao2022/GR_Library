#include "common.h"

// GNR-specific definitions
std::vector<std::vector<uint64_t>> GLOBAL_FULL_MASKS;

// Precompute full bitmasks for each variable (to test for tautologies)
void init_global_masks() {
    GLOBAL_FULL_MASKS.assign(num_variables, std::vector<uint64_t>(S_WORDS, 0));
    for (int v = 0; v < num_variables; ++v) {
        int card = (v < global_cardinalities.size()) ? global_cardinalities[v] : (S_WORDS * 64);
        for(int w = 0; w < S_WORDS; ++w) {
            uint64_t expected = 0;
            if (card >= (w + 1) * 64) expected = ~0ULL;
            else if (card > w * 64) expected = (1ULL << (card % 64)) - 1;
            GLOBAL_FULL_MASKS[v][w] = expected;
        }
    }
}

// GNR-specific Reason struct
struct Reason {
    std::vector<uint64_t> data; 
    int popcount = 0;

    // Allocates storage for the variable mask and per-variable state bitmasks.
    Reason() {
        if (num_variables > 0) {
            data.resize(V_WORDS + num_variables * S_WORDS, 0);
        }
    }

    inline uint64_t* var_mask() { return data.data(); }
    inline const uint64_t* var_mask() const { return data.data(); }

    inline uint64_t* states() { return data.data() + V_WORDS; }
    inline const uint64_t* states() const { return data.data() + V_WORDS; }

    inline bool has_var(int v) const { return (var_mask()[v / 64] >> (v % 64)) & 1; }
    inline void set_var(int v) { var_mask()[v / 64] |= (1ULL << (v % 64)); }
    inline void clear_var(int v) { var_mask()[v / 64] &= ~(1ULL << (v % 64)); }

    // Recompute number of variables present in this reason
    inline void update_popcount() {
        int count = 0;
        for (int w = 0; w < V_WORDS; ++w) {
            uint64_t m = var_mask()[w];
            count += POPCOUNT(m);
        }
        popcount = count;
    }

    // Compare two reasons lexicographically
    bool operator<(const Reason& o) const {
        if (popcount != o.popcount) return popcount < o.popcount;
        for (int w = 0; w < V_WORDS; ++w) {
            if (var_mask()[w] != o.var_mask()[w]) return var_mask()[w] < o.var_mask()[w];
        }
        size_t total_s = num_variables * S_WORDS;
        for (size_t w = 0; w < total_s; ++w) {
            if (states()[w] != o.states()[w]) return states()[w] < o.states()[w];
        }
        return false;
    }

    // Determine if two reasons are identical
    bool operator==(const Reason& o) const {
        if (popcount != o.popcount) return false;
        for (int w = 0; w < V_WORDS; ++w) if (var_mask()[w] != o.var_mask()[w]) return false;
        size_t total_s = num_variables * S_WORDS;
        for (size_t w = 0; w < total_s; ++w) if (states()[w] != o.states()[w]) return false;
        return true;
    }
};

using ReasonSet = std::vector<Reason>;
std::map<Node*, ReasonSet> global_cache;

// Returns true if variable v's states equate to true (a tautology)
inline bool is_full_mask(const Reason& r, int v) {
    int offset = v * S_WORDS;
    for(int w = 0; w < S_WORDS; ++w) {
        if ((r.states()[offset + w] & GLOBAL_FULL_MASKS[v][w]) != GLOBAL_FULL_MASKS[v][w]) return false;
    }
    return true;
}

// Determine if c1 subsumes c2
inline bool subsumes(const Reason& c1, const Reason& c2) {
    if (c1.popcount > c2.popcount) return false;
    for(int w = 0; w < V_WORDS; ++w) {
        if ((c1.var_mask()[w] & c2.var_mask()[w]) != c1.var_mask()[w]) return false;
    }
    size_t total_s = num_variables * S_WORDS;
    for (size_t w = 0; w < total_s; ++w) {
        if ((c2.states()[w] & c1.states()[w]) != c1.states()[w]) return false;
    }
    return true;
}

// Determine if a clause is subsumed by any other clause in the set
inline bool is_subsumed(const Reason& c, const ReasonSet& clauses) {
    for (const Reason& t : clauses) {
        if (subsumes(t, c)) return true;
    }
    return false;
}

// Remove all clauses subsumed by c from the set other than itself
void remove_subsumed(const Reason& c, ReasonSet& clauses) {
    size_t write_idx = 0;
    for (size_t read_idx = 0; read_idx < clauses.size(); ++read_idx) {
        if (&clauses[read_idx] == &c || !subsumes(c, clauses[read_idx])) {
            if (write_idx != read_idx) {
                clauses[write_idx] = std::move(clauses[read_idx]);
            }
            write_idx++;
        }
    }
    clauses.resize(write_idx);
}

// Sort, deduplicate, and remove subsumed reasons from a ReasonSet
ReasonSet batch_subsume(ReasonSet& S) {
    if (S.empty()) return S;
    
    std::sort(S.begin(), S.end()); 
    S.erase(std::unique(S.begin(), S.end()), S.end());
    
    ReasonSet result;
    result.reserve(S.size());
    int iter_count = 0;
    for (size_t i = 0; i < S.size(); ++i) {
        if (++iter_count % 128 == 0) check_timeout();

        bool subsumed = false;
        for (const Reason& t : result) {
            if (subsumes(t, S[i])) { subsumed = true; break; }
        }
        if (!subsumed) result.push_back(std::move(S[i]));
    }
    return result;
}

// Cross (Cartesian) product of two reason sets connected by an OR node
ReasonSet cross_product(const ReasonSet& setA, const ReasonSet& setB) {
    if (setA.empty() || setB.empty()) return ReasonSet();
    ReasonSet buffer;
    buffer.reserve(setA.size() * setB.size());

    int iter_count = 0;
    for (const auto& rA : setA) {
        if (++iter_count % 32 == 0) check_timeout();

        for (const auto& rB : setB) {
            Reason new_reason;
            bool tautology = false;
            
            for(int w = 0; w < V_WORDS; ++w) new_reason.var_mask()[w] = rA.var_mask()[w] | rB.var_mask()[w];
            new_reason.update_popcount();
            
            size_t total_s = num_variables * S_WORDS;
            for(size_t w = 0; w < total_s; ++w) {
                new_reason.states()[w] = rA.states()[w] | rB.states()[w];
            }
            
            for (int v = 0; v < num_variables; ++v) {
                if (new_reason.has_var(v) && is_full_mask(new_reason, v)) {
                    tautology = true; break;
                }
            }
            
            if (!tautology) buffer.push_back(std::move(new_reason));
        }
    }
    return batch_subsume(buffer);
}

// Recursively convert an NNF subtree into a set of clauses (CNF)
ReasonSet NNF_to_CNF(Node* delta) {
    check_timeout();

    if (global_cache.count(delta)) {
        ReasonSet res = global_cache[delta];
        delta->cache_hits_remaining--;
        if (delta->cache_hits_remaining <= 0) global_cache.erase(delta);
        return res;
    }

    ReasonSet cnf;

    if (delta->type == LEAF) {
        Reason r;
        r.set_var(delta->var_id);
        int offset = delta->var_id * S_WORDS;
        for (int w = 0; w < S_WORDS; ++w) r.states()[offset + w] = delta->states[w];
        r.update_popcount();

        if (!is_full_mask(r, delta->var_id)) {
            bool is_false = true;
            for (int w = 0; w < S_WORDS; ++w) if (r.states()[offset + w] != 0) is_false = false;
            if (is_false) cnf.push_back(Reason()); 
            else cnf.push_back(r);
        }
    }
    else if (delta->type == TRUE_NODE) { }
    else if (delta->type == FALSE_NODE) {
        cnf.push_back(Reason()); 
    }
    else if (delta->type == AND_NODE) {
        for (auto& child : delta->children) {
            ReasonSet child_cnf = NNF_to_CNF(child.get());
            cnf.insert(cnf.end(), std::make_move_iterator(child_cnf.begin()), std::make_move_iterator(child_cnf.end()));
        }
        cnf = batch_subsume(cnf);
    }
    else if (delta->type == OR_NODE) {
        if (!delta->children.empty()) {
            std::vector<ReasonSet> child_cnfs;
            for (auto& child : delta->children) child_cnfs.push_back(NNF_to_CNF(child.get()));
            
            std::sort(child_cnfs.begin(), child_cnfs.end(), [](const ReasonSet& a, const ReasonSet& b){ return a.size() < b.size(); });
            cnf = std::move(child_cnfs[0]);
            for (size_t i = 1; i < child_cnfs.size(); ++i) cnf = cross_product(cnf, child_cnfs[i]);
        }
    }

    delta->cache_hits_remaining--;
    if (delta->cache_hits_remaining > 0) global_cache[delta] = cnf;
    return cnf;
}

// Resolves two clauses on a given variable
bool resolve(const Reason& c1, const Reason& c2, int var, Reason& out_res) {
    int offset = var * S_WORDS;
    
    bool s1_sub_s2 = true;
    bool s2_sub_s1 = true;
    for(int w = 0; w < S_WORDS; ++w) {
        uint64_t s1 = c1.states()[offset + w];
        uint64_t s2 = c2.states()[offset + w];
        if ((s1 & s2) != s1) s1_sub_s2 = false;
        if ((s1 & s2) != s2) s2_sub_s1 = false;
    }
    if (s1_sub_s2 || s2_sub_s1) return false;

    for(int w = 0; w < V_WORDS; ++w) out_res.var_mask()[w] = c1.var_mask()[w] | c2.var_mask()[w];
    
    size_t total_s = num_variables * S_WORDS;
    for(size_t w = 0; w < total_s; ++w) {
        out_res.states()[w] = c1.states()[w] | c2.states()[w];
    }

    bool any_overlap = false;
    for(int w = 0; w < S_WORDS; ++w) {
        uint64_t s_int = c1.states()[offset + w] & c2.states()[offset + w];
        out_res.states()[offset + w] = s_int;
        if (s_int != 0) any_overlap = true;
    }
    if (!any_overlap) out_res.clear_var(var);
    
    out_res.update_popcount();
    
    for (int u = 0; u < num_variables; ++u) {
        if (u != var && out_res.has_var(u)) {
            if (is_full_mask(out_res, u)) return false; 
        }
    }
    return true;
}

// Computes GNRs from a CNF via resolution
ReasonSet compute_GNRs(ReasonSet cnf) {
    std::sort(cnf.begin(), cnf.end());

    std::vector<std::vector<uint64_t>> vars_list;
    std::vector<ReasonSet> list_of_clauses;

    // filter clauses for supersets/duplicates
    for (const Reason& clause : cnf) {
        bool skip = false;
        for (size_t i = 0; i < vars_list.size(); ++i) {
            bool exact_match = true, is_superset = true, is_strict = false;
            for(int w = 0; w < V_WORDS; ++w) {
                if (vars_list[i][w] != clause.var_mask()[w]) exact_match = false;
                if ((vars_list[i][w] & clause.var_mask()[w]) != vars_list[i][w]) is_superset = false;
                if (vars_list[i][w] != clause.var_mask()[w]) is_strict = true;
            }
            if (exact_match) { list_of_clauses[i].push_back(clause); skip = true; break; }
            if (is_superset && is_strict) { skip = true; break; }
        }
        if (!skip) {
            vars_list.push_back(std::vector<uint64_t>(clause.var_mask(), clause.var_mask() + V_WORDS));
            list_of_clauses.push_back({clause});
        }
    }

    ReasonSet var_min_GNRs;
    int iter_count = 0;

    // resolution
    for (size_t i = 0; i < list_of_clauses.size(); ++i) {
        ReasonSet clauses = std::move(list_of_clauses[i]);
        const std::vector<uint64_t>& group_vars = vars_list[i];

        for (int var = 0; var < num_variables; ++var) {
            if (!((group_vars[var / 64] >> (var % 64)) & 1)) continue;

            ReasonSet consensus_set;
            ReasonSet nonsubsumed_clauses = clauses;

            for (const Reason& c1 : clauses) {
                if (++iter_count % 32 == 0) check_timeout();

                // ensure c1 wasn't already removed by a prior resolution in this loop
                bool c1_is_active = false;
                for (const auto& r : nonsubsumed_clauses) {
                    if (r == c1) { c1_is_active = true; break; }
                }
                if (!c1_is_active) continue;

                ReasonSet consensus_list = consensus_set; 
                bool c1_subsumed = false;

                for (const Reason& c2 : consensus_list) {
                    
                    // ensure c2 wasn't already removed by a prior resolution in this loop
                    bool c2_is_active = false;
                    for (const auto& r : nonsubsumed_clauses) {
                        if (r == c2) { c2_is_active = true; break; }
                    }
                    if (!c2_is_active) continue;

                    Reason t;
                    if (!resolve(c1, c2, var, t)) continue;

                    if (is_subsumed(t, nonsubsumed_clauses)) continue;

                    remove_subsumed(t, consensus_set);
                    remove_subsumed(t, nonsubsumed_clauses);

                    consensus_set.push_back(t);
                    nonsubsumed_clauses.push_back(t);

                    // verify if c1 was killed by t
                    bool c1_still_exists = false;
                    for (const auto& r : nonsubsumed_clauses) {
                        if (r == c1) { c1_still_exists = true; break; }
                    }
                    if (!c1_still_exists) {
                        c1_subsumed = true;
                        c1_is_active = false;
                    }
                }
                
                if (!c1_subsumed && c1_is_active) consensus_set.push_back(c1);
            }
            clauses = std::move(nonsubsumed_clauses);
        }
        var_min_GNRs.insert(var_min_GNRs.end(), clauses.begin(), clauses.end());
    }
    
    std::sort(var_min_GNRs.begin(), var_min_GNRs.end());
    var_min_GNRs.erase(std::unique(var_min_GNRs.begin(), var_min_GNRs.end()), var_min_GNRs.end());
    
    return var_min_GNRs;
}

ReasonSet compute_gnrs_from_file(const std::string& filepath) {
    std::ifstream file_stream(filepath);
    if (!file_stream.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath);
    }

    NNFParser parser;
    auto root = parser.parse(file_stream);

    init_global_masks();

    std::function<void(Node*)> init_in_degree = [&](Node* node) {
        node->cache_hits_remaining = node->in_degree;
        for (auto& c : node->children) init_in_degree(c.get());
    };
    init_in_degree(root.get());

    auto start_time = std::chrono::high_resolution_clock::now();
    time_limit = start_time + std::chrono::minutes(1); // 1 minute timeout; change as needed

    global_cache.clear();
    ReasonSet cnf = NNF_to_CNF(root.get());
    return compute_GNRs(std::move(cnf));
}

BatchProcessResult process_gnr_file(const std::string& filename) {
    auto start_time = std::chrono::high_resolution_clock::now();
    ReasonSet result = compute_gnrs_from_file(filename);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end_time - start_time;

    long long total_length = 0;
    for (const auto& r : result) {
        for (int v = 0; v < num_variables; ++v) {
            if (r.has_var(v)) total_length++;
        }
    }

    return { (long long)result.size(), total_length, duration.count() };
}

// Runs batch testing over a range of numbered log files
void run_gnr_batch_test(const std::string& prefix, int n) {
    run_batch_test(prefix, n, "GNR", process_gnr_file);
}

// Example usage: ./gnr banknote_100_4_gr0.log
int main(int argc, char* argv[]) {
    // batch testing
    // if (argc == 3) {
    //     std::string prefix = argv[1];
    //     int n = std::stoi(argv[2]);
    //     run_gnr_batch_test(prefix, n);
    //     return 0;
    // }

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        std::cerr << "Example: " << argv[0] << " banknote_100_4_gr0.log.log" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];
    std::string outpath = "gnrs_" + basename_from_path(filepath);

    try {
        ReasonSet result = compute_gnrs_from_file(filepath);

        write_reasons_file(
            outpath,
            result,
            [](const Reason& r, int v) { return r.has_var(v); },
            [](const Reason& r, int v) { return r.states() + v * S_WORDS; }
        );

        std::cout << "Wrote " << result.size() << " GNRs to " << outpath << std::endl;

    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}