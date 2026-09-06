#pragma once

#include <string>
#include <vector>

namespace gr {

    struct Assignment { // smallest piece of reasons
        int variable; // var_num
        std::string states; // bitmask
    };

    using Reason = std::vector<Assignment>;
    using ReasonSet = std::vector<Reason>;

    ReasonSet compute_gsrs_from_file(const std::string& filepath);

    ReasonSet compute_gnrs_from_file(const std::string& filepath);

} // namespace gr