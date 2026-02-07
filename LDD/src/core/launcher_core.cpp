#include "launcher_core.h"
#include "downloader.h"
#include <windows.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

namespace {

bool isSafePathComponent(const std::string &value)
{
    if (value.empty()) {
        return false;
    }
    if (value.find("..") != std::string::npos) {
        return false;
    }
    for (char c : value) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc < 32 || c == '\\' || c == '/' || c == ':' || c == '"' || c == '\'') {
            return false;
        }
    }
    return true;
}

std::string trim(const std::string &s)
{
    const char *whitespace = " \t\r\n";
    size_t start = s.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    size_t end = s.find_last_not_of(whitespace);
    return s.substr(start, end - start + 1);
}

std::string toLower(const std::string &s)
{
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

std::string normalizeSha256(const std::string &value)
{
    std::string out = toLower(trim(value));
    const std::string prefixA = "sha256:";
    const std::string prefixB = "sha256=";
    if (out.rfind(prefixA, 0) == 0) {
        out = trim(out.substr(prefixA.size()));
    } else if (out.rfind(prefixB, 0) == 0) {
        out = trim(out.substr(prefixB.size()));
    }
    if (out.empty() || out.size() != 64) {
        return "";
    }
    if (!std::all_of(out.begin(), out.end(), [](unsigned char c) {
            return std::isxdigit(c) != 0;
        })) {
        return "";
    }
    return out;
}

bool fileStartsWithZipSignature(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }
    unsigned char sig[4] = {0, 0, 0, 0};
    in.read(reinterpret_cast<char *>(sig), sizeof(sig));
    if (in.gcount() < 4) {
        return false;
    }
    if (sig[0] != 'P' || sig[1] != 'K') {
        return false;
    }
    // PK 03 04 (local header), PK 05 06 (empty archive), PK 07 08 (spanned)
    if (sig[2] == 3 && sig[3] == 4) return true;
    if (sig[2] == 5 && sig[3] == 6) return true;
    if (sig[2] == 7 && sig[3] == 8) return true;
    return false;
}

bool getFileSize(const std::string &path, uintmax_t &sizeOut)
{
    std::error_code ec;
    sizeOut = std::filesystem::file_size(path, ec);
    return !ec;
}

