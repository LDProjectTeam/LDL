#ifndef BUILD_SELECTOR_H
#define BUILD_SELECTOR_H

#include <string>
#include <vector>

struct BuildInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string minecraftVersion;
    int javaVersion;
    std::string downloadUrl;
    std::string checksum;
    long long sizeBytes;
};

#endif // BUILD_SELECTOR_H
