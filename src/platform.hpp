#pragma once
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <winhttp.h>
#include <wincrypt.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <cwctype>

namespace fs=std::filesystem;
namespace sys {
inline std::wstring wide(const std::string& s) {
    if(s.empty())return {};
    int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),(int)s.size(),nullptr,0);
    if(!n)throw std::runtime_error("Invalid UTF-8");
    std::wstring w(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),w.data(),n);return w;
}
inline std::string utf8(const std::wstring& w) {
    if(w.empty())return {};
    int n=WideCharToMultiByte(CP_UTF8,0,w.data(),(int)w.size(),nullptr,0,nullptr,nullptr);
    std::string s(n,0);WideCharToMultiByte(CP_UTF8,0,w.data(),(int)w.size(),s.data(),n,nullptr,nullptr);return s;
}
inline std::wstring env(const wchar_t* name) {
    DWORD n=GetEnvironmentVariableW(name,nullptr,0);
    if(!n)return {};
    std::wstring r(n,L'\0');GetEnvironmentVariableW(name,r.data(),n);r.resize(n-1);return r;
}
inline std::wstring expand(const std::wstring& s) {
    DWORD n=ExpandEnvironmentStringsW(s.c_str(),nullptr,0);
    if(!n)throw std::runtime_error("Cannot expand environment");
    std::wstring r(n,L'\0');ExpandEnvironmentStringsW(s.c_str(),r.data(),n);r.resize(n-1);return r;
}
inline std::wstring trim(std::wstring w) {
    auto a=w.find_first_not_of(L" \t\r\n"),b=w.find_last_not_of(L" \t\r\n");
    return a==std::wstring::npos?L"":w.substr(a,b-a+1);
}
inline std::wstring normalizeLocation(std::wstring s) {
    s=expand(s);
    std::replace(s.begin(),s.end(),L'/',L'\\');
    // User/Downloads alias; supports Turkish folder name as well.
    size_t k=s.find(L'\\');
    if(k!=std::wstring::npos && s.find(L':')==std::wstring::npos && s[0]!=L'\\') {
        std::wstring user=s.substr(0,k), tail=s.substr(k+1);
        auto pos=tail.find(L'\\');
        std::wstring head=pos==std::wstring::npos?tail:tail.substr(0,pos);
        if(head==L"İndirilenler"||head==L"Downloads"||head==L"indirilenler") {
            std::wstring p=env(L"USERPROFILE");
            if(user==env(L"USERNAME")||user==L"kullanıcıadı"||user==L"user")
                return p+L"\\Downloads"+(pos==std::wstring::npos?L"":tail.substr(pos));
        }
    }
    return s;
}
inline std::wstring chooseFolder() {
    HRESULT init=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    IFileDialog* dlg=nullptr;std::wstring result;
    if(SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dlg)))) {
        DWORD flags=0;dlg->GetOptions(&flags);dlg->SetOptions(flags|FOS_PICKFOLDERS|FOS_PATHMUSTEXIST|FOS_FORCEFILESYSTEM);
        dlg->SetTitle(L"MGET - Choose installation directory");
        if(SUCCEEDED(dlg->Show(nullptr))) {
            IShellItem* item=nullptr;
            if(SUCCEEDED(dlg->GetResult(&item))) {
                PWSTR p=nullptr;if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&p))) {result=p;CoTaskMemFree(p);}
                item->Release();
            }
        }
        dlg->Release();
    }
    if(SUCCEEDED(init))CoUninitialize();
    return result;
}
inline bool confirm(const std::wstring& question) {
    std::wcout<<question<<L" [y/N]: "<<std::flush;
    std::wstring s;std::getline(std::wcin,s);
    return s==L"y"||s==L"Y"||s==L"e"||s==L"E";
}
inline bool hasTool(const std::wstring& name) {
    wchar_t buf[32768];DWORD n=SearchPathW(nullptr,name.c_str(),nullptr,32768,buf,nullptr);
    return n>0&&n<32768;
}
inline std::wstring quote(const std::wstring& s) {
    std::wstring out=L"\"";size_t back=0;
    for(wchar_t c:s) {
        if(c==L'\\'){++back;continue;}
        if(c==L'"'){out.append(back*2+1,L'\\');out+=c;back=0;continue;}
        out.append(back,L'\\');back=0;out+=c;
    }
    out.append(back*2,L'\\');return out+L"\"";
}
inline int run(const std::wstring& exe,const std::vector<std::wstring>& args={},const std::wstring& cwd=L"") {
    std::wstring cmd=quote(exe);
    for(const auto& a:args)cmd+=L" "+quote(a);
    STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
    std::vector<wchar_t> buffer(cmd.begin(),cmd.end());buffer.push_back(0);
    if(!CreateProcessW(nullptr,buffer.data(),nullptr,nullptr,FALSE,0,nullptr,cwd.empty()?nullptr:cwd.c_str(),&si,&pi))
        throw std::runtime_error("Failed to start process: "+utf8(exe)+" (Win32 "+std::to_string(GetLastError())+")");
    WaitForSingleObject(pi.hProcess,INFINITE);
    DWORD code=1;GetExitCodeProcess(pi.hProcess,&code);
    CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return (int)code;
}
inline std::string read(const fs::path& p) {
    std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Cannot read "+utf8(p.wstring()));
    return std::string(std::istreambuf_iterator<char>(f),{});
}
inline void write(const fs::path& p,const std::string& content) {
    fs::create_directories(p.parent_path());
    std::ofstream f(p,std::ios::binary|std::ios::trunc);if(!f)throw std::runtime_error("Cannot write "+utf8(p.wstring()));
    f<<content;if(!f)throw std::runtime_error("Write failed");
}
inline fs::path appData() {
    std::wstring p=env(L"LOCALAPPDATA");
    if(p.empty())throw std::runtime_error("LOCALAPPDATA unavailable");
    return fs::path(p)/L"MGET";
}
inline std::string sha256(const fs::path& p) {
    HCRYPTPROV prov=0;HCRYPTHASH hash=0;
    if(!CryptAcquireContextW(&prov,nullptr,nullptr,PROV_RSA_AES,CRYPT_VERIFYCONTEXT))throw std::runtime_error("Crypto provider failed");
    if(!CryptCreateHash(prov,CALG_SHA_256,0,0,&hash)){CryptReleaseContext(prov,0);throw std::runtime_error("SHA-256 failed");}
    std::ifstream f(p,std::ios::binary);if(!f){CryptDestroyHash(hash);CryptReleaseContext(prov,0);throw std::runtime_error("Cannot hash file");}
    char block[65536];
    while(f){f.read(block,sizeof(block));auto n=f.gcount();if(n&&!CryptHashData(hash,(BYTE*)block,(DWORD)n,0)){CryptDestroyHash(hash);CryptReleaseContext(prov,0);throw std::runtime_error("Hash update failed");}}
    BYTE bytes[32];DWORD len=32;
    if(!CryptGetHashParam(hash,HP_HASHVAL,bytes,&len,0)){CryptDestroyHash(hash);CryptReleaseContext(prov,0);throw std::runtime_error("Hash final failed");}
    CryptDestroyHash(hash);CryptReleaseContext(prov,0);
    static const char* dig="0123456789abcdef";std::string h;
    for(unsigned i=0;i<len;i++){h+=dig[bytes[i]>>4];h+=dig[bytes[i]&15];}return h;
}
inline std::wstring urlFilename(std::wstring url) {
    auto last=url.find_last_of(L'/');std::wstring name=url.substr(last==std::wstring::npos?0:last+1);
    auto q=name.find_first_of(L"?#");if(q!=std::wstring::npos)name.resize(q);
    for(auto& c:name)if(std::wstring(L"<>:\"/\\|?*").find(c)!=std::wstring::npos)c=L'_';
    if(name.empty()||name==L"."||name==L"..")name=L"download.bin";
    return name;
}
inline void download(const std::wstring& url,const fs::path& file) {
    if(url.rfind(L"https://",0)!=0)throw std::runtime_error("Only HTTPS downloads allowed");
    URL_COMPONENTSW comp{};comp.dwStructSize=sizeof(comp);
    wchar_t host[2048]={},path[16384]={},extra[8192]={};
    comp.lpszHostName=host;comp.dwHostNameLength=2048;
    comp.lpszUrlPath=path;comp.dwUrlPathLength=16384;
    comp.lpszExtraInfo=extra;comp.dwExtraInfoLength=8192;
    if(!WinHttpCrackUrl(url.c_str(),0,0,&comp))throw std::runtime_error("Invalid URL");
    HINTERNET sess=WinHttpOpen(L"MGET/0.1",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
    if(!sess)throw std::runtime_error("WinHTTP session failed");
    HINTERNET conn=WinHttpConnect(sess,host,comp.nPort,0);
    if(!conn){WinHttpCloseHandle(sess);throw std::runtime_error("WinHTTP connection failed");}
    std::wstring resource=std::wstring(path)+extra;
    HINTERNET req=WinHttpOpenRequest(conn,L"GET",resource.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE);
    if(!req){WinHttpCloseHandle(conn);WinHttpCloseHandle(sess);throw std::runtime_error("WinHTTP request failed");}
    WinHttpSetTimeouts(req,15000,15000,30000,30000);
    bool ok=WinHttpSendRequest(req,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)&&WinHttpReceiveResponse(req,nullptr);
    DWORD status=0,size=sizeof(status);
    if(ok)ok=WinHttpQueryHeaders(req,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX)&&status==200;
    if(!ok){WinHttpCloseHandle(req);WinHttpCloseHandle(conn);WinHttpCloseHandle(sess);throw std::runtime_error("HTTP download failed (status "+std::to_string(status)+")");}
    wchar_t contentType[256]={};DWORD ctLen=sizeof(contentType);
    if(WinHttpQueryHeaders(req,WINHTTP_QUERY_CONTENT_TYPE,WINHTTP_HEADER_NAME_BY_INDEX,contentType,&ctLen,WINHTTP_NO_HEADER_INDEX)) {
        if(std::wstring(contentType).find(L"text/html")!=std::wstring::npos) {
            WinHttpCloseHandle(req);WinHttpCloseHandle(conn);WinHttpCloseHandle(sess);
            throw std::runtime_error("Link returned an HTML page; use a direct public download URL");
        }
    }
    fs::create_directories(file.parent_path());fs::path part=file;part+=L".part";
    std::ofstream output(part,std::ios::binary|std::ios::trunc);
    if(!output){WinHttpCloseHandle(req);WinHttpCloseHandle(conn);WinHttpCloseHandle(sess);throw std::runtime_error("Cannot create download file");}
    bool good=true;DWORD got=0;char data[65536];uint64_t total=0;
    while(true) {
        got=0;
        if(!WinHttpReadData(req,data,sizeof(data),&got)){good=false;break;}
        if(got==0)break;
        total+=got;
        if(total>4ULL*1024*1024*1024){good=false;break;}
        output.write(data,got);if(!output){good=false;break;}
    }
    output.flush();
    if(!output)good=false;
    output.close();WinHttpCloseHandle(req);WinHttpCloseHandle(conn);WinHttpCloseHandle(sess);
    if(!good){fs::remove(part);throw std::runtime_error("Download incomplete");}
    if(fs::exists(file))fs::remove(file);fs::rename(part,file);
}
inline bool safeRelative(const fs::path& p) {
    if(p.empty()||p.is_absolute()||p.has_root_name()||p.has_root_directory())return false;
    for(const auto& part:p)if(part==L"..")return false;
    return true;
}
inline void setRegistry(const std::wstring& hive,const std::wstring& key,const std::wstring& name,const std::wstring& value) {
    HKEY root=hive==L"HKLM"?HKEY_LOCAL_MACHINE:hive==L"HKCU"?HKEY_CURRENT_USER:nullptr;
    if(!root)throw std::runtime_error("Unsupported registry hive");
    HKEY sub=nullptr;LSTATUS code=RegCreateKeyExW(root,key.c_str(),0,nullptr,0,KEY_SET_VALUE,nullptr,&sub,nullptr);
    if(code!=ERROR_SUCCESS)throw std::runtime_error("Registry open failed: "+std::to_string(code));
    code=RegSetValueExW(sub,name.c_str(),0,REG_SZ,(const BYTE*)value.c_str(),(DWORD)((value.size()+1)*sizeof(wchar_t)));
    RegCloseKey(sub);if(code!=ERROR_SUCCESS)throw std::runtime_error("Registry write failed");
}
inline void shortcut(const std::wstring& name,const fs::path& target) {
    HRESULT init=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    IShellLinkW* link=nullptr;
    if(FAILED(CoCreateInstance(CLSID_ShellLink,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&link))))
        throw std::runtime_error("Cannot create Windows shortcut");
    link->SetPath(target.c_str());IPersistFile* p=nullptr;
    if(FAILED(link->QueryInterface(IID_PPV_ARGS(&p)))){link->Release();throw std::runtime_error("Shortcut persistence failed");}
    fs::path dir=fs::path(env(L"APPDATA"))/L"Microsoft\\Windows\\Start Menu\\Programs";
    fs::create_directories(dir);
    HRESULT h=p->Save((dir/(name+L".lnk")).c_str(),TRUE);
    p->Release();link->Release();if(SUCCEEDED(init))CoUninitialize();
    if(FAILED(h))throw std::runtime_error("Shortcut save failed");
}
}
