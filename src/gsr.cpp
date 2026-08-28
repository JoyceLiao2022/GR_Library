#include "common.h"

// GSR-specific Reason struct
struct Reason {
    std::vector<uint64_t> var_mask; 
    std::vector<uint64_t> states;

    // Allocate variable mask and per-variable state bitmasks
    Reason() {
        if (num_variables > 0) {
            var_mask.resize(V_WORDS, 0);
            states.resize(num_variables * S_WORDS, 0);
        }
    }

    bool has_var(int v) const {
        return (var_mask[v / 64] >> (v % 64)) & 1;
    }

    void set_var(int v) {
        var_mask[v / 64] |= (1ULL << (v % 64));
    }

    // Compare two reasons lexicographically
    bool operator<(const Reason& o) const {
        for (int w = 0; w < V_WORDS; ++w) {
            if (var_mask[w] != o.var_mask[w]) return var_mask[w] < o.var_mask[w];
        }
        for (int v = 0; v < num_variables; ++v) {
            if (has_var(v)) {
                int offset = v * S_WORDS;
                for (int w = 0; w < S_WORDS; ++w) {
                    if (states[offset + w] != o.states[offset + w]) 
                        return states[offset + w] < o.states[offset + w];
                }
            }
        }
        return false;
    }

    // Determine if two reasons are identical
    bool operator==(const Reason& o) const {
        for (int w = 0; w < V_WORDS; ++w) {
            if (var_mask[w] != o.var_mask[w]) return false;
        }
        for (int v = 0; v < num_variables; ++v) {
            if (has_var(v)) {
                int offset = v * S_WORDS;
                for (int w = 0; w < S_WORDS; ++w) {
                    if (states[offset + w] != o.states[offset + w]) return false;
                }
            }
        }
        return true;
    }
};

using ReasonSet = std::vector<Reason>;
std::map<Node*, ReasonSet> global_cache;

// Performs subsumption on a set of reasons
ReasonSet subsume(ReasonSet S) {
    if (S.size() <= 1) return S;
    
    std::vector<int> popcounts(S.size());
    for(size_t i=0; i<S.size(); ++i) {
        int cnt = 0;
        for (int w = 0; w < V_WORDS; ++w) cnt += POPCOUNT(S[i].var_mask[w]);
        popcounts[i] = cnt;
    }

    std::vector<bool> keep(S.size(), true);

    int iter_count = 0;
    for (size_t i = 0; i < S.size(); ++i) {
        if (++iter_count % 128 == 0) check_timeout(); 

        if (!keep[i]) continue;
        const Reason& tau = S[i];

        for (size_t j = 0; j < S.size(); ++j) {
            if (i == j || !keep[j]) continue;
            if (popcounts[j] > popcounts[i]) continue; 

            const Reason& prime = S[j];
            bool var_subset = true;
            for (int w = 0; w < V_WORDS; ++w) {
                if ((tau.var_mask[w] & prime.var_mask[w]) != prime.var_mask[w]) {
                    var_subset = false; break;
                }
            }

            if (var_subset) {
                bool state_subset = true;
                for (int w = 0; w < V_WORDS; ++w) {
                    uint64_t mask = prime.var_mask[w];
                    while (mask) {
                        int bit = CTZ(mask);
                        int v = w * 64 + bit;
                        mask &= (mask - 1);
                        
                        int offset = v * S_WORDS;
                        for (int sw = 0; sw < S_WORDS; ++sw) {
                            if ((tau.states[offset + sw] & prime.states[offset + sw]) != tau.states[offset + sw]) {
                                state_subset = false; break;
                            }
                        }
                        if (!state_subset) break;
                    }
                    if (!state_subset) break;
                }
                
                if (state_subset) {
                    if (popcounts[i] == popcounts[j]) {
                        bool identical = true;
                        for (int w = 0; w < V_WORDS; ++w) if (tau.var_mask[w] != prime.var_mask[w]) { identical = false; break; }
                        if (identical) {
                            for (int w = 0; w < num_variables * S_WORDS; ++w) {
                                if (tau.states[w] != prime.states[w]) { identical = false; break; }
                            }
                        }
                        if (identical && j < i) { keep[i] = false; break; } 
                        else if (!identical) { keep[i] = false; break; }
                    } else {
                        keep[i] = false; break;
                    }
                }
            }
        }
    }
    
    ReasonSet result;
    result.reserve(S.size());
    for (size_t i = 0; i < S.size(); ++i) {
        if (keep[i]) result.push_back(std::move(S[i]));
    }
    return result;
}

