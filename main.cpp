// ArbuzProtect Launcher
// Консольный лаунчер Minecraft 1.21.4 (Fabric) для Windows
//
// Функции:
//  1. Старт   - если игра не скачана - скачивает vanilla 1.21.4 + Fabric Loader,
//               если скачана - сразу запускает
//  2. Настройки - ник и объём памяти (ОЗУ) для игры
//  3. Выход
//
// Требования для сборки (Windows):
//   MinGW-w64:  g++ -std=c++17 -O2 main.cpp -o ArbuzProtectLauncher.exe -lwinhttp -static
//   MSVC:       cl /std:c++17 /EHsc main.cpp /link winhttp.lib
//
// Требования для запуска:
//   - Установленная Java (рекомендуется Java 21) в PATH (команда "java" должна работать)
//   - Windows 10/11 (используется WinHTTP и PowerShell Expand-Archive для нативных библиотек)
//
// Игра скачивается в C:\ArbuzProtect\Game

#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>
#include <cstring>
#include <algorithm>

#include "json.hpp"

#pragma comment(lib, "winhttp.lib")

using json = nlohmann::json;
namespace fs = std::filesystem;

// ==================== ПУТИ ====================
static const std::string BASE_DIR   = "C:\\ArbuzProtect";
static const std::string GAME_DIR   = BASE_DIR + "\\Game";
static const std::string CONFIG_PATH = BASE_DIR + "\\config.json";
static const std::string VERSIONS_DIR  = GAME_DIR + "\\versions";
static const std::string LIBRARIES_DIR = GAME_DIR + "\\libraries";
static const std::string ASSETS_DIR    = GAME_DIR + "\\assets";
static const std::string NATIVES_DIR   = GAME_DIR + "\\natives";
static const std::string MARKER_FILE   = GAME_DIR + "\\.installed_1.21.4";

static const std::string MC_VERSION = "1.21.4";

// ==================== MD5 (для оффлайн UUID) ====================
// Компактная реализация MD5 (по мотивам RFC 1321, public domain)
namespace md5impl {
    typedef uint32_t u32;
    struct MD5Context { u32 a,b,c,d; };

    static inline u32 leftrotate(u32 x, u32 c) { return (x << c) | (x >> (32 - c)); }

    static void md5(const unsigned char* initial_msg, size_t initial_len, unsigned char digest[16]) {
        static const u32 r[] = {
            7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
            5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
            4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
            6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21 };
        static const u32 k[] = {
            0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
            0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
            0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
            0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
            0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
            0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
            0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
            0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391 };

        u32 h0=0x67452301,h1=0xefcdab89,h2=0x98badcfe,h3=0x10325476;

        size_t new_len = ((((initial_len + 8) / 64) + 1) * 64) - 8;
        std::vector<unsigned char> msg(new_len + 8, 0);
        memcpy(msg.data(), initial_msg, initial_len);
        msg[initial_len] = 0x80;
        uint64_t bits_len = (uint64_t)initial_len * 8;
        memcpy(msg.data() + new_len, &bits_len, 8);

        for (size_t offset = 0; offset < msg.size(); offset += 64) {
            u32 w[16];
            for (int i = 0; i < 16; i++) {
                w[i] = (u32)msg[offset + i*4] | ((u32)msg[offset + i*4 + 1] << 8) |
                       ((u32)msg[offset + i*4 + 2] << 16) | ((u32)msg[offset + i*4 + 3] << 24);
            }
            u32 a=h0,b=h1,c=h2,d=h3;
            for (u32 i = 0; i < 64; i++) {
                u32 f,g;
                if (i < 16) { f = (b & c) | (~b & d); g = i; }
                else if (i < 32) { f = (d & b) | (~d & c); g = (5*i + 1) % 16; }
                else if (i < 48) { f = b ^ c ^ d; g = (3*i + 5) % 16; }
                else { f = c ^ (b | ~d); g = (7*i) % 16; }
                u32 temp = d; d = c; c = b;
                b = b + leftrotate(a + f + k[i] + w[g], r[i]);
                a = temp;
            }
            h0 += a; h1 += b; h2 += c; h3 += d;
        }
        memcpy(digest,    &h0, 4);
        memcpy(digest+4,  &h1, 4);
        memcpy(digest+8,  &h2, 4);
        memcpy(digest+12, &h3, 4);
    }
}

