#ifndef JAVA_MANAGER_H
#define JAVA_MANAGER_H

#include <string>
#include <vector>

class JavaManager
{
public:
    JavaManager();
    
    std::string findJava(int requiredVersion);
    bool installJava(int version);
    std::vector<std::string> findAllJavaVersions();

private:
    std::string launcherPath;
    
    std::string getExecutablePath() const;
    std::string getJavaPath() const;
    bool isValidJava(const std::string &path, int version);
};

#endif // JAVA_MANAGER_H
