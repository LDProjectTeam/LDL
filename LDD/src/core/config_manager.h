#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <vector>
#include <string>
#include "build_info.h"

class ConfigManager
{
public:
    ConfigManager();
    
    std::vector<BuildInfo> loadBuilds();
    void saveBuild(const BuildInfo &build);
    void removeBuild(const std::string &buildId);
    std::string getLauncherPath() const;

private:
    std::string launcherPath;
    std::string buildsJsonPath;
    
    std::string getExecutablePath() const;
};

#endif // CONFIG_MANAGER_H
