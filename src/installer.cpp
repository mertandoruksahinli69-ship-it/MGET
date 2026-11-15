#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cwctype>
#include <iostream>
#include <cctype>

namespace fs = std::filesystem;
namespace {
constexpr int PAYLOAD_ID = 101;
constexpr const char* INSTALLER_VERSION = "0.1.0-beta.2";

std::wstring getEnv(const wchar_t* name) {
    const DWORD n = GetEnvironmentVariableW(name, nullptr, 0);
    if (!n) throw std::runtime_error("Missing environment variable");
    std::wstring value(n, L'\0');
    const DWORD written = GetEnvironmentVariableW(name, value.data(), n);
    if (written == 0 || written >= n)
        throw std::runtime_error("Cannot read environment variable");
    value.resize(written);
    return value;
}
std::wstring fromUtf8(const std::string& input) {
    if (input.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                          input.data(), (int)input.size(), nullptr, 0);
    if (!count) throw std::runtime_error("Please enter the folder path in UTF-8");
    std::wstring output(count, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input.data(), (int)input.size(),
                        output.data(), count);
    return output;
}
std::string toUtf8(const std::wstring& input) {
    if (input.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, 0, input.data(), (int)input.size(),
                                          nullptr, 0, nullptr, nullptr);
    if (!count) throw std::runtime_error("Unicode conversion failed");
    std::string output(count, '\0');
    WideCharToMultiByte(CP_UTF8, 0, input.data(), (int)input.size(),
                        output.data(), count, nullptr, nullptr);
    return output;
}
std::wstring normalize(std::wstring value) {
    std::replace(value.begin(), value.end(), L'/', L'\\');
    while (value.size() > 3 && value.back() == L'\\') value.pop_back();
    std::transform(value.begin(), value.end(), value.begin(),
                   [](wchar_t c) {return std::towlower(c);});
    return value;
}
void pauseIfInteractive() {
    DWORD mode = 0;
    if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode)) {
        std::cout << "\nPress Enter to close / Kapatmak icin Enter..." << std::flush;
        std::string unused;
        std::getline(std::cin, unused);
    }
}
bool yesNo(const char* prompt) {
    for (;;) {
        std::cout << prompt << " [Y/N]: " << std::flush;
        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << "\nCancelled (no input).\n";
            return false;
        }
        auto start = input.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            std::cout << "Please type Y or N.\n";
            continue;
        }
        auto end = input.find_last_not_of(" \t\r\n");
        input = input.substr(start, end-start+1);
        if (input == "y" || input == "Y") return true;
        if (input == "n" || input == "N") return false;
        std::cout << "Please type Y or N.\n";
    }
}
std::wstring parseFolder(std::string input) {
    if (input.size() >= 2 && input.front() == '"' && input.back() == '"')
        input = input.substr(1, input.size()-2);
    if (input.empty()) throw std::runtime_error("No installation directory provided");
    auto dir = fs::absolute(fs::path(fromUtf8(input))).lexically_normal();
    if (dir == dir.root_path() || dir.empty())
        throw std::runtime_error("Installing to drive root is not allowed");
    auto windows = fs::path(getEnv(L"SystemRoot")).lexically_normal();
    if (normalize(dir.wstring()) == normalize(windows.wstring()))
        throw std::runtime_error("Do not install directly in the Windows system folder");
    return dir.wstring();
}
void installPayload(const fs::path& target) {
    HRSRC res = FindResourceW(nullptr, MAKEINTRESOURCEW(PAYLOAD_ID), RT_RCDATA);
    if (!res) throw std::runtime_error("Installer has no embedded MGET executable");
    DWORD size = SizeofResource(nullptr, res);
    HGLOBAL data = LoadResource(nullptr, res);
    const void* bytes = data ? LockResource(data) : nullptr;
    if (!bytes || size < 1024)
        throw std::runtime_error("Embedded executable missing or invalid");
    fs::create_directories(target);

    const fs::path exe = target/L"mget.exe";
    const fs::path pending = target/L"mget.new";
    const fs::path backup = target/L"mget.old";
    HANDLE f = CreateFileW(pending.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE)
        throw std::runtime_error("Cannot write executable to the target folder");
    const auto* buffer = static_cast<const BYTE*>(bytes);
    DWORD done = 0;
    bool wrote = true;
    while (done < size) {
        DWORD written = 0;
        if (!WriteFile(f, buffer+done, size-done, &written, nullptr) || written == 0) {
            wrote = false; break;
        }
        done += written;
    }
    if (wrote) wrote = FlushFileBuffers(f) != 0;
    CloseHandle(f);
    if (!wrote) {
        DeleteFileW(pending.c_str());
        throw std::runtime_error("Could not finish writing executable");
    }
    const bool hasPrevious = fs::exists(exe);
    if (hasPrevious) {
        if (fs::exists(backup)) fs::remove(backup);
        if (!MoveFileExW(exe.c_str(), backup.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            DeleteFileW(pending.c_str());
            throw std::runtime_error("Cannot replace running MGET; close it and retry");
        }
    }
    if (!MoveFileExW(pending.c_str(), exe.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        if (hasPrevious)
            MoveFileExW(backup.c_str(), exe.c_str(), MOVEFILE_REPLACE_EXISTING);
        DeleteFileW(pending.c_str());
        throw std::runtime_error("Could not complete installation; previous version restored");
    }
}
void installUninstallerPayload(const fs::path& target) {
    HRSRC res=FindResourceW(nullptr,MAKEINTRESOURCEW(102),RT_RCDATA);
    if(!res)throw std::runtime_error("Installer is missing embedded uninstaller");
    DWORD size=SizeofResource(nullptr,res);
    HGLOBAL h=LoadResource(nullptr,res);
    const BYTE* bytes=h?static_cast<const BYTE*>(LockResource(h)):nullptr;
    if(!bytes||size<1024)throw std::runtime_error("Embedded uninstaller invalid");
    fs::path dest=target/L"MGET-Uninstall.exe";
    fs::path staged=target/L"MGET-Uninstall.new";
    HANDLE file=CreateFileW(staged.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot write embedded uninstaller");
    DWORD done=0;bool success=true;
    while(done<size){
        DWORD written=0;
        if(!WriteFile(file,bytes+done,size-done,&written,nullptr)||!written){success=false;break;}
        done+=written;
    }
    if(success)success=FlushFileBuffers(file)!=0;
    CloseHandle(file);
    if(!success){DeleteFileW(staged.c_str());throw std::runtime_error("Cannot complete uninstaller writing");}
    if(!MoveFileExW(staged.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING)) {
        DeleteFileW(staged.c_str());
        throw std::runtime_error("Cannot replace uninstaller (close it and retry)");
    }
}
void registerUninstaller(const fs::path& dir) {
    const wchar_t* keyName=L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGET";
    HKEY key=nullptr;
    if(RegCreateKeyExW(HKEY_CURRENT_USER,keyName,0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)!=ERROR_SUCCESS)
        throw std::runtime_error("Cannot register Windows uninstaller");
    auto regString=[&](const wchar_t* name,const std::wstring& value){
        if(RegSetValueExW(key,name,0,REG_SZ,(const BYTE*)value.c_str(),
           DWORD((value.size()+1)*sizeof(wchar_t)))!=ERROR_SUCCESS)
            throw std::runtime_error("Cannot save uninstall metadata");
    };
    try{
        regString(L"DisplayName",L"MGET Package Manager");
        regString(L"DisplayVersion",L"0.1.0-beta.2");
        regString(L"Publisher",L"MGET");
        regString(L"InstallLocation",dir.wstring());
        regString(L"DisplayIcon",(dir/L"MGET-Uninstall.exe").wstring());
        regString(L"UninstallString",L"\""+(dir/L"MGET-Uninstall.exe").wstring()+L"\"");
        DWORD one=1;
        if(RegSetValueExW(key,L"NoModify",0,REG_DWORD,(BYTE*)&one,sizeof(one))!=ERROR_SUCCESS ||
           RegSetValueExW(key,L"NoRepair",0,REG_DWORD,(BYTE*)&one,sizeof(one))!=ERROR_SUCCESS)
            throw std::runtime_error("Cannot set uninstall metadata");
    }catch(...){RegCloseKey(key);throw;}
    RegCloseKey(key);
}

std::wstring readUserPath(HKEY key, DWORD& registryType) {
    DWORD len = 0;
    registryType = REG_EXPAND_SZ;
    LSTATUS rc = RegQueryValueExW(key, L"Path", nullptr, &registryType, nullptr, &len);
    if (rc == ERROR_FILE_NOT_FOUND) return {};
    if (rc != ERROR_SUCCESS) throw std::runtime_error("Cannot inspect user PATH");
    if (registryType != REG_EXPAND_SZ && registryType != REG_SZ)
        throw std::runtime_error("User PATH registry value has unexpected type");
    std::vector<BYTE> buffer(len + sizeof(wchar_t), 0);
    rc = RegQueryValueExW(key, L"Path", nullptr, &registryType, buffer.data(), &len);
    if (rc != ERROR_SUCCESS) throw std::runtime_error("Cannot read user PATH");
    return std::wstring(reinterpret_cast<const wchar_t*>(buffer.data()));
}
bool pathContains(const std::wstring& value, const std::wstring& item) {
    const auto expected = normalize(item);
    size_t begin = 0;
    while (begin < value.size()) {
        auto end = value.find(L';', begin);
        if (end == std::wstring::npos) end = value.size();
        std::wstring candidate = value.substr(begin, end-begin);
        auto left = candidate.find_first_not_of(L" \t\r\n\"");
        auto right = candidate.find_last_not_of(L" \t\r\n\"");
        if (left != std::wstring::npos &&
            normalize(candidate.substr(left, right-left+1)) == expected)
            return true;
        if (end == value.size()) break;
        begin = end+1;
    }
    return false;
}
void addUserPath(const fs::path& directory) {
    HKEY key = nullptr;
    auto rc = RegCreateKeyExW(HKEY_CURRENT_USER, L"Environment", 0, nullptr, 0,
                              KEY_READ | KEY_SET_VALUE, nullptr, &key, nullptr);
    if (rc != ERROR_SUCCESS) throw std::runtime_error("Cannot open Windows user PATH");
    try {
        DWORD type = REG_EXPAND_SZ;
        auto path = readUserPath(key, type);
        if (!pathContains(path, directory.wstring())) {
            if (!path.empty() && path.back() != L';') path += L';';
            path += directory.wstring();
            rc = RegSetValueExW(key, L"Path", 0, type,
                reinterpret_cast<const BYTE*>(path.c_str()),
                DWORD((path.size()+1)*sizeof(wchar_t)));
            if (rc != ERROR_SUCCESS) throw std::runtime_error("Could not update user PATH");
        }
    } catch (...) { RegCloseKey(key); throw; }
    RegCloseKey(key);
    DWORD_PTR ignored = 0;
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
                        reinterpret_cast<LPARAM>(L"Environment"),
                        SMTO_ABORTIFHUNG, 3000, &ignored);
}
}
int wmain(int argc, wchar_t** argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    try {
        std::wstring selected;
        bool skipPathForTest = false;
        for (int i=1; i<argc; ++i) {
            const std::wstring arg = argv[i];
            if (arg == L"--verify-payload") {
                HRSRC r = FindResourceW(nullptr, MAKEINTRESOURCEW(PAYLOAD_ID), RT_RCDATA);
                return r && SizeofResource(nullptr, r) >= 1024 ? 0 : 10;
            }
            if (arg == L"--help" || arg == L"-h") {
                std::cout << "MGET Setup " << INSTALLER_VERSION
                          << "\nUsage: MGET-Setup.exe [--dir=INSTALL_FOLDER]\n";
                return 0;
            }
            if (arg == L"--no-path-for-test") {
                skipPathForTest = true;
                continue;
            }
            if (arg == L"--dir" && i+1<argc) {
                selected = argv[++i];
                continue;
            }
            if (arg.rfind(L"--dir=",0)==0) {
                selected = arg.substr(6);
                continue;
            }
            throw std::runtime_error("Unknown option; use --help");
        }

        const fs::path defaultDir = fs::path(getEnv(L"LOCALAPPDATA"))/L"Programs"/L"MGET";
        std::cout << "\n====================================\n"
                     " MGET Installer " << INSTALLER_VERSION << "\n"
                     "====================================\n\n"
                     "Single EXE setup | Windows user PATH\n"
                     "Default: %LOCALAPPDATA%\\Programs\\MGET\n\n";

        if (!yesNo("Install MGET now? / MGET kurulumu baslasin mi?")) {
            std::cout << "Installation cancelled. No changes made.\n";
            pauseIfInteractive();
            return 0;
        }

        fs::path directory;
        if (!selected.empty()) {
            directory = fs::absolute(fs::path(selected)).lexically_normal();
        } else if (yesNo("Use default installation folder? / Varsayilan klasor kullanilsin mi?")) {
            directory = defaultDir;
        } else {
            std::cout << "Enter custom folder (empty = cancel): " << std::flush;
            std::string entered;
            if (!std::getline(std::cin, entered) || entered.empty()) {
                std::cout << "Installation cancelled.\n";
                pauseIfInteractive();
                return 0;
            }
            directory = fs::path(parseFolder(entered));
        }
        if (directory.empty() || directory == directory.root_path())
            throw std::runtime_error("Invalid installation directory");
        std::cout << "\nInstalling to: " << toUtf8(directory.wstring()) << "\n";
        installPayload(directory);
        installUninstallerPayload(directory);
        if (!skipPathForTest) {
            try {
                registerUninstaller(directory);
                addUserPath(directory);
            } catch (...) {
                std::cout << "MGET binaries installed, but Windows registration or PATH failed.\n"
                             "Run MGET-Uninstall.exe to remove the application if needed.\n";
                return 2;
            }
        }
        std::cout << "\n[OK] MGET and MGET-Uninstall.exe installed successfully.\n"
                     "Open a NEW CMD or PowerShell terminal and run:\n"
                     "  mget --version\n"
                     "  mget doctor\n";
        if (skipPathForTest)
            std::cout << "Note: PATH update skipped for local test.\n";
        pauseIfInteractive();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] Installation failed: " << e.what() << "\n";
        pauseIfInteractive();
        return 1;
    }
}
