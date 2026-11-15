#pragma once
#include "platform.hpp"
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
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
    std::vector<std::wstring> parts;std::wstringstream ss(path);std::wstring s;
    while(std::getline(ss,s,L'/'))parts.push_back(s);
    if(parts.size()<2)throw std::runtime_error("GitHub URL requires owner/repository");
    Repo r;r.owner=parts[0];r.name=parts[1];
    if(r.name.size()>4&&r.name.ends_with(L".git"))r.name.resize(r.name.size()-4);
    if(!identifier(r.owner)||!identifier(r.name))throw std::runtime_error("Invalid GitHub owner/repository");
    if(parts.size()==2)return r;
    if(parts[2]!=L"releases")throw std::runtime_error("Expected GitHub repository URL or /releases URL");
    r.releases=true;
    if(parts.size()==3||(parts.size()==4&&parts[3]==L"latest"))return r;
    if(parts.size()==5&&parts[3]==L"tag") {
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
    HttpHandle(const HttpHandle&)=delete;
    HttpHandle& operator=(const HttpHandle&)=delete;
    explicit HttpHandle(HINTERNET handle):h(handle){}
};
inline std::string fetchPage(const std::wstring& address) {
    const std::wstring prefix=L"https://github.com/";
    if(address.rfind(prefix,0)!=0)throw std::runtime_error("Only public GitHub pages can be fetched");
    URL_COMPONENTSW c{};c.dwStructSize=sizeof(c);
    wchar_t host[256]={},route[16384]={},extra[4096]={};
    c.lpszHostName=host;c.dwHostNameLength=256;
    c.lpszUrlPath=route;c.dwUrlPathLength=16384;
    c.lpszExtraInfo=extra;c.dwExtraInfoLength=4096;
    if(!WinHttpCrackUrl(address.c_str(),0,0,&c))throw std::runtime_error("Bad GitHub page URL");
    if(_wcsicmp(host,L"github.com")!=0)throw std::runtime_error("Non-GitHub host rejected");
    HttpHandle session(WinHttpOpen(L"MGET/0.1 (Windows GitHub HTML client)",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));
    if(!session.h)throw std::runtime_error("Cannot open GitHub web session");
    HttpHandle connection(WinHttpConnect(session.h,L"github.com",INTERNET_DEFAULT_HTTPS_PORT,0));
    if(!connection.h)throw std::runtime_error("Cannot connect to github.com");
    std::wstring uri=std::wstring(route)+extra;
    HttpHandle req(WinHttpOpenRequest(connection.h,L"GET",uri.c_str(),nullptr,
        WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));
    if(!req.h)throw std::runtime_error("Cannot prepare GitHub page request");
    WinHttpSetTimeouts(req.h,12000,12000,30000,30000);
    const wchar_t headers[]=L"Accept: text/html\r\n";
    if(!WinHttpSendRequest(req.h,headers,(DWORD)-1,WINHTTP_NO_REQUEST_DATA,0,0,0)
        ||!WinHttpReceiveResponse(req.h,nullptr))
        throw std::runtime_error("Failed to fetch GitHub web page");
    DWORD status=0,len=sizeof(status);
    if(!WinHttpQueryHeaders(req.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,&status,&len,WINHTTP_NO_HEADER_INDEX))
        throw std::runtime_error("Cannot inspect GitHub HTTP response");
    if(status!=200)
        throw std::runtime_error("GitHub page returned HTTP "+std::to_string(status)+
            "; check URL, connectivity or GitHub access");
    std::string html;char bytes[32768];DWORD read=0;
    for(;;) {
        if(!WinHttpReadData(req.h,bytes,sizeof(bytes),&read))
            throw std::runtime_error("GitHub web page download interrupted");
        if(!read)break;
        if(html.size()+read>8*1024*1024)
            throw std::runtime_error("GitHub web page too large");
        html.append(bytes,read);
    }
    return html; // In memory only: MGET never caches HTML pages.
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
inline std::string decodePercent(const std::string& s) {
    auto hex=[](char c)->int {
        if(c>='0'&&c<='9')return c-'0';
        if(c>='a'&&c<='f')return c-'a'+10;
        if(c>='A'&&c<='F')return c-'A'+10;
        return -1;
    };
    std::string result;
    for(size_t i=0;i<s.size();++i) {
        if(s[i]=='%'&&i+2<s.size()) {
            int a=hex(s[i+1]),b=hex(s[i+2]);
            if(a>=0&&b>=0){result+=(char)(a*16+b);i+=2;continue;}
        }
        result+=s[i];
    }
    return result;
}
inline std::wstring defaultBranch(const Repo& repo) {
    const auto html=fetchPage(L"https://github.com/"+repo.owner+L"/"+repo.name);
    const std::string key="\"defaultBranch\":\"";
    auto i=html.find(key);
    if(i==std::string::npos)throw std::runtime_error("Repository default branch absent in GitHub web page");
    i+=key.size();auto j=html.find('"',i);
    if(j==std::string::npos)throw std::runtime_error("Malformed GitHub default branch");
    std::wstring branch=sys::wide(html.substr(i,j-i));
    if(branch.empty()||branch.size()>200)throw std::runtime_error("Invalid default branch");
    for(wchar_t c:branch)
        if(!((c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||
             (c>=L'0'&&c<=L'9')||c==L'_'||c==L'-'||c==L'.'||c==L'/'))
            throw std::runtime_error("Unsupported branch in GitHub web page");
    return branch;
}
inline std::vector<size_t> selection(const std::wstring& input,size_t maximum) {
    // Any hyphen-separated item numbers, in any order; NOT a numeric range.
    std::vector<size_t> result;std::set<size_t> seen;
    std::wstringstream ss(input);std::wstring part;
    while(std::getline(ss,part,L'-')) {
        part=sys::trim(part);
        if(part.empty()||part.size()>8)throw std::runtime_error("Selection must be numbers separated by '-'");
        size_t number=0;
        for(wchar_t c:part) {
            if(c<L'0'||c>L'9')throw std::runtime_error("Selection must contain numbers only");
            number=number*10+(c-L'0');
        }
        if(number==0||number>maximum)
            throw std::runtime_error("Choose a number between 1 and "+std::to_string(maximum)+
                                     " (0 is not a command)");
        if(seen.insert(number-1).second)result.push_back(number-1);
    }
    if(result.empty())throw std::runtime_error("No file selected");
    return result;
}
inline std::string releaseFragmentUrl(const std::string& html,const Repo& repo) {
    const std::string marker="/releases/expanded_assets/";
    const auto pos=html.find(marker);
    if(pos==std::string::npos)
        throw std::runtime_error("No expanded release assets section found on GitHub page");
    const auto src=html.rfind("src=\"",pos);
    if(src==std::string::npos||pos-src>200)
        throw std::runtime_error("Cannot locate release assets HTML link");
    const auto begin=src+5,end=html.find('"',begin);
    if(end==std::string::npos)throw std::runtime_error("Invalid GitHub assets section");
    std::string link=html.substr(begin,end-begin);
    const auto expected="https://github.com/"+sys::utf8(repo.owner)+"/"+sys::utf8(repo.name)+marker;
    if(link.rfind(expected,0)!=0)
        throw std::runtime_error("Unexpected release assets URL");
    if(link.find("..")!=std::string::npos)
        throw std::runtime_error("Unsafe release assets URL");
    return link;
}
struct Asset {std::wstring name,url;};
inline std::vector<Asset> parseAssets(const std::string& fragment,const Repo& repo) {
    const std::string prefix="/"+sys::utf8(repo.owner)+"/"+sys::utf8(repo.name)+"/releases/download/";
    std::vector<Asset> assets;std::set<std::string> seen;
    size_t from=0;
    while(true) {
        auto at=fragment.find("href=\"",from);
        if(at==std::string::npos)break;
        at+=6;auto end=fragment.find('"',at);
        if(end==std::string::npos)break;
        auto href=fragment.substr(at,end-at);
        from=end+1;
        if(href.rfind(prefix,0)!=0||!seen.insert(href).second)continue;
        auto rest=href.substr(prefix.size());
        auto slash=rest.find('/');
        if(slash==std::string::npos||slash==0||slash==rest.size()-1)continue;
        auto filename=rest.substr(slash+1);
        if(filename.find('/')!=std::string::npos||filename.find('?')!=std::string::npos||
           filename.find('#')!=std::string::npos||filename.find('&')!=std::string::npos)continue;
        auto name=sanitize(sys::wide(decodePercent(filename)));
        assets.push_back({name,sys::wide("https://github.com"+href)});
        if(assets.size()>1000)throw std::runtime_error("Too many GitHub release assets");
    }
    return assets;
}
inline void getOne(const std::wstring& url,const fs::path& dest,bool dry) {
    if(!dest.has_filename())throw std::runtime_error("Destination must be a file");
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
    fs::path dir;
    if(loc.empty()) {
        if(dry)dir=L"<selected folder>";
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
        auto link=L"https://github.com/"+r.owner+L"/"+r.name+L"/archive/refs/heads/"+branch+L".zip";
        std::wcout<<L"GitHub repository: "<<r.owner<<L"/"<<r.name<<L"\nDefault branch: "<<branch<<L"\n";
        getOne(link,dir/sanitize(r.name+L"-"+safeBranch+L".zip"),dry);
        return;
    }
    auto page=L"https://github.com/"+r.owner+L"/"+r.name+L"/releases/";
    page+=r.tag.empty()?L"latest":(L"tag/"+r.tag);
    auto html=fetchPage(page);
    const auto fragmentLink=releaseFragmentUrl(html,r);
    auto fragment=fetchPage(sys::wide(fragmentLink));
    const auto assets=parseAssets(fragment,r);
    if(assets.empty()) {
        std::wcout<<L"This release has no downloadable attached files.\n";
        return;
    }
    std::wcout<<L"GitHub release: "<<r.owner<<L"/"<<r.name<<L"\n";
    for(size_t i=0;i<assets.size();++i)
        std::wcout<<L"["<<i+1<<L"] "<<assets[i].name<<L"\n";
    std::wstring selected=chosen;
    if(selected.empty()&&!all) {
        std::wcout<<L"Which files? Type any list numbers with hyphens (e.g. 4-5-2 or 1-4-5)\nSelection: "<<std::flush;
        if(!std::getline(std::wcin,selected))throw std::runtime_error("Selection cancelled");
    }
    std::vector<size_t> indices;
    if(all){for(size_t i=0;i<assets.size();++i)indices.push_back(i);}
    else indices=selection(selected,assets.size());
    for(auto i:indices)
        getOne(assets[i].url,dir/assets[i].name,dry);
    std::wcout<<L"Selected release files saved to: "<<dir.wstring()<<L"\n";
}
}
