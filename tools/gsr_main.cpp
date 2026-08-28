namespace gr::detail::gsr {
    int run_gsr_cli(int argc, char* argv[]);
}

int main(int argc, char* argv[]) {
    return gr::detail::gsr::run_gsr_cli(argc, argv);
}