// Оффлайн UUID как у ванильного клиента: MD5("OfflinePlayer:<ник>") с выставленными битами версии/варианта
static std::string offlineUuid(const std::string& username) {
    std::string data = "OfflinePlayer:" + username;
    unsigned char digest[16];
    md5impl::md5((const unsigned char*)data.data(), data.size(), digest);
    digest[6] = (digest[6] & 0x0F) | 0x30; // версия 3
    digest[8] = (digest[8] & 0x3F) | 0x80; // вариант RFC4122

    char buf[64];
    snprintf(buf, sizeof(buf),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        digest[0],digest[1],digest[2],digest[3],digest[4],digest[5],digest[6],digest[7],
        digest[8],digest[9],digest[10],digest[11],digest[12],digest[13],digest[14],digest[15]);
    return std::string(buf);
}

// ==================== HTTP (WinHTTP) ====================
struct ParsedUrl {
    std::wstring host;
    std::wstring path;
    INTERNET_PORT port;
    bool https;
};

static ParsedUrl parseUrl(const std::string& url) {
    ParsedUrl r{};
    std::string s = url;
    bool https = s.rfind("https://", 0) == 0;
    std::string rest = https ? s.substr(8) : (s.rfind("http://",0)==0 ? s.substr(7) : s);
    size_t slash = rest.find('/');
    std::string host = (slash == std::string::npos) ? rest : rest.substr(0, slash);
    std::string path = (slash == std::string::npos) ? "/" : rest.substr(slash);
    r.host = std::wstring(host.begin(), host.end());
    r.path = std::wstring(path.begin(), path.end());
    r.port = https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    r.https = https;
    return r;
}

// Скачивает URL целиком в строку (для JSON манифестов)
static bool httpGetString(const std::string& url, std::string& out, int redirectsLeft = 5) {
    ParsedUrl pu = parseUrl(url);
    HINTERNET hSession = WinHttpOpen(L"ArbuzProtectLauncher/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;
    HINTERNET hConnect = WinHttpConnect(hSession, pu.host.c_str(), pu.port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }
    DWORD flags = pu.https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", pu.path.c_str(),
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    bool ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, NULL);

    if (ok) {
        DWORD statusCode = 0, size = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_FLAG_NUMBER | WINHTTP_QUERY_STATUS_CODE,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size, WINHTTP_NO_HEADER_INDEX);

        if ((statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 307) && redirectsLeft > 0) {
            wchar_t locBuf[2048]; DWORD locSize = sizeof(locBuf);
            if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX,
                    locBuf, &locSize, WINHTTP_NO_HEADER_INDEX)) {
                std::wstring wloc(locBuf);
                std::string loc(wloc.begin(), wloc.end());
                WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
                return httpGetString(loc, out, redirectsLeft - 1);
            }
        }

        DWORD dwSize = 0;
        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
            if (dwSize == 0) break;
            std::vector<char> buf(dwSize);
            DWORD dwRead = 0;
            if (!WinHttpReadData(hRequest, buf.data(), dwSize, &dwRead)) break;
            out.append(buf.data(), dwRead);
        } while (dwSize > 0);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ok;
}

// Скачивает URL в файл (создаёт директории при необходимости)
static bool httpDownloadFile(const std::string& url, const std::string& outPath, int redirectsLeft = 5) {
    fs::create_directories(fs::path(outPath).parent_path());

    ParsedUrl pu = parseUrl(url);
    HINTERNET hSession = WinHttpOpen(L"ArbuzProtectLauncher/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;
    HINTERNET hConnect = WinHttpConnect(hSession, pu.host.c_str(), pu.port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); return false; }
    DWORD flags = pu.https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", pu.path.c_str(),
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return false; }

    bool ok = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(hRequest, NULL);

    if (ok) {
        DWORD statusCode = 0, size = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_FLAG_NUMBER | WINHTTP_QUERY_STATUS_CODE,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size, WINHTTP_NO_HEADER_INDEX);

        if ((statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 307) && redirectsLeft > 0) {
            wchar_t locBuf[2048]; DWORD locSize = sizeof(locBuf);
            if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX,
                    locBuf, &locSize, WINHTTP_NO_HEADER_INDEX)) {
                std::wstring wloc(locBuf);
                std::string loc(wloc.begin(), wloc.end());
                WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
                return httpDownloadFile(loc, outPath, redirectsLeft - 1);
            }
        }

        std::ofstream ofs(outPath, std::ios::binary);
        if (!ofs) { ok = false; }
        else {
            DWORD dwSize = 0;
            do {
                dwSize = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
                if (dwSize == 0) break;
                std::vector<char> buf(dwSize);
                DWORD dwRead = 0;
                if (!WinHttpReadData(hRequest, buf.data(), dwSize, &dwRead)) break;
                ofs.write(buf.data(), dwRead);
            } while (dwSize > 0);
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return ok;
}

