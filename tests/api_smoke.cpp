//#include <gr/gr.h>
//#include <iostream>
//
//int main()
//{
//    auto gsrs =
//        gr::compute_gsrs_from_file("tests/data/toy.log");
//
//    auto gnrs =
//        gr::compute_gnrs_from_file("tests/data/toy.log");
//
//    std::cout << "GSR count: " << gsrs.size() << '\n';
//    std::cout << "GNR count: " << gnrs.size() << '\n';
//
//    return 0;
//}

#include <gr/gr.h>
#include <iostream>

int main()
{
    auto gsrs =
        gr::compute_gsrs_from_file("tests/data/toy.log");

    auto gnrs =
        gr::compute_gnrs_from_file("tests/data/toy.log");

    std::cout << "GSR count: " << gsrs.size() << '\n';
    std::cout << "GNR count: " << gnrs.size() << '\n';

    std::cout << "\nGSRs:\n";

    for (const auto& reason : gsrs) {
        for (const auto& assignment : reason) {
            std::cout
                << assignment.variable
                << " "
                << assignment.states
                << " ";
        }

        std::cout << '\n';
    }

    std::cout << "\nGNRs:\n";

    for (const auto& reason : gnrs) {
        for (const auto& assignment : reason) {
            std::cout
                << assignment.variable
                << " "
                << assignment.states
                << " ";
        }

        std::cout << '\n';
    }

    return 0;
}