// Removes variable exclusive reasons
ReasonSet var_minimize(ReasonSet S, const std::vector<uint64_t>& V_mask) {
    if (S.size() <= 1) return S;

    std::vector<int> popcounts(S.size());
    for(size_t i=0; i<S.size(); ++i) {
        int cnt = 0;
        for (int w = 0; w < V_WORDS; ++w) cnt += POPCOUNT(S[i].var_mask[w]);
        popcounts[i] = cnt;
    }

    std::vector<bool> keep(S.size(), true);

    int iter_count = 0;
    for (size_t i = 0; i < S.size(); ++i) {
        if (++iter_count % 128 == 0) check_timeout(); 

        if (!keep[i]) continue;
        const Reason& tau = S[i];

        for (size_t j = 0; j < S.size(); ++j) {
            if (i == j || !keep[j]) continue;
            if (popcounts[j] >= popcounts[i]) continue; 

            const Reason& prime = S[j];
            bool var_subset = true;
            for (int w = 0; w < V_WORDS; ++w) {
                if ((tau.var_mask[w] & prime.var_mask[w]) != prime.var_mask[w]) {
                    var_subset = false; break;
                }
            }

            if (var_subset) {
                bool extra_in_V = true;
                for (int w = 0; w < V_WORDS; ++w) {
                    uint64_t extra = tau.var_mask[w] ^ prime.var_mask[w];
                    if ((extra & ~V_mask[w]) != 0) {
                        extra_in_V = false; break;
                    }
                }
                if (extra_in_V) {
                    keep[i] = false; break;
                }
            }
        }
    }
    
    ReasonSet result;
    result.reserve(S.size());
    for (size_t i = 0; i < S.size(); ++i) {
        if (keep[i]) result.push_back(std::move(S[i]));
    }
    return result;
}

