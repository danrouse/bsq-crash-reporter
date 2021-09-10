// CRASHERS:
void (*fuckyou)();
// fuckyou();
//
// (before il2cpp init):
// auto crashDamnit = il2cpp_functions::domain_get();
// GlobalNamespace::MainFlowCoordinator::_get__startWithSettings();

#include "main.hpp"

#include <chrono>
// Application state to bundle with crash reports
std::string gameVersion;
std::string deviceUniqueIdentifier;
std::string operatingSystem;
std::string prevSceneName;
std::string nextSceneName;
int currentSceneTime = 0;

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
        {"gameVersion", gameVersion},
        {"operatingSystem", operatingSystem},
        {"deviceUniqueIdentifier", deviceUniqueIdentifier},
        {"prevSceneName", prevSceneName},
        {"nextSceneName", nextSceneName},
        {"secondsInScene", currentSceneTime ? std::to_string((int)time(NULL) - currentSceneTime) : "0"},
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
    for (const auto& field : fields) {
        getLogger().debug("Upload field: %s: \"%s\"", field.first, field.second.c_str());
    }
    // auto result = uploadCrashLog(fields);
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

extern "C" void init() {}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
}

MAKE_HOOK_FIND_CLASS_UNSAFE_STATIC(
    SceneManager_Internal_ActiveSceneChanged,
    "UnityEngine.SceneManagement", "SceneManager", "Internal_ActiveSceneChanged",
    void,
    Il2CppObject* prevScene,
    Il2CppObject* nextScene
) {
    static auto IsValid = il2cpp_utils::FindMethod("UnityEngine.SceneManagement", "Scene", "IsValid");
    static auto GetName = il2cpp_utils::FindMethod("UnityEngine.SceneManagement", "Scene", "get_name");
    if (il2cpp_utils::RunMethod<bool>(&prevScene, IsValid).value()) {
        prevSceneName = to_utf8(csstrtostr(il2cpp_utils::RunMethod<Il2CppString*>(&prevScene, GetName).value()));
    } else {
        prevSceneName = "";
    }
    if (il2cpp_utils::RunMethod<bool>(&nextScene, IsValid).value()) {
        nextSceneName = to_utf8(csstrtostr(il2cpp_utils::RunMethod<Il2CppString*>(&nextScene, GetName).value()));
    } else {
        nextSceneName = "";
    }
    currentSceneTime = (int)time(NULL);
    SceneManager_Internal_ActiveSceneChanged(prevScene, nextScene);
}

MAKE_HOOK_FIND_CLASS_INSTANCE(
    CrashOnFirstScene,
    "", "HealthWarningFlowCoordinator", "DidActivate",
    void,
    Il2CppObject* self,
    bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling
) {
    getLogger().debug("calling orig hook with a nullptr what could go wrong?");
    CrashOnFirstScene(nullptr, firstActivation, addedToHierarchy, screenSystemEnabling);
}

extern "C" void load() {
    il2cpp_functions::Init();

    INSTALL_HOOK(getLogger(), SceneManager_Internal_ActiveSceneChanged);
    INSTALL_HOOK(getLogger(), CrashOnFirstScene);

    gameVersion = to_utf8(csstrtostr(
        il2cpp_utils::RunMethod<Il2CppString*>(nullptr, il2cpp_utils::FindMethod("UnityEngine", "Application", "get_version")).value()
    ));
    deviceUniqueIdentifier = to_utf8(csstrtostr(
        reinterpret_cast<function_ptr_t<Il2CppString*>>(il2cpp_functions::resolve_icall("UnityEngine.SystemInfo::GetDeviceUniqueIdentifier()"))()
    ));
    operatingSystem = to_utf8(csstrtostr(
        reinterpret_cast<function_ptr_t<Il2CppString*>>(il2cpp_functions::resolve_icall("UnityEngine.SystemInfo::GetOperatingSystem()"))()
    ));

    // fuckyou();
}
