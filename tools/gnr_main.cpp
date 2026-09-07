//namespace gr::detail::gnr {
//    int run_gnr_cli(int argc, char* argv[]);
//}
//
//int main(int argc, char* argv[]) {
//    return gr::detail::gnr::run_gnr_cli(argc, argv);
//}

#include <gr/gr.h>
#include "cli_common.h"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr
            << "Usage: "
            << argv[0]
            << " <input_file>\n";

        return 1;
    }

    std::string filepath = argv[1];

    std::string outpath =
        "gnrs_" + gr_cli::basename_from_path(filepath);

    try {
        gr::ReasonSet result =
            gr::compute_gnrs_from_file(filepath);

        gr_cli::write_reasons_file(
            outpath,
            result
        );

        std::cout
            << "Wrote "
            << result.size()
            << " GNRs to "
            << outpath
            << '\n';
    }
    catch (const std::exception& e) {
        std::cerr
            << "Error: "
            << e.what()
            << '\n';

        return 1;
    }

    return 0;
}