// ==================== КОНФИГ ====================
struct Config {
    std::string username = "Player";
    int memoryGB = 4;
};

static Config loadConfig() {
    Config cfg;
    if (fs::exists(CONFIG_PATH)) {
        try {
            std::ifstream f(CONFIG_PATH);
            json j; f >> j;
            if (j.contains("username")) cfg.username = j["username"].get<std::string>();
            if (j.contains("memoryGB")) cfg.memoryGB = j["memoryGB"].get<int>();
        } catch (...) {
            // используем значения по умолчанию, если конфиг повреждён
        }
    }
    return cfg;
}

static void saveConfig(const Config& cfg) {
    json j;
    j["username"] = cfg.username;
    j["memoryGB"] = cfg.memoryGB;
    fs::create_directories(BASE_DIR);
    std::ofstream f(CONFIG_PATH);
    f << j.dump(4);
}

// ==================== ПРОВЕРКА ПРАВИЛ БИБЛИОТЕК (только Windows) ====================
static bool ruleAllowsWindows(const json& rulesArr) {
    // Если правил нет - библиотека разрешена
    if (rulesArr.is_null() || !rulesArr.is_array() || rulesArr.empty()) return true;
    bool allowed = false;
    for (auto& rule : rulesArr) {
        std::string action = rule.value("action", "allow");
        bool matches = true;
        if (rule.contains("os")) {
            std::string osName = rule["os"].value("name", "");
            if (!osName.empty() && osName != "windows") matches = false;
        }
        if (rule.contains("features")) {
            // Спец-функции (демо, кастомное разрешение и т.п.) не поддерживаем - пропускаем такие правила
            matches = false;
        }
        if (matches) allowed = (action == "allow");
    }
    return allowed;
}

// ==================== СКАЧИВАНИЕ БИБЛИОТЕК ====================
struct LibResult {
    std::vector<std::string> classpathJars; // абсолютные пути к jar для classpath
    std::vector<std::string> nativeJars;     // jar-файлы с нативными библиотеками (нужно распаковать)
};

static void progressPrint(const std::string& what) {
    std::cout << "  -> " << what << std::endl;
}

static LibResult downloadLibraries(const json& libsArr) {
    LibResult result;
    size_t total = libsArr.size();
    size_t idx = 0;
    for (auto& lib : libsArr) {
        idx++;
        if (lib.contains("rules") && !ruleAllowsWindows(lib["rules"])) continue;
        if (!lib.contains("downloads")) continue;
        auto& downloads = lib["downloads"];
        std::string name = lib.value("name", "?");

        if (downloads.contains("artifact")) {
            auto& art = downloads["artifact"];
            std::string relPath = art.value("path", "");
            std::string url = art.value("url", "");
            if (relPath.empty() || url.empty()) continue;
            std::string outPath = LIBRARIES_DIR + "\\" + relPath;
            std::replace(outPath.begin(), outPath.end(), '/', '\\');
            if (!fs::exists(outPath)) {
                std::cout << "[" << idx << "/" << total << "] Библиотека: " << name << std::endl;
                httpDownloadFile(url, outPath);
            }
            bool isNative = name.find("natives") != std::string::npos;
            if (isNative) result.nativeJars.push_back(outPath);
            else result.classpathJars.push_back(outPath);
        }

        if (downloads.contains("classifiers")) {
            auto& cls = downloads["classifiers"];
            if (cls.contains("natives-windows")) {
                auto& art = cls["natives-windows"];
                std::string relPath = art.value("path", "");
                std::string url = art.value("url", "");
                if (!relPath.empty() && !url.empty()) {
                    std::string outPath = LIBRARIES_DIR + "\\" + relPath;
                    std::replace(outPath.begin(), outPath.end(), '/', '\\');
                    if (!fs::exists(outPath)) {
                        std::cout << "[" << idx << "/" << total << "] Нативная библиотека: " << name << std::endl;
                        httpDownloadFile(url, outPath);
                    }
                    result.nativeJars.push_back(outPath);
                }
            }
        }
    }
    return result;
}

