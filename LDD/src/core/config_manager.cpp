#include "config_manager.h"
#include <fstream>
#include <windows.h>
#include <sstream>
#include <algorithm>

std::string ConfigManager::getExecutablePath() const
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string fullPath(path);
    return fullPath.substr(0, fullPath.find_last_of("\\/"));
}

ConfigManager::ConfigManager()
{
    launcherPath = getExecutablePath();
    buildsJsonPath = launcherPath + "\\builds.json";
}

// Простой JSON парсер для этого приложения
static BuildInfo parseJsonBuild(const std::string &jsonStr)
{
    BuildInfo build;
    
    // Извлекаем значения между кавычками
    auto extractValue = [&jsonStr](const std::string &key) -> std::string {
        size_t keyPos = jsonStr.find("\"" + key + "\"");
        if (keyPos == std::string::npos) return "";
        
        size_t colonPos = jsonStr.find(":", keyPos);
        size_t quoteStart = jsonStr.find("\"", colonPos);
        size_t quoteEnd = jsonStr.find("\"", quoteStart + 1);
        
        if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
            return jsonStr.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        }
        return "";
    };
    
    auto extractNumber = [&jsonStr](const std::string &key) -> long long {
        size_t keyPos = jsonStr.find("\"" + key + "\"");
        if (keyPos == std::string::npos) return 0;
        
        size_t colonPos = jsonStr.find(":", keyPos);
        size_t numStart = jsonStr.find_first_of("0123456789", colonPos);
        size_t numEnd = jsonStr.find_first_not_of("0123456789", numStart);
        
        if (numStart != std::string::npos && numEnd != std::string::npos) {
            std::string numStr = jsonStr.substr(numStart, numEnd - numStart);
            return std::stoll(numStr);
        }
        return 0;
    };
    
    build.id = extractValue("id");
    build.name = extractValue("name");
    build.description = extractValue("description");
    build.minecraftVersion = extractValue("minecraftVersion");
    build.loader = extractValue("loader");
    build.javaVersion = static_cast<int>(extractNumber("javaVersion"));
    build.downloadUrl = extractValue("downloadUrl");
    build.checksum = extractValue("checksum");
    build.sizeBytes = extractNumber("sizeBytes");
    build.imageUrl = extractValue("imageUrl");
    build.githubRepo = extractValue("githubRepo");
    build.githubAsset = extractValue("githubAsset");
    build.tagsCsv = extractValue("tags");
    build.javaPath = extractValue("javaPath");
    const bool hasIsFree = jsonStr.find("\"isFree\"") != std::string::npos;
    build.isFree = hasIsFree ? (extractNumber("isFree") != 0) : true;
    build.priceCents = static_cast<int>(extractNumber("priceCents"));
    build.locked = extractNumber("locked") != 0;
    
    return build;
}
static std::string escapeJson(const std::string &value)
{
    std::string out;
    out.reserve(value.size() + 8);
    for (char c : value) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\n";
            break;
        case '\r':
            out += "\r";
            break;
        case '\t':
            out += "\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 32) {
                out += ' ';
            } else {
                out += c;
            }
        }
    }
    return out;
}

static bool writeBuildsJson(const std::string &path, const std::vector<BuildInfo> &builds)
{
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file << "{\n  \"builds\": [\n";
    for (size_t i = 0; i < builds.size(); ++i) {
        const auto &b = builds[i];
        file << "    {\n";
        file << "      \"id\": \"" << escapeJson(b.id) << "\",\n";
        file << "      \"name\": \"" << escapeJson(b.name) << "\",\n";
        file << "      \"description\": \"" << escapeJson(b.description) << "\",\n";
        file << "      \"minecraftVersion\": \"" << escapeJson(b.minecraftVersion) << "\",\n";
        file << "      \"loader\": \"" << escapeJson(b.loader) << "\",\n";
        file << "      \"javaVersion\": " << b.javaVersion << ",\n";
        file << "      \"downloadUrl\": \"" << escapeJson(b.downloadUrl) << "\",\n";
        file << "      \"checksum\": \"" << escapeJson(b.checksum) << "\",\n";
        file << "      \"sizeBytes\": " << b.sizeBytes << ",\n";
        file << "      \"imageUrl\": \"" << escapeJson(b.imageUrl) << "\",\n";
        file << "      \"githubRepo\": \"" << escapeJson(b.githubRepo) << "\",\n";
        file << "      \"githubAsset\": \"" << escapeJson(b.githubAsset) << "\",\n";
        file << "      \"tags\": \"" << escapeJson(b.tagsCsv) << "\",\n";
        file << "      \"javaPath\": \"" << escapeJson(b.javaPath) << "\",\n";
        file << "      \"isFree\": " << (b.isFree ? 1 : 0) << ",\n";
        file << "      \"priceCents\": " << b.priceCents << ",\n";
        file << "      \"locked\": " << (b.locked ? 1 : 0) << "\n";
        file << "    }" << (i + 1 < builds.size() ? "," : "") << "\n";
    }
    file << "  ]\n}\n";

    return file.good();
}


std::vector<BuildInfo> ConfigManager::loadBuilds()
{
    std::vector<BuildInfo> builds;
    
    std::ifstream file(buildsJsonPath);
    if (!file.is_open()) {
        return builds;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    
    // Найдём все объекты внутри массива "builds"
    size_t buildsStart = content.find("\"builds\"");
    if (buildsStart == std::string::npos) return builds;
    
    size_t arrayStart = content.find("[", buildsStart);
    size_t arrayEnd = content.find("]", arrayStart);
    
    if (arrayStart == std::string::npos || arrayEnd == std::string::npos) {
        return builds;
    }
    
    // Разбираем каждый объект {..}
    size_t pos = arrayStart + 1;
    while (pos < arrayEnd) {
        size_t objStart = content.find("{", pos);
        if (objStart == std::string::npos || objStart >= arrayEnd) break;
        
        size_t objEnd = content.find("}", objStart);
        if (objEnd == std::string::npos || objEnd > arrayEnd) break;
        
        std::string objStr = content.substr(objStart, objEnd - objStart + 1);
        BuildInfo build = parseJsonBuild(objStr);
        
        if (!build.id.empty()) {
            builds.push_back(build);
        }
        
        pos = objEnd + 1;
    }
    
    return builds;
}

void ConfigManager::saveBuild(const BuildInfo &build)
{
    auto builds = loadBuilds();
    auto it = std::find_if(builds.begin(), builds.end(), [&](const BuildInfo &b) {
        return b.id == build.id;
    });
    if (it != builds.end()) {
        *it = build;
    } else {
        builds.push_back(build);
    }
    writeBuildsJson(buildsJsonPath, builds);
}

void ConfigManager::removeBuild(const std::string &buildId)
{
    auto builds = loadBuilds();
    builds.erase(std::remove_if(builds.begin(), builds.end(), [&](const BuildInfo &b) {
        return b.id == buildId;
    }), builds.end());
    writeBuildsJson(buildsJsonPath, builds);
}

std::string ConfigManager::getLauncherPath() const
{
    return launcherPath;
}
