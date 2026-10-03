#ifndef SIGNAL_HANDLER_HPP
#define SIGNAL_HANDLER_HPP

#include <csignal>
#include <atomic>
#include <functional>

namespace lms {

class SignalHandler {
public:
    static void init();
    static bool isRunning();
    static void requestShutdown();
    static void setDiagnosticsCallback(std::function<void()> cb);

private:
    static void handleSignal(int signum);
    static std::atomic<bool> s_running;
    static std::function<void()> s_diagnosticsCallback;
};

} // namespace lms

#endif // SIGNAL_HANDLER_HPP
