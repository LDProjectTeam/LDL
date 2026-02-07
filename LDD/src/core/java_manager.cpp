#include "java_manager.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cctype>

namespace {

int parseJavaMajorVersion(const std::string &text)
{
    size_t i = text.find_first_of("0123456789");
    if (i == std::string::npos) {
        return 0;
    }

    int first = 0;
    while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
        first = first * 10 + (text[i] - '0');
        ++i;
    }

    if (first == 1 && i < text.size() && text[i] == '.') {
        ++i;
        int second = 0;
        bool has_second = false;
        while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
            second = second * 10 + (text[i] - '0');
            ++i;
            has_second = true;
        }
        if (has_second) {
            return second;
        }
    }

    return first;
}

int getJavaMajorVersion(const std::string &javaPath)
{
    std::string command = "\"" + javaPath + "\" -version 2>&1";
    FILE *pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        return 0;
    }

    std::string output;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    _pclose(pipe);

    return parseJavaMajorVersion(output);
}

} // namespace

std::string JavaManager::getExecutablePath() const
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string fullPath(path);
    return fullPath.substr(0, fullPath.find_last_of("\\/"));
}

JavaManager::JavaManager()
{
    launcherPath = getExecutablePath();
}

std::string JavaManager::getJavaPath() const
{
    return launcherPath + "\\java";
}

std::string JavaManager::findJava(int requiredVersion)
{
    // Проверяем локальную папку лаунчера
    std::string localJava = getJavaPath() + "\\bin\\java.exe";
    if (isValidJava(localJava, requiredVersion)) {
        return localJava;
    }
    
    // Проверяем системную Java через PATH
    FILE* pipe = _popen("where java.exe 2>nul", "r");
    if (pipe) {
        char path[MAX_PATH];
        while (fgets(path, sizeof(path), pipe) != NULL) {
            // Trim line breaks.
            std::string javaPath(path);
            while (!javaPath.empty() && (javaPath.back() == '\n' || javaPath.back() == '\r')) {
                javaPath.pop_back();
            }
            if (javaPath.empty()) {
                continue;
            }
            if (isValidJava(javaPath, requiredVersion)) {
                _pclose(pipe);

                std::cout << "Java found in system: " << javaPath << "\n";
                return javaPath;
            }
        }
        _pclose(pipe);
    }
    
    // Проверяем реестр Windows (старый метод)
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                      "SOFTWARE\\JavaSoft\\Java Runtime Environment", 
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        char version[256];
        DWORD size = sizeof(version);
        
        if (RegQueryValueExA(hKey, "CurrentVersion", NULL, NULL, 
                            (LPBYTE)version, &size) == ERROR_SUCCESS) {
            
            std::string keyPath = "SOFTWARE\\JavaSoft\\Java Runtime Environment\\";
            keyPath += version;
            
            HKEY hVersionKey;
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath.c_str(), 
                             0, KEY_READ, &hVersionKey) == ERROR_SUCCESS) {
                
                char javaPath[MAX_PATH];
                size = sizeof(javaPath);
                
                if (RegQueryValueExA(hVersionKey, "JavaHome", NULL, NULL, 
                                    (LPBYTE)javaPath, &size) == ERROR_SUCCESS) {
                    
                    std::string result = std::string(javaPath) + "\\bin\\java.exe";
                    RegCloseKey(hVersionKey);
                    RegCloseKey(hKey);
                    if (isValidJava(result, requiredVersion)) {
                        return result;
                    }
                }
                RegCloseKey(hVersionKey);
            }
        }
        
        RegCloseKey(hKey);
    }
    
    return "";
}

bool JavaManager::installJava(int version)
{
    std::cout << "WARN Установка Java требует ручного вмешательства\n";
    std::cout << "Загрузите Java " << version << " отсюда:\n";
    std::cout << "https://adoptium.net/ (рекомендуется)\n";
    std::cout << "или\n";
    std::cout << "https://www.oracle.com/java/technologies/downloads/\n";
    std::cout << "И установите в папку: " << getJavaPath() << "\n";
    
    return false;
}

std::vector<std::string> JavaManager::findAllJavaVersions()
{
    std::vector<std::string> versions;
    // Реализация позже
    return versions;
}

bool JavaManager::isValidJava(const std::string &path, int version)
{
    if (!std::ifstream(path).good()) {
        return false;
    }
    if (version <= 0) {
        return true;
    }
    int major = getJavaMajorVersion(path);
    return major == version;
}