// Распаковка нативных jar-файлов в папку natives через PowerShell (Expand-Archive)
static void extractNatives(const std::vector<std::string>& nativeJars) {
    fs::create_directories(NATIVES_DIR);
    for (auto& jar : nativeJars) {
        if (!fs::exists(jar)) continue;
        std::string tmpZip = jar + ".zip";
        try { fs::copy_file(jar, tmpZip, fs::copy_options::overwrite_existing); }
        catch (...) { continue; }
        std::string cmd = "powershell -NoProfile -Command \"Expand-Archive -Path '" + tmpZip +
            "' -DestinationPath '" + NATIVES_DIR + "' -Force\" >NUL 2>&1";
        system(cmd.c_str());
        std::error_code ec;
        fs::remove(tmpZip, ec);
    }
    // Убираем ненужную META-INF из папки с нативами
    std::error_code ec;
    fs::remove_all(NATIVES_DIR + "\\META-INF", ec);
}

// ==================== СКАЧИВАНИЕ ASSETS ====================
static void downloadAssets(const json& versionJson) {
    if (!versionJson.contains("assetIndex")) return;
    auto& ai = versionJson["assetIndex"];
    std::string assetIndexId = ai.value("id", "legacy");
    std::string indexUrl = ai.value("url", "");
    if (indexUrl.empty()) return;

    std::string indexPath = ASSETS_DIR + "\\indexes\\" + assetIndexId + ".json";
    fs::create_directories(fs::path(indexPath).parent_path());
    if (!fs::exists(indexPath)) {
        std::cout << "Скачивание индекса ассетов..." << std::endl;
        httpDownloadFile(indexUrl, indexPath);
    }

    std::ifstream f(indexPath);
    json idx; f >> idx;
    if (!idx.contains("objects")) return;
    auto& objects = idx["objects"];
    size_t total = objects.size(), i = 0;
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        i++;
        std::string hash = it.value()["hash"].get<std::string>();
        std::string prefix = hash.substr(0, 2);
        std::string outPath = ASSETS_DIR + "\\objects\\" + prefix + "\\" + hash;
        if (fs::exists(outPath)) continue;
        std::string url = "https://resources.download.minecraft.net/" + prefix + "/" + hash;
        if (i % 50 == 0 || i == total)
            std::cout << "Ассеты: " << i << " / " << total << std::endl;
        httpDownloadFile(url, outPath);
    }
}

// ==================== УСТАНОВКА MINECRAFT + FABRIC ====================
static bool isInstalled() {
    return fs::exists(MARKER_FILE);
}

// Сохраняем сведения об установленной версии между установкой и запуском
static std::string g_fabricVersionId;
static std::string g_fabricLoaderVersion;

