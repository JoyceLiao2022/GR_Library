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
//    std::cout << "\nGSRs:\n";
//
//    for (const auto& reason : gsrs) {
//        for (const auto& assignment : reason) {
//            std::cout
//                << assignment.variable
//                << " "
//                << assignment.states
//                << " ";
//        }
//
//        std::cout << '\n';
//    }
//
//    std::cout << "\nGNRs:\n";
//
//    for (const auto& reason : gnrs) {
//        for (const auto& assignment : reason) {
//            std::cout
//                << assignment.variable
//                << " "
//                << assignment.states
//                << " ";
//        }
//
//        std::cout << '\n';
//    }
//
//    return 0;
//}

#include <gr/gr.h>

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: api_smoke <toy.log>\n";
        return 1;
    }

    auto gsrs = gr::compute_gsrs_from_file(argv[1]);
    auto gnrs = gr::compute_gnrs_from_file(argv[1]);

    if (gsrs.size() != 2) {
        std::cerr << "Expected 2 GSRs, got "
            << gsrs.size() << '\n';
        return 1;
    }

    if (gnrs.size() != 1) {
        std::cerr << "Expected 1 GNR, got "
            << gnrs.size() << '\n';
        return 1;
    }

    std::cout << "C++ API smoke test passed\n";
    return 0;
}