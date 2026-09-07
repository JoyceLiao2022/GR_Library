#pragma once

#include <gr/gr.h>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>

namespace gr_cli {

    inline std::string basename_from_path(const std::string& path)
    {
        std::size_t pos = path.find_last_of("/\\");

        if (pos == std::string::npos) {
            return path;
        }

        return path.substr(pos + 1);
    }

    inline void write_reasons_file(
        const std::string& outpath,
        const gr::ReasonSet& reasons)
    {
        std::vector<int> lengths;
        lengths.reserve(reasons.size());

        long long total_length = 0;

        for (const auto& reason : reasons) {
            int length = static_cast<int>(reason.size());
            lengths.push_back(length);
            total_length += length;
        }

        int min_length =
            reasons.empty()
            ? 0
            : *std::min_element(lengths.begin(), lengths.end());

        int max_length =
            reasons.empty()
            ? 0
            : *std::max_element(lengths.begin(), lengths.end());

        double avg_length =
            reasons.empty()
            ? 0.0
            : static_cast<double>(total_length) / reasons.size();

        std::ofstream out(outpath);

        if (!out.is_open()) {
            throw std::runtime_error(
                "Could not open output file: " + outpath
            );
        }

        out << "total " << reasons.size() << '\n';
        out << "min_length " << min_length << '\n';
        out << "max_length " << max_length << '\n';

        out << std::fixed << std::setprecision(2)
            << "avg_length " << avg_length << '\n';

        for (const auto& reason : reasons) {
            for (const auto& assignment : reason) {
                out << assignment.variable
                    << " "
                    << assignment.states
                    << " ";
            }

            out << '\n';
        }
    }

} // namespace gr_cli