namespace gr::detail::gnr {
    int run_gnr_cli(int argc, char* argv[]);
}

int main(int argc, char* argv[]) {
    return gr::detail::gnr::run_gnr_cli(argc, argv);
}