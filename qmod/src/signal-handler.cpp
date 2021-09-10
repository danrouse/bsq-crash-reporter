#include "signal-handler.hpp"

GameState gameState;

std::unordered_map<int, void (*)(int, struct siginfo*, void*)> signalHandlers;
void signalHandler(int signal, siginfo_t* inst, void* ctx) {
    auto context = static_cast<ucontext_t*>(ctx);
    auto logcatLines = getLogcatLines(25);
    auto lr = context->uc_mcontext.regs[30];
    auto backtrace = getBacktraceLines(25,  lr, 0);
    auto backtraceLibs = getLibraryDataStrings(backtrace.libraries);
    auto memoryLibs = getLibraryDataStrings(getLoadedMods());

    getLogger().debug("Handle signal: %d", signal);

    FormFields fields = {
        {"gameVersion", gameState.gameVersion},
        {"operatingSystem", gameState.operatingSystem},
        {"deviceUniqueIdentifier", gameState.deviceUniqueIdentifier},
        {"prevSceneName", gameState.prevSceneName},
        {"nextSceneName", gameState.nextSceneName},
        {"secondsInScene", gameState.currentSceneTime
            ? std::to_string((int)time(NULL) - gameState.currentSceneTime)
            : "0"},
        {"registerSP", string_format("0x%016llx", context->uc_mcontext.sp)},
        {"registerLR", string_format("0x%016llx", lr)},
        {"registerPC", string_format("0x%016llx", context->uc_mcontext.pc)},
        {"signal", std::to_string(signal)},
        {"faultAddress", string_format("0x%02llx", context->uc_mcontext.fault_address)},
        {"memoryLibs", memoryLibs.first},
        {"memoryLibBuildIds", memoryLibs.second},
        {"backtraceLibs", backtraceLibs.first},
        {"backtraceLibBuildIds", backtraceLibs.second},
        {"backtrace", backtrace.lines},
        {"log", logcatLines},
    };
    // for (const auto& field : fields) {
    //     getLogger().debug("Upload field: %s: \"%s\"", field.first, field.second.c_str());
    // }
    auto result = uploadCrashLog(fields);
    // getLogger().debug("response: %d, %s", result.success, result.text.c_str());

    if (signalHandlers[signal]) (*signalHandlers[signal])(signal, inst, ctx);
    _exit(1); // Time to die, Mr. Bond
}

void registerSignalHandlers() {
    struct sigaction newAction;
    newAction.sa_sigaction = signalHandler;
    sigemptyset(&newAction.sa_mask);
    newAction.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART | SA_RESETHAND;
    sigaction(SIGILL, &newAction, NULL);
    sigaction(SIGABRT, &newAction, NULL);
    sigaction(SIGBUS, &newAction, NULL);
    sigaction(SIGFPE, &newAction, NULL);
    sigaction(SIGSEGV, &newAction, NULL);
    sigaction(SIGPIPE, &newAction, NULL);
    sigaction(SIGSTKFLT, &newAction, NULL);
}

MAKE_HOOK(hook_sigaction, nullptr, int, int signum, struct sigaction * act, void * oldact) {
    if (act && (
        signum == SIGILL ||
        signum == SIGABRT ||
        signum == SIGBUS ||
        signum == SIGFPE ||
        signum == SIGSEGV ||
        signum == SIGPIPE ||
        signum == SIGSTKFLT
    )) {
        signalHandlers[signum] = act->sa_sigaction;
        return 0;
    } else {
        return hook_sigaction(signum, act, oldact);
    }
}

__attribute__((constructor)) void startup() {
    registerSignalHandlers();
    INSTALL_HOOK_DIRECT(getLogger(), hook_sigaction, dlsym(RTLD_DEFAULT, "sigaction"));
}
