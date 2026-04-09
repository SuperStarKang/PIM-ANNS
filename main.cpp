#include "main.h"
#include <cstring>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

namespace {

void crash_signal_handler(int sig)
{
    const char header[] = "\n[pim-trace] fatal signal caught, dumping backtrace to stderr\n";
    write(STDERR_FILENO, header, sizeof(header) - 1);

    void *frames[64];
    int frame_count = backtrace(frames, 64);
    backtrace_symbols_fd(frames, frame_count, STDERR_FILENO);

    _exit(128 + sig);
}

void install_crash_handlers()
{
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = crash_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
}

} // namespace

int main(int argc, char **argv)
{
    install_crash_handlers();

    int nprobe = -1; 
    if (argc > 1)
    {
        nprobe = atoi(argv[1]);
    }
    else
    {
        printf("Usage: %s <nprobe>\n", argv[0]);
        return 1;
    }

    

    CPPkernel(10,  nprobe);

    return 0;
}
