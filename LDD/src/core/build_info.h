#ifndef BUILD_INFO_H
#define BUILD_INFO_H

#include <string>

struct BuildInfo {
    std::string id;
    std::string name;
    std::string description;
    std::string minecraftVersion;
    std::string loader;
    int javaVersion;
    std::string downloadUrl;
    std::string checksum;
    long long sizeBytes;
    std::string imageUrl;
    std::string githubRepo;
    std::string githubAsset;
    std::string tagsCsv;
    std::string javaPath;
    bool isFree = true;
    int priceCents = 0;
    bool locked = false;
};

#endif // BUILD_INFO_H