static bool installMinecraftFabric() {
    fs::create_directories(GAME_DIR);
    fs::create_directories(VERSIONS_DIR);
    fs::create_directories(LIBRARIES_DIR);
    fs::create_directories(ASSETS_DIR);

    std::cout << "=== Установка Minecraft " << MC_VERSION << " + Fabric ===" << std::endl;

    // 1. Манифест версий
    std::cout << "Получение списка версий Minecraft..." << std::endl;
    std::string manifestStr;
    if (!httpGetString("https://launchermeta.mojang.com/mc/game/version_manifest_v2.json", manifestStr)) {
        std::cout << "Ошибка: не удалось получить манифест версий (проверьте интернет-соединение)." << std::endl;
        return false;
    }
    json manifest = json::parse(manifestStr);
    std::string versionUrl;
    for (auto& v : manifest["versions"]) {
        if (v["id"] == MC_VERSION) { versionUrl = v["url"].get<std::string>(); break; }
    }
    if (versionUrl.empty()) {
        std::cout << "Ошибка: версия " << MC_VERSION << " не найдена в манифесте." << std::endl;
        return false;
    }

    // 2. JSON версии Minecraft
    std::string versionJsonStr;
    if (!httpGetString(versionUrl, versionJsonStr)) {
        std::cout << "Ошибка получения данных версии." << std::endl;
        return false;
    }
    json versionJson = json::parse(versionJsonStr);
    fs::create_directories(VERSIONS_DIR + "\\" + MC_VERSION);
    { std::ofstream f(VERSIONS_DIR + "\\" + MC_VERSION + "\\" + MC_VERSION + ".json"); f << versionJson.dump(2); }

    // 3. Клиентский jar
    std::string clientJarPath = VERSIONS_DIR + "\\" + MC_VERSION + "\\" + MC_VERSION + ".jar";
    if (!fs::exists(clientJarPath)) {
        std::cout << "Скачивание клиента игры..." << std::endl;
        std::string clientUrl = versionJson["downloads"]["client"]["url"].get<std::string>();
        httpDownloadFile(clientUrl, clientJarPath);
    }

    // 4. Библиотеки ванильной версии
    std::cout << "Скачивание библиотек Minecraft..." << std::endl;
    LibResult vanillaLibs = downloadLibraries(versionJson["libraries"]);

    // 5. Ассеты
    std::cout << "Скачивание ассетов (звуки, текстуры) - это может занять время..." << std::endl;
    downloadAssets(versionJson);

    // 6. Fabric loader - список версий
    std::cout << "Получение версий Fabric Loader..." << std::endl;
    std::string loaderListStr;
    if (!httpGetString("https://meta.fabricmc.net/v2/versions/loader/" + MC_VERSION, loaderListStr)) {
        std::cout << "Ошибка: не удалось получить версии Fabric Loader." << std::endl;
        return false;
    }
    json loaderList = json::parse(loaderListStr);
    if (!loaderList.is_array() || loaderList.empty()) {
        std::cout << "Ошибка: Fabric не поддерживает версию " << MC_VERSION << "." << std::endl;
        return false;
    }
    std::string loaderVersion;
    for (auto& entry : loaderList) {
        if (entry["loader"].value("stable", false)) { loaderVersion = entry["loader"]["version"].get<std::string>(); break; }
    }
    if (loaderVersion.empty()) loaderVersion = loaderList[0]["loader"]["version"].get<std::string>();
    g_fabricLoaderVersion = loaderVersion;
    g_fabricVersionId = "fabric-loader-" + loaderVersion + "-" + MC_VERSION;

    // 7. Fabric profile json
    std::cout << "Скачивание профиля Fabric (" << loaderVersion << ")..." << std::endl;
    std::string profileStr;
    std::string profileUrl = "https://meta.fabricmc.net/v2/versions/loader/" + MC_VERSION + "/" + loaderVersion + "/profile/json";
    if (!httpGetString(profileUrl, profileStr)) {
        std::cout << "Ошибка получения профиля Fabric." << std::endl;
        return false;
    }
    json fabricJson = json::parse(profileStr);
    fs::create_directories(VERSIONS_DIR + "\\" + g_fabricVersionId);
    { std::ofstream f(VERSIONS_DIR + "\\" + g_fabricVersionId + "\\" + g_fabricVersionId + ".json"); f << fabricJson.dump(2); }

    // 8. Библиотеки Fabric
    std::cout << "Скачивание библиотек Fabric..." << std::endl;
    LibResult fabricLibs = downloadLibraries(fabricJson["libraries"]);

    // 9. Распаковка нативных библиотек
    std::cout << "Распаковка нативных библиотек..." << std::endl;
    extractNatives(vanillaLibs.nativeJars);

    // 10. Маркер успешной установки
    { std::ofstream f(MARKER_FILE); f << g_fabricVersionId; }

    std::cout << "=== Установка завершена ===" << std::endl;
    return true;
}

// Подставляет плейсхолдеры ${...} в аргументах запуска
static std::string substitutePlaceholders(std::string arg, const std::vector<std::pair<std::string,std::string>>& values) {
    for (auto& kv : values) {
        std::string placeholder = "${" + kv.first + "}";
        size_t pos;
        while ((pos = arg.find(placeholder)) != std::string::npos) {
            arg.replace(pos, placeholder.length(), kv.second);
        }
    }
    return arg;
}

