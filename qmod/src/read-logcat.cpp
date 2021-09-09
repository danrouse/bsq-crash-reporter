#include "read-logcat.hpp"

std::string getLogcatLines(int numLines) {
    auto fp = popen(string_format("logcat -t %d OVRPlatform:S VrApi:S chatty:S OsSdk:S", numLines).c_str(), "r");
    std::string output;
    bool isFirstLine = true;
    char buf[1024];
    while (fgets(buf, sizeof(buf), fp)) {
        if (isFirstLine) {
            isFirstLine = false;
        } else {
            output.append(buf);
        }
    }
    pclose(fp);
    output.pop_back();
    return output;
}
