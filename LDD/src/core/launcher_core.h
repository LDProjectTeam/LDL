#ifndef LAUNCHER_CORE_H
#define LAUNCHER_CORE_H

#include <string>
#include "build_info.h"

class LauncherCore
{
public:
    LauncherCore();
    
    bool launchGame(const BuildInfo &build, const std::string &javaPath);
    bool downloadBuild(const BuildInfo &build);

private:
    std::string launcherPath;
    
    std::string getExecutablePath() const;
    std::string getGamePath() const;
    std::string getVersionPath(const std::string &version) const;
    bool extractZip(const std::string &zipPath, const std::string &extractPath);
};

#endif // LAUNCHER_CORE_H