// Достаёт аргументы (game/jvm) из json "arguments", поддерживая простые строки и условные объекты
static std::vector<std::string> extractArgs(const json& argsArr) {
    std::vector<std::string> result;
    if (argsArr.is_null() || !argsArr.is_array()) return result;
    for (auto& item : argsArr) {
        if (item.is_string()) {
            result.push_back(item.get<std::string>());
        } else if (item.is_object()) {
            if (item.contains("rules") && !ruleAllowsWindows(item["rules"])) continue;
            if (item.contains("features")) continue; // не поддерживаем спец-функции
            if (item.contains("value")) {
                if (item["value"].is_string()) result.push_back(item["value"].get<std::string>());
                else if (item["value"].is_array())
                    for (auto& v : item["value"]) result.push_back(v.get<std::string>());
            }
        }
    }
    return result;
}

// ==================== ЗАПУСК ИГРЫ ====================
static void launchGame(const Config& cfg) {
    if (g_fabricVersionId.empty()) {
        // Игра уже была установлена ранее - читаем id версии из маркера
        std::ifstream f(MARKER_FILE);
        std::getline(f, g_fabricVersionId);
    }
    std::string fabricJsonPath = VERSIONS_DIR + "\\" + g_fabricVersionId + "\\" + g_fabricVersionId + ".json";
    std::string vanillaJsonPath = VERSIONS_DIR + "\\" + MC_VERSION + "\\" + MC_VERSION + ".json";
    if (!fs::exists(fabricJsonPath) || !fs::exists(vanillaJsonPath)) {
        std::cout << "Ошибка: файлы установленной версии не найдены. Переустановите игру (удалите папку Game)." << std::endl;
        return;
    }

    json fabricJson, vanillaJson;
    { std::ifstream f(fabricJsonPath); f >> fabricJson; }
    { std::ifstream f(vanillaJsonPath); f >> vanillaJson; }

    std::string clientJarPath = VERSIONS_DIR + "\\" + MC_VERSION + "\\" + MC_VERSION + ".jar";

    // Собираем classpath: ванильные библиотеки + fabric библиотеки + клиентский jar
    LibResult vanillaLibs = downloadLibraries(vanillaJson["libraries"]);   // уже скачаны - только соберёт пути
    LibResult fabricLibs  = downloadLibraries(fabricJson["libraries"]);

    std::vector<std::string> classpath;
    for (auto& p : vanillaLibs.classpathJars) classpath.push_back(p);
    for (auto& p : fabricLibs.classpathJars) classpath.push_back(p);
    classpath.push_back(clientJarPath);

    std::string classpathStr;
    for (size_t i = 0; i < classpath.size(); i++) {
        classpathStr += classpath[i];
        if (i + 1 < classpath.size()) classpathStr += ";";
    }

    std::string mainClass = fabricJson.value("mainClass", "net.fabricmc.loader.impl.launch.knot.KnotClient");
    std::string assetIndexId = vanillaJson["assetIndex"].value("id", MC_VERSION);
    std::string uuid = offlineUuid(cfg.username);

    std::vector<std::pair<std::string,std::string>> values = {
        {"auth_player_name", cfg.username},
        {"version_name", MC_VERSION},
        {"game_directory", GAME_DIR},
        {"assets_root", ASSETS_DIR},
        {"assets_index_name", assetIndexId},
        {"auth_uuid", uuid},
        {"auth_access_token", "0"},
        {"clientid", "-"},
        {"auth_xuid", "-"},
        {"user_type", "legacy"},
        {"version_type", "release"},
        {"natives_directory", NATIVES_DIR},
        {"launcher_name", "ArbuzProtectLauncher"},
        {"launcher_version", "1.0"},
        {"classpath", classpathStr}
    };

    std::vector<std::string> gameArgs = extractArgs(vanillaJson.value("arguments", json()).value("game", json()));
    if (fabricJson.contains("arguments") && fabricJson["arguments"].contains("game")) {
        auto extra = extractArgs(fabricJson["arguments"]["game"]);
        for (auto& a : extra) gameArgs.push_back(a);
    }
    std::vector<std::string> jvmArgs = extractArgs(vanillaJson.value("arguments", json()).value("jvm", json()));
    if (fabricJson.contains("arguments") && fabricJson["arguments"].contains("jvm")) {
        auto extra = extractArgs(fabricJson["arguments"]["jvm"]);
        for (auto& a : extra) jvmArgs.push_back(a);
    }

    // Формируем командную строку
    std::ostringstream cmd;
    cmd << "java -Xmx" << cfg.memoryGB << "G -Xms" << cfg.memoryGB << "G ";
    cmd << "-Djava.library.path=\"" << NATIVES_DIR << "\" ";
    for (auto& a : jvmArgs) {
        std::string sub = substitutePlaceholders(a, values);
        if (sub.find("-cp") != std::string::npos || sub.find("-classpath") != std::string::npos) continue; // добавим сами
        cmd << sub << " ";
    }
    cmd << "-cp \"" << classpathStr << "\" ";
    cmd << mainClass << " ";
    for (auto& a : gameArgs) {
        cmd << substitutePlaceholders(a, values) << " ";
    }

    std::cout << "Запуск Minecraft " << MC_VERSION << " (Fabric " << g_fabricLoaderVersion << ")..." << std::endl;

    std::string cmdline = cmd.str();
    STARTUPINFOA si; PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::vector<char> cmdBuf(cmdline.begin(), cmdline.end());
    cmdBuf.push_back('\0');

    if (CreateProcessA(NULL, cmdBuf.data(), NULL, NULL, FALSE, 0, NULL, GAME_DIR.c_str(), &si, &pi)) {
        std::cout << "Игра запущена." << std::endl;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        std::cout << "Не удалось запустить java. Убедитесь, что Java установлена и доступна в PATH." << std::endl;
    }
}

