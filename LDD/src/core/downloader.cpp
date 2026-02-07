#include "downloader.h"
#include <iostream>
#include <windows.h>
#include <wininet.h>
#include <vector>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>

#pragma comment(lib, "wininet.lib")

Downloader::Downloader()
{
}

Downloader::~Downloader()
{
}

bool Downloader::downloadFile(const std::string &url, const std::string &savePath, const std::string &extraHeaders)
{
    lastErrorMessage.clear();
    HINTERNET hInternetSession = InternetOpenA("MinecraftLauncher/1.0",
                                               INTERNET_OPEN_TYPE_PRECONFIG,
                                               NULL, NULL, 0);
    if (!hInternetSession) {
        lastErrorMessage = "InternetOpen failed";
        return false;
    }

    const char *headers = extraHeaders.empty() ? nullptr : extraHeaders.c_str();
    DWORD headersLength = extraHeaders.empty() ? 0 : static_cast<DWORD>(extraHeaders.size());

    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_UI;
    if (url.rfind("https://", 0) == 0) {
        flags |= INTERNET_FLAG_SECURE;
    }
    HINTERNET hHttpFile = InternetOpenUrlA(hInternetSession, url.c_str(),
                                           headers, headersLength,
                                           flags, 0);
    if (!hHttpFile) {
        lastErrorMessage = "InternetOpenUrl failed: " + url;
        InternetCloseHandle(hInternetSession);
        return false;
    }

    // Check HTTP status code when available (accept 200/206).
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (HttpQueryInfoA(hHttpFile,
                       HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                       &statusCode, &statusCodeSize, NULL)) {
        if (statusCode != 200 && statusCode != 206) {
            lastErrorMessage = "HTTP error " + std::to_string(statusCode) + ": " + url;
            InternetCloseHandle(hHttpFile);
            InternetCloseHandle(hInternetSession);
            return false;
        }
    }

    const QString filePath = QString::fromUtf8(savePath.c_str());
    QFileInfo info(filePath);
    QDir dir(info.absolutePath());
    if (!dir.exists() && !dir.mkpath(".")) {
        lastErrorMessage = "Failed to create directory for " + savePath;
        InternetCloseHandle(hHttpFile);
        InternetCloseHandle(hInternetSession);
        return false;
    }

    QFile outFile(filePath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        lastErrorMessage = "Failed to write file: " + savePath;
        InternetCloseHandle(hHttpFile);
        InternetCloseHandle(hInternetSession);
        return false;
    }

    DWORD contentLength = 0;
    DWORD contentLengthSize = sizeof(contentLength);
    if (HttpQueryInfoA(hHttpFile,
                       HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
                       &contentLength, &contentLengthSize, NULL)) {
        if (contentLength > 0) {
            outFile.resize(static_cast<qint64>(contentLength));
            outFile.seek(0);
        }
    }

    const DWORD bufferSize = 256 * 1024;
    std::vector<char> buffer(bufferSize);
    DWORD bytesRead = 0;
    long long totalBytes = 0;
    bool ok = true;

    while (true) {
        if (!InternetReadFile(hHttpFile, buffer.data(), bufferSize, &bytesRead)) {
            lastErrorMessage = "InternetReadFile failed";
            ok = false;
            break;
        }
        if (bytesRead == 0) {
            break;
        }

        if (outFile.write(buffer.data(), bytesRead) != bytesRead) {
            lastErrorMessage = "Failed to write file: " + savePath;
            ok = false;
            break;
        }
        totalBytes += bytesRead;

        if (progressCallback) {
            progressCallback(static_cast<long long>(totalBytes));
        }
    }

    outFile.close();
    InternetCloseHandle(hHttpFile);
    InternetCloseHandle(hInternetSession);

    return ok;
}

void Downloader::setProgressCallback(std::function<void(long long)> callback)
{
    progressCallback = callback;
}

void Downloader::cancel()
{
    // TODO: implement cancel if download becomes async.
}

const std::string &Downloader::lastError() const
{
    return lastErrorMessage;
}
