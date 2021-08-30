#include "main.hpp"

// TODO: This seems like a bad place to store data
const char* dataFilePath = "/sdcard/Android/data/com.beatgames.beatsaber/files/crash-reporter.txt";

std::string latestTombstonePath;
time_t latestTombstoneTime;
bool hasNewTombstone = false;
const bool ALWAYS_SHOW_DIALOG = true; // for debugging

time_t readLatestKnownTombstoneTime() {
    time_t latest;
    std::ifstream reader(dataFilePath);
    if (reader) {
        reader >> latest;
        reader.close();
    }
    return latest;
}

void getLatestTombstone(time_t* timestamp, std::string* filename) {
    static std::string tombstonePathStem = "/sdcard/Android/data/com.beatgames.beatsaber/files/tombstone_0";
    struct stat statResult;
    for (int i = 2; i >= 0; i--) {
        std::string tombstonePath = tombstonePathStem + std::to_string(i);
        if (stat(tombstonePath.c_str(), &statResult) == 0) {
            if (!timestamp || statResult.st_mtime > *timestamp) {
                *timestamp = statResult.st_mtime;
                *filename = tombstonePath;
            }
        }
    }
}

extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;

    time_t latestKnownTombstoneTime = readLatestKnownTombstoneTime();
    getLogger().debug("Last known tombstone time: %ld", latestKnownTombstoneTime);

    getLatestTombstone(&latestTombstoneTime, &latestTombstonePath);
    getLogger().debug("Latest tombstone time: %ld - %s", latestTombstoneTime, latestTombstonePath.c_str());

    if (!latestKnownTombstoneTime || latestTombstoneTime > latestKnownTombstoneTime) {
        getLogger().info("A new tombstone was detected, a crash report dialog will be shown");
        hasNewTombstone = true;
    }    
}

// TODO: Confirm if this is actually not a terrible place to hook the main menu
// (On scene transition is more common (and more involved to setup), but I don't know why?)
MAKE_HOOK_MATCH(MainFlowCoordinator_DidActivate, &GlobalNamespace::MainFlowCoordinator::DidActivate, void, GlobalNamespace::MainFlowCoordinator* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    MainFlowCoordinator_DidActivate(self, firstActivation, addedToHierarchy, screenSystemEnabling);
    if (ALWAYS_SHOW_DIALOG || (firstActivation && hasNewTombstone)) {
        getLogger().info("Showing crash report dialog");
        showCrashReportDialog(latestTombstonePath.c_str());
        std::ofstream writer(dataFilePath);
        if (writer) {
            getLogger().debug("Writing latest tombstone time to %s", dataFilePath);
            writer << latestTombstoneTime;
            writer.close();
        }
    }
}

extern "C" void load() {
    il2cpp_functions::Init();
    INSTALL_HOOK(getLogger(), MainFlowCoordinator_DidActivate);
}
