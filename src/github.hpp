#pragma once
#include "mini_json.hpp"
#include "platform.hpp"
#include <set>
#include <sstream>
#include <limits>

namespace gh {
struct Repo {
    std::wstring owner, name, tag;
    bool releases=false, directAsset=false;
    std::wstring directUrl;
};
inline bool identifier(const std::wstring& s) {
    if(s.empty()||s.size()>150||s==L"."||s==L"..")return false;
    for(wchar_t c:s) {
        if(!((c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||
            (c>=L'0'&&c<=L'9')||c==L'_'||c==L'-'||c==L'.'))
            return false;
    }
    return true;
}
inline bool isGitHub(const std::wstring& u) {
    return u.rfind(L"https://github.com/",0)==0||
           u.rfind(L"https://www.github.com/",0)==0;
}
inline Repo parseURL(const std::wstring& u) {
    size_t start=u.find(L'/',8);
    if(start==std::wstring::npos)throw std::runtime_error("Invalid GitHub URL");
    auto path=u.substr(start+1);
    auto end=path.find_first_of(L"?#");
    if(end!=std::wstring::npos)path.resize(end);
    while(!path.empty()&&path.back()==L'/')path.pop_back();
    std::vector<std::wstring> parts;
    std::wstringstream ss(path);std::wstring s;
    while(std::getline(ss,s,L'/'))parts.push_back(s);
    if(parts.size()<2)throw std::runtime_error("GitHub URL requires owner/repository");
    Repo r; r.owner=parts[0];r.name=parts[1];
    if(r.name.size()>4&&r.name.ends_with(L".git"))r.name.resize(r.name.size()-4);
    if(!identifier(r.owner)||!identifier(r.name))throw std::runtime_error("Invalid GitHub owner/repository");
    if(parts.size()==2)return r;
    if(parts[2]!=L"releases")throw std::runtime_error("Expected GitHub repository URL or /releases URL");
    r.releases=true;
    if(parts.size()==3||(parts.size()==4&&parts[3]==L"latest"))return r;
    if(parts.size()>=5&&parts[3]==L"tag") {
        r.tag=parts[4];
        if(!identifier(r.tag))throw std::runtime_error("Unsupported release tag");
        return r;
    }
    if(parts.size()>=6&&parts[3]==L"download") {
        r.directAsset=true;r.directUrl=u;return r;
    }
    throw std::runtime_error("Unrecognized GitHub release URL");
}
struct HttpHandle {
    HINTERNET h=nullptr;
    ~HttpHandle(){if(h)WinHttpCloseHandle(h);}
};
inline std::string jsonGet(const std::wstring& endpoint) {
    if(endpoint.rfind(L"https://api.github.com/repos/",0)!=0)
        throw std::runtime_error("GitHub API URL rejected");
    URL_COMPONENTSW c{};c.dwStructSize=sizeof(c);
    wchar_t host[256]={},route[4096]={},extra[4096]={};
    c.lpszHostName=host;c.dwHostNameLength=256;
    c.lpszUrlPath=route;c.dwUrlPathLength=4096;
    c.lpszExtraInfo=extra;c.dwExtraInfoLength=4096;
    if(!WinHttpCrackUrl(endpoint.c_str(),0,0,&c))throw std::runtime_error("Bad GitHub API URL");
    HttpHandle session{WinHttpOpen(L"MGET/0.2 (GitHub public API)",
         WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};
    if(!session.h)throw std::runtime_error("Cannot open GitHub API session");
    HttpHandle connection{WinHttpConnect(session.h,host,INTERNET_DEFAULT_HTTPS_PORT,0)};
    if(!connection.h)throw std::runtime_error("Cannot connect to GitHub API");
    std::wstring uri=std::wstring(route)+extra;
    HttpHandle request{WinHttpOpenRequest(connection.h,L"GET",uri.c_str(),nullptr,
         WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE)};
    if(!request.h)throw std::runtime_error("Cannot build GitHub API request");
    WinHttpSetTimeouts(request.h,10000,10000,30000,30000);
    const wchar_t* headers=L"Accept: application/vnd.github+json\r\n";
    if(!WinHttpSendRequest(request.h,headers,(DWORD)-1L,WINHTTP_NO_REQUEST_DATA,0,0,0)||
       !WinHttpReceiveResponse(request.h,nullptr))
        throw std::runtime_error("GitHub API request failed");
    DWORD status=0,len=sizeof(status);
    if(!WinHttpQueryHeaders(request.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
          WINHTTP_HEADER_NAME_BY_INDEX,&status,&len,WINHTTP_NO_HEADER_INDEX))
        throw std::runtime_error("Cannot read GitHub API status");
    if(status!=200) {
        if(status==403||status==429)throw std::runtime_error("GitHub API rate limit/access blocked (HTTP "+std::to_string(status)+"). Try again after the rate limit resets or use an authenticated GitHub client.");
        throw std::runtime_error("GitHub API HTTP "+std::to_string(status)+" (check repository URL)");
    }
    std::string body;char buffer[32768];DWORD read=0;
    for(;;) {
        if(!WinHttpReadData(request.h,buffer,sizeof(buffer),&read))
            throw std::runtime_error("GitHub API read failed");
        if(!read)break;
        if(body.size()+read>4*1024*1024)throw std::runtime_error("GitHub API response too large");
        body.append(buffer,read);
    }
    return body;
}
inline std::wstring sanitize(std::wstring name) {
    if(name.empty()||name==L"."||name==L"..")throw std::runtime_error("Invalid GitHub file name");
    for(wchar_t c:name) {
        if(c<32||std::wstring(L"<>:\"/\\|?*").find(c)!=std::wstring::npos)
            throw std::runtime_error("Unsafe GitHub file name");
    }
    if(name.back()==L'.'||name.back()==L' ')throw std::runtime_error("Unsafe file name suffix");
    return name;
}
inline std::wstring defaultBranch(const Repo& repo) {
    auto meta=mj::parse(jsonGet(L"https://api.github.com/repos/"+repo.owner+L"/"+repo.name));
    auto branch=sys::wide(meta.at("default_branch").str());
    if(branch.empty())throw std::runtime_error("Repository default branch unavailable");
    for(wchar_t c:branch) {
        if(!((c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||
             (c>=L'0'&&c<=L'9')||c==L'_'||c==L'-'||c==L'.'||c==L'/'))
            throw std::runtime_error("Unsupported branch characters");
    }
    return branch;
}
inline std::set<size_t> selection(const std::wstring& input,size_t maximum) {
    std::set<size_t> result;std::wstringstream ss(input);
    std::wstring part;
    while(std::getline(ss,part,L'-')) {
        part=sys::trim(part);
        if(part.empty()||part.size()>8)throw std::runtime_error("Selection must be numbers separated by '-'");
        size_t number=0;
        for(wchar_t c:part) {
            if(c<L'0'||c>L'9')throw std::runtime_error("Selection must contain numbers only");
            number=number*10+(c-L'0');
        }
        if(number==0||number>maximum)
            throw std::runtime_error("Select a number from 1 to "+std::to_string(maximum)+
                                     "; 0 is not a selection");
        result.insert(number-1);
    }
    if(result.empty())throw std::runtime_error("No file selected");
    return result;
}
inline void getOne(const std::wstring& url,const fs::path& dest,bool dry) {
    if(dest.has_filename()==false)throw std::runtime_error("Destination must be a file");
    std::wcout<<L"File: "<<dest.wstring()<<L"\n";
    if(dry){std::wcout<<L"[dry-run] No download\n";return;}
    if(fs::exists(dest)&&!sys::confirm(L"Overwrite existing file "+dest.wstring()+L"?"))
        throw std::runtime_error("Overwrite declined");
    sys::download(url,dest);
    std::wcout<<L"Downloaded: "<<dest.wstring()<<L"\nSHA256: "
              <<sys::wide(sys::sha256(dest))<<L"\n";
}
inline void handle(const std::wstring& input,const std::wstring& loc,
                   bool dry,bool all,const std::wstring& chosen=L"") {
    Repo r=parseURL(input);
    // There is no persistent cache: API JSON stays in RAM, downloads go to --loc / picker.
    fs::path dir;
    if(loc.empty()){
        if(dry){dir=L"<selected folder>";}
        else {
            auto picker=sys::chooseFolder();
            if(picker.empty())throw std::runtime_error("Download folder selection cancelled");
            dir=fs::path(picker);
        }
    } else dir=fs::absolute(fs::path(sys::normalizeLocation(loc))).lexically_normal();
    if(!dry&&dir==dir.root_path())throw std::runtime_error("Please choose a non-root download directory");
    if(r.directAsset) {
        auto name=sanitize(sys::urlFilename(r.directUrl));
        getOne(r.directUrl,dir/name,dry);return;
    }
    if(!r.releases) {
        auto branch=defaultBranch(r);
        auto safeBranch=branch;std::replace(safeBranch.begin(),safeBranch.end(),L'/',L'-');
        auto url=L"https://github.com/"+r.owner+L"/"+r.name+L"/archive/refs/heads/"+branch+L".zip";
        auto output=dir/sanitize(r.name+L"-"+safeBranch+L".zip");
        std::wcout<<L"GitHub repository: "<<r.owner<<L"/"<<r.name
                  <<L"\nSource branch: "<<branch<<L"\n";
        getOne(url,output,dry);return;
    }
    std::wstring api=L"https://api.github.com/repos/"+r.owner+L"/"+r.name+L"/releases/";
    api+=r.tag.empty()?L"latest":(L"tags/"+r.tag);
    auto release=mj::parse(jsonGet(api));
    const auto& arr=release.at("assets");
    if(arr.type!=mj::Value::Array)throw std::runtime_error("GitHub release has no assets list");
    if(arr.a.empty()) {
        std::wcout<<L"This release has no attached files. No download performed.\n";return;
    }
    struct Asset{std::wstring name,url,digest;double size=0;};
    std::vector<Asset> assets;
    for(const auto& a:arr.a) {
        Asset item;
        item.name=sanitize(sys::wide(a.at("name").str()));
        item.url=sys::wide(a.at("browser_download_url").str());
        item.digest=sys::wide(a.at("digest").str());
        if(item.url.rfind(L"https://github.com/",0)!=0)
            throw std::runtime_error("Unexpected GitHub release asset URL");
        item.size=a.at("size").n;
        assets.push_back(std::move(item));
    }
    std::wcout<<L"GitHub release: "<<r.owner<<L"/"<<r.name
              <<L" ("<<sys::wide(release.at("tag_name").str())<<L")\n";
    for(size_t i=0;i<assets.size();++i)
        std::wcout<<L" "<<i+1<<L"  "<<assets[i].name<<L" ("
            <<(unsigned long long)(assets[i].size/1024)<<L" KB)\n";
    std::wstring selected=chosen;
    if(selected.empty()&&!all) {
        std::wcout<<L"Which files? Example: 1-2-4 (each number selects one file, 0 is invalid)\nSelection: ";
        if(!std::getline(std::wcin,selected))throw std::runtime_error("Selection cancelled");
    }
    std::set<size_t> indices;
    if(all){for(size_t i=0;i<assets.size();++i)indices.insert(i);}
    else indices=selection(selected,assets.size());
    for(auto i:indices) {
        const auto& file=assets[i];
        fs::path output=dir/file.name;
        getOne(file.url,output,dry);
        if(!dry&&file.digest.rfind(L"sha256:",0)==0) {
            auto expected=file.digest.substr(7);
            auto actual=sys::wide(sys::sha256(output));
            if(_wcsicmp(expected.c_str(),actual.c_str())!=0) {
                fs::remove(output);
                throw std::runtime_error("Release file SHA-256 mismatch");
            }
        }
    }
    std::wcout<<L"Selected GitHub assets downloaded to: "<<dir.wstring()<<L"\n";
}
}