// ==================== МЕНЮ ====================
static void clearScreen() { system("cls"); }

static void printHeader() {
    std::cout << "=====================================\n";
    std::cout << "        ArbuzProtect Launcher\n";
    std::cout << "        Minecraft " << MC_VERSION << " + Fabric\n";
    std::cout << "=====================================\n\n";
}

static void settingsMenu(Config& cfg) {
    while (true) {
        clearScreen();
        printHeader();
        std::cout << "Настройки:\n";
        std::cout << "  Ник: " << cfg.username << "\n";
        std::cout << "  Память: " << cfg.memoryGB << " GB\n\n";
        std::cout << "1. Изменить ник\n";
        std::cout << "2. Изменить память\n";
        std::cout << "3. Назад в главное меню\n";
        std::cout << "\nВыбор: ";

        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "1") {
            std::cout << "Введите новый ник (3-16 символов, буквы/цифры/_): ";
            std::string nick;
            std::getline(std::cin, nick);
            if (nick.length() >= 3 && nick.length() <= 16) {
                cfg.username = nick;
                saveConfig(cfg);
                std::cout << "Ник сохранён.\n";
            } else {
                std::cout << "Некорректный ник.\n";
            }
            std::cout << "Нажмите Enter..."; std::cin.get();
        } else if (choice == "2") {
            std::cout << "Введите объём памяти в GB (1-32): ";
            std::string memStr;
            std::getline(std::cin, memStr);
            try {
                int mem = std::stoi(memStr);
                if (mem >= 1 && mem <= 32) {
                    cfg.memoryGB = mem;
                    saveConfig(cfg);
                    std::cout << "Память сохранена.\n";
                } else {
                    std::cout << "Значение должно быть от 1 до 32.\n";
                }
            } catch (...) {
                std::cout << "Некорректное значение.\n";
            }
            std::cout << "Нажмите Enter..."; std::cin.get();
        } else if (choice == "3") {
            return;
        }
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    fs::create_directories(BASE_DIR);
    fs::create_directories(GAME_DIR);

    Config cfg = loadConfig();
    saveConfig(cfg); // создаём config.json при первом запуске

    while (true) {
        clearScreen();
        printHeader();
        std::cout << "1. Старт\n";
        std::cout << "2. Настройки\n";
        std::cout << "3. Выход\n";
        std::cout << "\nВыбор: ";

        std::string choice;
        std::getline(std::cin, choice);

        if (choice == "1") {
            clearScreen();
            printHeader();
            if (!isInstalled()) {
                std::cout << "Minecraft не установлен. Начинаю установку...\n\n";
                if (installMinecraftFabric()) {
                    launchGame(cfg);
                } else {
                    std::cout << "\nУстановка не удалась.\n";
                }
            } else {
                std::cout << "Minecraft уже установлен. Запуск...\n\n";
                launchGame(cfg);
            }
            std::cout << "\nНажмите Enter для возврата в меню...";
            std::cin.get();
        } else if (choice == "2") {
            settingsMenu(cfg);
        } else if (choice == "3") {
            std::cout << "До встречи!\n";
            break;
        }
    }
    return 0;
}