void removeNoThrow(const std::string &path)
{
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

std::string bytesToHex(const std::vector<unsigned char> &bytes)
{
    static const char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (unsigned char b : bytes) {
        out.push_back(kHex[(b >> 4) & 0xF]);
        out.push_back(kHex[b & 0xF]);
    }
    return out;
}

bool sha256File(const std::string &path, std::string &outHex)
{
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    DWORD cbData = 0;
    DWORD cbHash = 0;
    DWORD cbHashObject = 0;
    bool ok = false;

    if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0))) {
        return false;
    }

    if (!BCRYPT_SUCCESS(BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH,
                                          reinterpret_cast<PUCHAR>(&cbHashObject),
                                          sizeof(cbHashObject), &cbData, 0))) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    if (!BCRYPT_SUCCESS(BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH,
                                          reinterpret_cast<PUCHAR>(&cbHash),
                                          sizeof(cbHash), &cbData, 0))) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    std::vector<UCHAR> hashObject(cbHashObject);
    std::vector<UCHAR> hash(cbHash);

    if (!BCRYPT_SUCCESS(BCryptCreateHash(hAlg, &hHash, hashObject.data(), cbHashObject,
                                         NULL, 0, 0))) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    std::vector<char> buffer(1024 * 1024);
    while (in.good()) {
        in.read(buffer.data(), buffer.size());
        std::streamsize readBytes = in.gcount();
        if (readBytes > 0) {
            if (!BCRYPT_SUCCESS(BCryptHashData(hHash,
                                               reinterpret_cast<PUCHAR>(buffer.data()),
                                               static_cast<ULONG>(readBytes), 0))) {
                in.close();
                BCryptDestroyHash(hHash);
                BCryptCloseAlgorithmProvider(hAlg, 0);
                return false;
            }
        }
    }

    if (in.bad()) {
        in.close();
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    if (!BCRYPT_SUCCESS(BCryptFinishHash(hHash, hash.data(), cbHash, 0))) {
        in.close();
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    in.close();
    outHex = bytesToHex(hash);
    ok = true;

    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    return ok;
}

bool validateZipFile(const BuildInfo &build, const std::string &zipPath)
{
    if (!fileStartsWithZipSignature(zipPath)) {
        std::cerr << "X Downloaded file is not a ZIP archive. Check direct download URL.\n";
        return false;
    }

    uintmax_t sizeBytes = 0;
    if (!getFileSize(zipPath, sizeBytes)) {
        std::cerr << "X Unable to read downloaded file size.\n";
        return false;
    }
    if (sizeBytes == 0) {
        std::cerr << "X Downloaded file is empty.\n";
        return false;
    }
    if (build.sizeBytes > 0 && sizeBytes != static_cast<uintmax_t>(build.sizeBytes)) {
        std::cerr << "X Size mismatch: expected " << build.sizeBytes
                  << ", got " << static_cast<long long>(sizeBytes) << ".\n";
        return false;
    }

    if (!build.checksum.empty()) {
        std::string expected = normalizeSha256(build.checksum);
        if (expected.empty()) {
            std::cerr << "X Checksum must be SHA-256 (64 hex chars).\n";
            return false;
        }
        std::string actual;
        if (!sha256File(zipPath, actual)) {
            std::cerr << "X Failed to compute SHA-256 for downloaded file.\n";
            return false;
        }
        if (actual != expected) {
            std::cerr << "X Checksum mismatch.\n";
            return false;
        }
    }

    return true;
}

bool writeExtractScript(const std::string &scriptPath)
{
    std::ofstream out(scriptPath, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }

    out << "param(\n";
    out << "    [Parameter(Mandatory=$true)][string]$ZipPath,\n";
    out << "    [Parameter(Mandatory=$true)][string]$DestPath\n";
    out << ")\n";
    out << "$ErrorActionPreference = 'Stop'\n";
    out << "Add-Type -AssemblyName 'System.IO.Compression.FileSystem'\n";
    out << "$zipFull = [System.IO.Path]::GetFullPath($ZipPath)\n";
    out << "$destFull = [System.IO.Path]::GetFullPath($DestPath)\n";
    out << "if (-not $destFull.EndsWith([System.IO.Path]::DirectorySeparatorChar)) { $destFull += [System.IO.Path]::DirectorySeparatorChar }\n";
    out << "$fs = [System.IO.File]::OpenRead($zipFull)\n";
    out << "try {\n";
    out << "  $archive = New-Object System.IO.Compression.ZipArchive($fs)\n";
    out << "  foreach($entry in $archive.Entries) {\n";
    out << "    if ([string]::IsNullOrEmpty($entry.FullName)) { continue }\n";
    out << "    $target = [System.IO.Path]::GetFullPath([System.IO.Path]::Combine($destFull, $entry.FullName))\n";
    out << "    if (-not $target.StartsWith($destFull, [System.StringComparison]::OrdinalIgnoreCase)) {\n";
    out << "      throw \"ZipSlip detected: $($entry.FullName)\"\n";
    out << "    }\n";
    out << "  }\n";
    out << "} finally {\n";
    out << "  if ($archive) { $archive.Dispose() }\n";
    out << "  $fs.Dispose()\n";
    out << "}\n";
    out << "[System.IO.Compression.ZipFile]::ExtractToDirectory($zipFull, $destFull, $true)\n";

    return out.good();
}

bool extractZipSafe(const std::string &zipPath, const std::string &extractPath)
{
    std::error_code ec;
    std::filesystem::create_directories(extractPath, ec);
    if (ec) {
        std::cerr << "X Failed to create extract directory.\n";
        return false;
    }

    std::string scriptPath = extractPath + "\\_extract.ps1";
    if (!writeExtractScript(scriptPath)) {
        std::cerr << "X Failed to write extraction script.\n";
        return false;
    }

    std::string command = "powershell -NoProfile -ExecutionPolicy Bypass -File \"" + scriptPath +
                          "\" -ZipPath \"" + zipPath + "\" -DestPath \"" + extractPath + "\"";

    int result = system(command.c_str());
    removeNoThrow(scriptPath);

    if (result != 0) {
        std::cerr << "X Extraction failed (code: " << result << ").\n";
        return false;
    }

    return true;
}

} // namespace

