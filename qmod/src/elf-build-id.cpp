#include "elf-build-id.hpp"

std::pair<std::string, std::string> getLibraryDataStrings(std::unordered_set<std::string> libraries) {
    std::string libraryNames = "";
    std::string buildIds = "";
    for (const auto& path : libraries) {
        auto buildId = getBuildId(path.c_str());
        auto libraryName = path.substr(path.find_last_of("/") + 1);
        libraryNames.append(libraryName + "\n");
        buildIds.append(buildId + "\n");
    }
    if (libraries.size() > 0) {
        libraryNames.pop_back();
        buildIds.pop_back();
    }
    return {libraryNames, buildIds};
}

std::unordered_map<const char*, std::string> buildIdMemo;
std::string getBuildId(const char* elfPath) {
    if (buildIdMemo.contains(elfPath)) return buildIdMemo[elfPath];
    
    auto fp = fopen(elfPath, "r");
    std::string buildId;
    if (fp) {
        Elf64_Ehdr ehdr;
        fseek(fp, 0, SEEK_SET);
        fread(&ehdr, 1, sizeof(Elf64_Ehdr), fp);
        fseek(fp, ehdr.e_shoff, SEEK_SET);
        Elf64_Shdr* shTable = (Elf64_Shdr*)malloc(ehdr.e_shentsize * ehdr.e_shnum);
        fread(shTable, 1, ehdr.e_shentsize * ehdr.e_shnum, fp);
        char* buff = (char*)malloc(shTable[ehdr.e_shstrndx].sh_size);
        fseek(fp, shTable[ehdr.e_shstrndx].sh_offset, SEEK_SET);
        fread(buff, 1, shTable[ehdr.e_shstrndx].sh_size, fp);
        for (int i = 0; i < ehdr.e_shnum; i++) {
            if (!strcmp(".note.gnu.build-id", buff + shTable[i].sh_name)) {
                char *rawBuildId = (char*)malloc(shTable[i].sh_size - 0x10);
                fseek(fp, shTable[i].sh_offset + 0x10, SEEK_SET);
                fread(rawBuildId, 1, shTable[i].sh_size - 0x10, fp);
                char* hashString = NULL;
                for (int j = 0; j < shTable[i].sh_size - 0x10; j++) {
                    buildId.append(string_format("%02x", rawBuildId[j]));
                }
            }
        }
        fclose(fp);
    }
    if (!buildId.empty()) buildIdMemo[elfPath] = buildId;
    return buildId;
}
