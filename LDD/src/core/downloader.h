#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <string>
#include <functional>

class Downloader
{
public:
    Downloader();
    ~Downloader();
    
    bool downloadFile(const std::string &url, const std::string &savePath, const std::string &extraHeaders = "");
    void setProgressCallback(std::function<void(long long)> callback);
    void cancel();
    const std::string &lastError() const;

private:
    std::function<void(long long)> progressCallback;
    std::string lastErrorMessage;
};

#endif // DOWNLOADER_H