std::string LauncherCore::getExecutablePath() const
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string fullPath(path);
    return fullPath.substr(0, fullPath.find_last_of("\\/"));
}

LauncherCore::LauncherCore()
{
    launcherPath = getExecutablePath();
}

std::string LauncherCore::getGamePath() const
{
    return launcherPath + "\\minecraft";
}

std::string LauncherCore::getVersionPath(const std::string &version) const
{
    return getGamePath() + "\\versions\\" + version;
}

bool LauncherCore::extractZip(const std::string &zipPath, const std::string &extractPath)
{
    std::cout << "Extracting archive...\n";
    return extractZipSafe(zipPath, extractPath);
}

bool LauncherCore::downloadBuild(const BuildInfo &build)
{
    if (!isSafePathComponent(build.id)) {
        std::cerr << "X Invalid build id.\n";
        return false;
    }
    if (build.downloadUrl.empty()) {
        std::cerr << "X Missing download URL.\n";
        return false;
    }

    std::string buildPath = getGamePath() + "\\" + build.id;
    std::filesystem::create_directories(buildPath);

    std::string zipPath = buildPath + "\\build.zip";
    std::string tempPath = zipPath + ".part";
    std::string extractMarker = buildPath + "\\.extracted";

    auto validateExistingZip = [&]() -> bool {
        if (!std::filesystem::exists(zipPath)) {
            return false;
        }
        if (!validateZipFile(build, zipPath)) {
            removeNoThrow(zipPath);
            removeNoThrow(extractMarker);
            return false;
        }
        return true;
    };

    if (!validateExistingZip()) {
        if (std::filesystem::exists(tempPath)) {
            removeNoThrow(tempPath);
        }

        if (build.sizeBytes > 0) {
            std::cout << "Downloading build (" << (build.sizeBytes / 1000000) << " MB)...\n";
        } else {
            std::cout << "Downloading build...\n";
        }

        Downloader downloader;
        downloader.setProgressCallback([](int mb) {
            std::cout << "   Downloaded: " << mb << " MB\r" << std::flush;
        });

        if (!downloader.downloadFile(build.downloadUrl, tempPath)) {
            std::cerr << "X Download failed.\n";
            removeNoThrow(tempPath);
            return false;
        }

        std::cout << "\nDownload finished.\n";

        if (!validateZipFile(build, tempPath)) {
            removeNoThrow(tempPath);
            return false;
        }

        std::error_code ec;
        std::filesystem::rename(tempPath, zipPath, ec);
        if (ec) {
            std::cerr << "X Failed to move downloaded file.\n";
            removeNoThrow(tempPath);
            return false;
        }
    }

    if (!std::filesystem::exists(extractMarker)) {
        if (!extractZip(zipPath, buildPath)) {
            return false;
        }
        std::ofstream marker(extractMarker, std::ios::binary);
        if (!marker.is_open()) {
            std::cerr << "X Failed to write extraction marker.\n";
            return false;
        }
        marker << "ok";
        marker.close();
    }

    return true;
}

bool LauncherCore::launchGame(const BuildInfo &build, const std::string &javaPath)
{
    std::string buildPath = getGamePath() + "\\" + build.id;

    // Command to launch Minecraft
    std::string command = "cmd /c \"\"" + javaPath + "\" -Xmx2G -Xms2G ";
    command += "-Djava.library.path=\"" + buildPath + "\\natives\" ";
    command += "-cp \"" + buildPath + "\\*\" ";
    command += "net.minecraft.client.main.Main\"";

    std::cout << "\nLaunching Minecraft...\n";
    std::cout << "Command: " << command << "\n\n";

    int result = system(command.c_str());
    return result == 0;
}