// Cross (Cartesian) product of two reason sets connected by an AND node
ReasonSet cross_product(const ReasonSet& setA, const ReasonSet& setB) {
    ReasonSet result;
    if (setA.empty() || setB.empty()) return result;

    result.reserve(setA.size() * setB.size());

    int iter_count = 0;
    for (const auto& rA : setA) {
        if (++iter_count % 32 == 0) check_timeout(); 

        for (const auto& rB : setB) {
            bool contradiction = false;
            
            for (int w = 0; w < V_WORDS; ++w) {
                uint64_t overlap = rA.var_mask[w] & rB.var_mask[w];
                while (overlap) {
                    int bit = CTZ(overlap);
                    int v = w * 64 + bit;
                    overlap &= (overlap - 1);
                    
                    int offset = v * S_WORDS;
                    bool any_state = false;
                    for (int sw = 0; sw < S_WORDS; ++sw) {
                        if ((rA.states[offset + sw] & rB.states[offset + sw]) != 0) {
                            any_state = true;
                            break;
                        }
                    }
                    if (!any_state) { contradiction = true; break; }
                }
                if (contradiction) break;
            }
            
            if (contradiction) continue;

            Reason new_reason = rA;
            for (int w = 0; w < V_WORDS; ++w) {
                uint64_t maskB = rB.var_mask[w];
                new_reason.var_mask[w] |= maskB;
                
                while(maskB) {
                    int bit = CTZ(maskB);
                    int v = w * 64 + bit;
                    maskB &= (maskB - 1);
                    
                    int offset = v * S_WORDS;
                    if (rA.has_var(v)) {
                        for (int sw = 0; sw < S_WORDS; ++sw) {
                            new_reason.states[offset + sw] &= rB.states[offset + sw];
                        }
                    } else {
                        for (int sw = 0; sw < S_WORDS; ++sw) {
                            new_reason.states[offset + sw] = rB.states[offset + sw];
                        }
                    }
                }
            }
            result.push_back(std::move(new_reason));
        }
    }

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

// Recursively computes GSRs
ReasonSet GSR(Node* delta, bool is_root = false) {
    check_timeout(); 

    if (global_cache.count(delta)) {
        ReasonSet res = global_cache[delta];
        if (!is_root) {
            delta->cache_hits_remaining--;
            if (delta->cache_hits_remaining <= 0) global_cache.erase(delta);
        }
        return res;
    }

    ReasonSet gsr;

    if (delta->type == LEAF) {
        Reason r;
        r.set_var(delta->var_id);
        int offset = delta->var_id * S_WORDS;
        for (int w = 0; w < S_WORDS; ++w) r.states[offset + w] = delta->states[w];
        gsr.push_back(std::move(r));
    }
    else if (delta->type == TRUE_NODE) gsr.push_back(Reason()); 
    else if (delta->type == FALSE_NODE) { /* Leave empty */ }
    else if (delta->type == OR_NODE) {
        for (auto& child : delta->children) {
            ReasonSet child_gsr = GSR(child.get());
            for (auto& r : child_gsr) gsr.push_back(std::move(r));
        }
        std::sort(gsr.begin(), gsr.end());
        gsr.erase(std::unique(gsr.begin(), gsr.end()), gsr.end());
        gsr = subsume(std::move(gsr));
    }
    else if (delta->type == AND_NODE) {
        if (!delta->children.empty()) {
            gsr = GSR(delta->children[0].get());
            for (size_t i = 1; i < delta->children.size(); ++i) {
                gsr = cross_product(gsr, GSR(delta->children[i].get()));
                gsr = subsume(std::move(gsr)); 
            }
        } else {
            gsr.push_back(Reason()); 
        }
    }

    if (is_root || (delta->type == AND_NODE && gsr.size() > 10)) {
        gsr = var_minimize(std::move(gsr), delta->V_mask);
    }

    if (!is_root) {
        delta->cache_hits_remaining--;
        if (delta->cache_hits_remaining > 0) {
            global_cache[delta] = gsr;
        }
    }

    return gsr;
}

// Computes per-node exclusive variable masks (V_mask) needed for variable minimization
void precompute_exclusive_vars(std::shared_ptr<Node> root) {
    std::set<Node*> visited;
    std::vector<Node*> all_nodes;
    
    // toposort
    std::function<void(std::shared_ptr<Node>)> get_all_nodes = [&](std::shared_ptr<Node> node) {
        if (visited.count(node.get())) return;
        visited.insert(node.get());
        for (auto& child : node->children) get_all_nodes(child);
        all_nodes.push_back(node.get());
    };
    get_all_nodes(root);

    for (Node* n : all_nodes) n->cache_hits_remaining = n->in_degree;
    for (Node* n : all_nodes) {
        if (n->type == LEAF) {
            n->in_vars[n->var_id / 64] |= (1ULL << (n->var_id % 64));
        } else {
            for (auto& c : n->children) {
                for (int w = 0; w < V_WORDS; ++w) n->in_vars[w] |= c->in_vars[w];
            }
        }
    }

    std::queue<Node*> Q;
    Q.push(root.get());
    while (!Q.empty()) {
        Node* n = Q.front();
        Q.pop();

        for (auto& c_ptr : n->children) {
            Node* c = c_ptr.get();
            
            for (int w = 0; w < V_WORDS; ++w) c->out_vars[w] |= n->out_vars[w];

            if (n->type == AND_NODE) {
                for (auto& cc_ptr : n->children) {
                    Node* cc = cc_ptr.get();
                    if (cc != c) {
                        bool intersects = false;
                        for (int w = 0; w < V_WORDS; ++w) {
                            if (c->in_vars[w] & cc->in_vars[w]) { intersects = true; break; }
                        }
                        if (intersects) {
                            for (int w = 0; w < V_WORDS; ++w) c->out_vars[w] |= cc->in_vars[w];
                        }
                    }
                }
            }

            c->processed_parents++;
            if (c->processed_parents == c->in_degree) Q.push(c);
        }
    }

    for (Node* n : all_nodes) {
        for (int w = 0; w < V_WORDS; ++w) {
            n->V_mask[w] = n->in_vars[w] & ~(n->out_vars[w]);
        }
    }
}

ReasonSet compute_gsrs_from_file(const std::string& filepath) {
    std::ifstream file_stream(filepath);
    if (!file_stream.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath);
    }

    NNFParser parser;
    auto root = parser.parse(file_stream);

    time_limit = std::chrono::high_resolution_clock::now() + std::chrono::minutes(1); // 1 minute timeout; change as needed

    precompute_exclusive_vars(root);

    global_cache.clear();
    return GSR(root.get(), true);
}

BatchProcessResult process_gsr_file(const std::string& filename) {
    auto start_time = std::chrono::high_resolution_clock::now();
    ReasonSet result = compute_gsrs_from_file(filename);
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
void run_gsr_batch_test(const std::string& prefix, int n) {
    run_batch_test(prefix, n, "GSR", process_gsr_file);
}

// Example usage: ./gsr banknote_100_4_gr0.log
int run_gsr_cli(int argc, char* argv[]) {
    // batch testing
    // if (argc == 3) {
    //     std::string prefix = argv[1];
    //     int n = std::stoi(argv[2]);
    //     run_gsr_batch_test(prefix, n);
    //     return 0;
    // }

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        std::cerr << "Example: " << argv[0] << " banknote_100_4_gr0.log" << std::endl;
        return 1;
    }

    std::string filepath = argv[1];
    std::string outpath = "gsrs_" + basename_from_path(filepath);

    try {
        ReasonSet result = compute_gsrs_from_file(filepath);

        write_reasons_file(
            outpath,
            result,
            [](const Reason& r, int v) { return r.has_var(v); },
            [](const Reason& r, int v) { return r.states.data() + v * S_WORDS; }
        );

        std::cout << "Wrote " << result.size() << " GSRs to " << outpath << std::endl;

    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}