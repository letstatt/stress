#include "stress.h"
#include "terminal.h"
#include <optional>

namespace {
    std::optional<terminal_token> t_opt;
}

void termination_handler();

int main(int argc, char* argv[]) {
    // handle uncaught C++ exceptions
    std::set_terminate(termination_handler);

    if (argc == 1) {
        stress::usage();
        return 0;
    }

    try {
        t_opt.emplace();
        stress::start(argc, argv);

    } catch (std::runtime_error const& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }

    return 0;
}

void termination_handler() {
    if (t_opt.has_value()) {
        terminal_token token(std::move(t_opt.value()));
        t_opt = std::nullopt;
        terminal::printTerminationMessage();
     // token.~terminal_token();
    }
    abort();
}
