#include "terminal.h"
#include <termios.h>
#include <pthread.h>
#include <optional>
#include <csignal>
#include <cstring>
#include <unistd.h>

// terminal_token implementation

namespace {
    std::atomic<std::optional<pthread_t>> blockingThread;
    struct termios oldt;

    // signal redirection is needed when master thread
    // caught a signal and should interrupt user input
    // waiting that is in another thread
    void signalRedirection(int sig) {
        auto opt = blockingThread.load();
        pthread_t self = pthread_self();
        if (opt && opt.value() != self) {
            pthread_kill(opt.value(), sig);
        } else {
            terminal_utils::interruptionHandler(sig);
        }
    }
}

void terminal_token::init() {
    struct sigaction sa{};
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = &signalRedirection;
    sa.sa_flags = 0;// not SA_RESTART! todo: why?

    bool ok = true;

    auto setSignalHandler = [&sa, &ok](int) {
        while (sigaction(SIGTERM, &sa, nullptr) == -1) {
            if (errno != EINTR) {
                ok = false;
                break;
            }
        }
    };

    setSignalHandler(SIGTERM); // kill
    setSignalHandler(SIGINT);  // ctrl+c
    setSignalHandler(SIGILL);  // illegal instruction
    setSignalHandler(SIGFPE);  // arithmetic exception
    setSignalHandler(SIGSEGV); // segfault
    signal(SIGPIPE, SIG_IGN); // prevents crash while writing to pipe without readers

    if (!ok) {
        throw std::runtime_error("[!] Unable to set signal handler");
    }

    // we assume that compilers are not interactive
    if (tcgetattr(STDIN_FILENO, &oldt) == -1) {
        throw std::runtime_error("[!] Unable to get terminal attributes");
    }

    struct termios newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VTIME] = 0;
    newt.c_cc[VMIN] = 1;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1) {
        throw std::runtime_error("[!] Unable to set terminal attributes");
    }
}

terminal_token::~terminal_token() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

// terminal implementation

int terminal::getChar() noexcept {
    tcflush(STDIN_FILENO, TCIFLUSH);
    blockingThread.store(pthread_self());

    int ch = 3;
    read(STDIN_FILENO, &ch, 1);
    blockingThread.load().reset();
    return ch;
}

void terminal::safePrint(const char *str, size_t len) noexcept {
    write(STDOUT_FILENO, str, len);
}
