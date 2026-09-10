#include <gr/gr.h>

#include <iostream>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: gr_example <input.log>\n";
        return 1;
    }

    auto gsrs = gr::compute_gsrs_from_file(argv[1]);
    auto gnrs = gr::compute_gnrs_from_file(argv[1]);

    std::cout << "GSR count: " << gsrs.size() << '\n';

    for (const auto& reason : gsrs) {
        for (const auto& assignment : reason) {
            std::cout
                << assignment.variable
                << ":"
                << assignment.states
                << " ";
        }

        std::cout << '\n';
    }

    std::cout << "GNR count: " << gnrs.size() << '\n';

    for (const auto& reason : gnrs) {
        for (const auto& assignment : reason) {
            std::cout
                << assignment.variable
                << ":"
                << assignment.states
                << " ";
        }

        std::cout << '\n';

        return 0;
    }
}