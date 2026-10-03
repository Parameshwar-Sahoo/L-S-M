#include "ipc/SignalHandler.hpp"
#include <iostream>
#include <cstring>

namespace lms {

std::atomic<bool> SignalHandler::s_running{true};
std::function<void()> SignalHandler::s_diagnosticsCallback = nullptr;

void SignalHandler::handleSignal(int signum)
{
    if (signum == SIGINT || signum == SIGTERM) {
        s_running.store(false);
    } else if (signum == SIGUSR1) {
        if (s_diagnosticsCallback) {
            s_diagnosticsCallback();
        }
    }
}

void SignalHandler::init()
{
    struct sigaction sa{};
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    ::sigaction(SIGINT, &sa, nullptr);
    ::sigaction(SIGTERM, &sa, nullptr);
    ::sigaction(SIGUSR1, &sa, nullptr);
}

bool SignalHandler::isRunning()
{
    return s_running.load();
}

void SignalHandler::requestShutdown()
{
    s_running.store(false);
}

void SignalHandler::setDiagnosticsCallback(std::function<void()> cb)
{
    s_diagnosticsCallback = std::move(cb);
}

} // namespace lms
