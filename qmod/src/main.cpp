// CRASHERS:
void (*fuckyou)();
// fuckyou();
//
// (before il2cpp init):
// auto crashDamnit = il2cpp_functions::domain_get();
// GlobalNamespace::MainFlowCoordinator::_get__startWithSettings();

#include "main.hpp"

extern GameState gameState;

void setupInitialGameState() {
    gameState.gameVersion = to_utf8(csstrtostr(
        il2cpp_utils::RunMethod<Il2CppString*>(nullptr, il2cpp_utils::FindMethod("UnityEngine", "Application", "get_version")).value()
    ));
    gameState.deviceUniqueIdentifier = to_utf8(csstrtostr(
        reinterpret_cast<function_ptr_t<Il2CppString*>>(il2cpp_functions::resolve_icall("UnityEngine.SystemInfo::GetDeviceUniqueIdentifier()"))()
    ));
    gameState.operatingSystem = to_utf8(csstrtostr(
        reinterpret_cast<function_ptr_t<Il2CppString*>>(il2cpp_functions::resolve_icall("UnityEngine.SystemInfo::GetOperatingSystem()"))()
    ));
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
        gameState.prevSceneName = to_utf8(csstrtostr(il2cpp_utils::RunMethod<Il2CppString*>(&prevScene, GetName).value()));
    } else {
        gameState.prevSceneName = "";
    }
    if (il2cpp_utils::RunMethod<bool>(&nextScene, IsValid).value()) {
        gameState.nextSceneName = to_utf8(csstrtostr(il2cpp_utils::RunMethod<Il2CppString*>(&nextScene, GetName).value()));
    } else {
        gameState.nextSceneName = "";
    }
    gameState.currentSceneTime = (int)time(NULL);
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

extern "C" void init() {}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
}

extern "C" void load() {
    il2cpp_functions::Init();

    INSTALL_HOOK(getLogger(), SceneManager_Internal_ActiveSceneChanged);
    INSTALL_HOOK(getLogger(), CrashOnFirstScene);

    setupInitialGameState();
}
