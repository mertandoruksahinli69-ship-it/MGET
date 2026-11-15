#define NOMINMAX
#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <filesystem>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cwctype>

namespace fs = std::filesystem;
namespace {
constexpr int PAYLOAD_ID=101;
std::wstring getEnv(const wchar_t* name) {
    DWORD n=GetEnvironmentVariableW(name,nullptr,0);
    if(!n)throw std::runtime_error("Missing environment variable");
    std::wstring v(n,L'\0');
    if(!GetEnvironmentVariableW(name,v.data(),n))throw std::runtime_error("Cannot read environment variable");
    v.resize(n-1);return v;
}
std::wstring normalize(std::wstring v) {
    std::replace(v.begin(),v.end(),L'/',L'\\');
    while(v.size()>3&&v.back()==L'\\')v.pop_back();
    std::transform(v.begin(),v.end(),v.begin(),[](wchar_t c){return std::towlower(c);});
    return v;
}
std::wstring selectFolder() {
    std::wstring folder;
    HRESULT init=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    if(FAILED(init))return folder;
    IFileDialog* dialog=nullptr;
    if(SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog)))) {
        DWORD flags=0;
        dialog->GetOptions(&flags);
        dialog->SetOptions(flags|FOS_PICKFOLDERS|FOS_FORCEFILESYSTEM|FOS_PATHMUSTEXIST);
        dialog->SetTitle(L"MGET - Select installation folder");
        if(SUCCEEDED(dialog->Show(nullptr))) {
            IShellItem* item=nullptr;
            if(SUCCEEDED(dialog->GetResult(&item))) {
                PWSTR path=nullptr;
                if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&path))) {
                    folder=path;CoTaskMemFree(path);
                }
                item->Release();
            }
        }
        dialog->Release();
    }
    CoUninitialize();
    return folder;
}
void installPayload(const fs::path& target) {
    HRSRC res=FindResourceW(nullptr,MAKEINTRESOURCEW(PAYLOAD_ID),RT_RCDATA);
    if(!res)throw std::runtime_error("Installer has no embedded MGET payload");
    DWORD size=SizeofResource(nullptr,res);
    HGLOBAL data=LoadResource(nullptr,res);
    const void* bytes=data?LockResource(data):nullptr;
    if(!bytes||!size)throw std::runtime_error("Cannot read embedded payload");
    fs::create_directories(target);
    fs::path exe=target/L"mget.exe",pending=target/L"mget.new",backup=target/L"mget.old";
    HANDLE f=CreateFileW(pending.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(f==INVALID_HANDLE_VALUE)throw std::runtime_error("Cannot write MGET payload to selected folder");
    const BYTE* buffer=static_cast<const BYTE*>(bytes);
    DWORD done=0;
    bool wrote=true;
    while(done<size) {
        DWORD written=0;
        if(!WriteFile(f,buffer+done,size-done,&written,nullptr)||written==0) {wrote=false;break;}
        done+=written;
    }
    if(wrote)wrote=FlushFileBuffers(f)!=0;
    CloseHandle(f);
    if(!wrote) {DeleteFileW(pending.c_str());throw std::runtime_error("Failed writing executable");}
    bool hasPrevious=fs::exists(exe);
    if(hasPrevious) {
        if(fs::exists(backup))fs::remove(backup);
        if(!MoveFileExW(exe.c_str(),backup.c_str(),MOVEFILE_REPLACE_EXISTING)) {
            DeleteFileW(pending.c_str());
            throw std::runtime_error("Cannot replace existing mget.exe. Close MGET and retry");
        }
    }
    if(!MoveFileExW(pending.c_str(),exe.c_str(),MOVEFILE_REPLACE_EXISTING)) {
        if(hasPrevious)MoveFileExW(backup.c_str(),exe.c_str(),MOVEFILE_REPLACE_EXISTING);
        DeleteFileW(pending.c_str());throw std::runtime_error("Cannot finalize installation");
    }
}
std::wstring readUserPath(HKEY key,DWORD& registryType) {
    DWORD len=0;registryType=REG_EXPAND_SZ;
    auto rc=RegQueryValueExW(key,L"Path",nullptr,&registryType,nullptr,&len);
    if(rc==ERROR_FILE_NOT_FOUND)return L"";
    if(rc!=ERROR_SUCCESS)throw std::runtime_error("Cannot read user PATH");
    if(registryType!=REG_EXPAND_SZ&&registryType!=REG_SZ)throw std::runtime_error("User PATH has unexpected registry type");
    std::vector<BYTE> bytes(len+sizeof(wchar_t),0);
    rc=RegQueryValueExW(key,L"Path",nullptr,&registryType,bytes.data(),&len);
    if(rc!=ERROR_SUCCESS)throw std::runtime_error("Cannot fetch user PATH");
    return std::wstring(reinterpret_cast<const wchar_t*>(bytes.data()));
}
bool pathContains(const std::wstring& value,const std::wstring& item) {
    auto expected=normalize(item);
    size_t begin=0;
    while(begin<value.size()) {
        auto end=value.find(L';',begin);
        if(end==std::wstring::npos)end=value.size();
        std::wstring candidate=value.substr(begin,end-begin);
        auto left=candidate.find_first_not_of(L" \t\r\n\"");
        auto right=candidate.find_last_not_of(L" \t\r\n\"");
        if(left!=std::wstring::npos && normalize(candidate.substr(left,right-left+1))==expected)return true;
        if(end==value.size())break;begin=end+1;
    }
    return false;
}
void addUserPath(const fs::path& directory) {
    HKEY key=nullptr;
    auto rc=RegCreateKeyExW(HKEY_CURRENT_USER,L"Environment",0,nullptr,0,KEY_READ|KEY_SET_VALUE,nullptr,&key,nullptr);
    if(rc!=ERROR_SUCCESS)throw std::runtime_error("Cannot open HKCU Environment");
    try {
        DWORD type=REG_EXPAND_SZ;
        auto path=readUserPath(key,type);
        if(!pathContains(path,directory.wstring())) {
            if(!path.empty()&&path.back()!=L';')path+=L';';
            path+=directory.wstring();
            rc=RegSetValueExW(key,L"Path",0,type,(const BYTE*)path.c_str(),DWORD((path.size()+1)*sizeof(wchar_t)));
            if(rc!=ERROR_SUCCESS)throw std::runtime_error("Cannot set user PATH");
        }
    }catch(...){RegCloseKey(key);throw;}
    RegCloseKey(key);
    DWORD_PTR ignored=0;
    SendMessageTimeoutW(HWND_BROADCAST,WM_SETTINGCHANGE,0,(LPARAM)L"Environment",SMTO_ABORTIFHUNG,3000,&ignored);
}
void alert(const std::wstring& t,UINT flags=MB_ICONINFORMATION) {
    MessageBoxW(nullptr,t.c_str(),L"MGET Setup",flags|MB_OK|MB_SETFOREGROUND);
}
}
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR,int) {
    try {
        fs::path dir=fs::path(getEnv(L"LOCALAPPDATA"))/L"Programs"/L"MGET";
        const int choice=MessageBoxW(nullptr,
            L"MGET Setup 0.1.0\n\n"
            L"Yes: Install to the recommended folder.\n"
            L"No: Choose a different folder.\n"
            L"Cancel: Exit without changes.\n\n"
            L"Default: %LOCALAPPDATA%\\Programs\\MGET\n\n"
            L"Kurulum sonrasi MGET kullanici PATH degiskenine eklenir.",
            L"MGET Installer / Kurulum", MB_YESNOCANCEL|MB_ICONQUESTION|MB_DEFBUTTON1|MB_SETFOREGROUND);
        if(choice==IDCANCEL)return 0;
        if(choice==IDNO) {
            auto selection=selectFolder();
            if(selection.empty())return 0;
            dir=fs::path(selection)/L"MGET";
        }
        installPayload(dir);
        try {
            addUserPath(dir);
        }catch(const std::exception&) {
            alert(L"MGET installed at:\n"+dir.wstring()+L"\n\n"
                  L"Windows PATH could not be updated. Add the folder to your user PATH manually.",
                  MB_ICONWARNING);
            return 2;
        }
        alert(L"MGET installation complete!\n\n"
              L"Installed: "+(dir/L"mget.exe").wstring()+L"\n\n"
              L"PATH updated for your Windows user.\n"
              L"Open a NEW terminal and run:\n\n"
              L"mget --version\n"
              L"mget doctor\n\n"
              L"Kurulum tamamlandi. Yeni terminal acin ve 'mget' yazin.");
        return 0;
    } catch(const std::exception& e) {
        std::string msg=e.what();
        std::wstring w(msg.begin(),msg.end());
        alert(L"Installation failed / Kurulum basarisiz\n\n"+w,MB_ICONERROR);
        return 1;
    }
}
