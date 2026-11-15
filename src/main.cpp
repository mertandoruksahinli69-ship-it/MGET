#include "mini_json.hpp"
#include "platform.hpp"
#include "github.hpp"
#include <regex>
#include <chrono>
#include <thread>

using Args=std::vector<std::wstring>;
constexpr const char* VERSION="0.1.0";
static std::wstring option(const Args& a,const std::wstring& key) {
    for(size_t i=0;i<a.size();++i) {
        if(a[i]==key && i+1<a.size())return a[i+1];
        if(a[i].rfind(key+L"=",0)==0)return a[i].substr(key.size()+1);
        if(key==L"--loc"&&a[i].rfind(L"--loc{",0)==0&&a[i].back()==L'}')
            return a[i].substr(6,a[i].size()-7);
    }
    return L"";
}
static bool flag(const Args& a,const std::wstring& key) {
    return std::find(a.begin(),a.end(),key)!=a.end();
}
static std::wstring lower(std::wstring s) {
    std::transform(s.begin(),s.end(),s.begin(),[](wchar_t c){return std::towlower(c);});return s;
}
static bool url(const std::wstring& x){return x.rfind(L"https://",0)==0;}
static std::wstring makeId(const fs::path& p) {
    auto s=p.stem().wstring();for(auto& c:s)if(c==L'\t'||c==L'\n'||c==L'\r')c=L'_';return s;
}
static fs::path stateFile(){return sys::appData()/L"installed.tsv";}
static void record(const std::wstring& id,const fs::path& path) {
    if(id.find_first_of(L"\t\r\n")!=std::wstring::npos)throw std::runtime_error("Unsafe package ID");
    auto p=path.wstring();if(p.find_first_of(L"\t\r\n")!=std::wstring::npos)throw std::runtime_error("Unsafe tracked file path");
    fs::create_directories(sys::appData());
    std::ofstream f(stateFile(),std::ios::binary|std::ios::app);
    if(!f)throw std::runtime_error("Cannot save package record");
    f<<sys::utf8(id)<<"\t"<<sys::utf8(p)<<"\n";
}
static fs::path destination(const Args& args) {
    std::wstring loc=option(args,L"--loc");
    if(loc.empty()){
        std::wcout<<L"Choose installation folder (Cancel to abort)...\n";
        loc=sys::chooseFolder();
        if(loc.empty())throw std::runtime_error("No destination selected");
    }
    loc=sys::normalizeLocation(loc);
    fs::path p=fs::absolute(fs::path(loc)).lexically_normal();
    if(p==p.root_path())throw std::runtime_error("Refusing to install into a drive root");
    return p;
}
static fs::path safeTarget(const fs::path& root,const fs::path& rel) {
    if(!sys::safeRelative(rel))throw std::runtime_error("Unsafe relative path in manifest");
    // Keep --dry-run side-effect-free. copyOne creates directories only when writing.
    auto parent=fs::weakly_canonical(root/rel).wstring();
    auto base=fs::weakly_canonical(root).wstring();
    if(parent.size()<base.size()||_wcsnicmp(parent.c_str(),base.c_str(),base.size())!=0||
        (parent.size()>base.size()&&parent[base.size()]!=L'\\'))
        throw std::runtime_error("Manifest path escapes destination");
    return root/rel;
}
static std::wstring replaceVar(std::wstring text,const fs::path& target) {
    const std::wstring token=L"$INSTALL_DIR";size_t pos=0;
    while((pos=text.find(token,pos))!=std::wstring::npos){text.replace(pos,token.size(),target.wstring());pos+=target.wstring().size();}
    return text;
}
static void copyOne(const fs::path& src,const fs::path& to,const std::wstring& id,bool dry=false) {
    if(fs::is_symlink(src)||!fs::is_regular_file(src))throw std::runtime_error("Source is not a regular file");
    std::wcout<<L"  "<<src.wstring()<<L" -> "<<to.wstring()<<L"\n";
    if(dry)return;
    fs::create_directories(to.parent_path());
    fs::copy_file(src,to,fs::copy_options::overwrite_existing);
    record(id,to);
}
static bool manifestFiles(const mj::Value& m,const fs::path& source,const fs::path& target,const std::wstring& id,bool dry) {
    const auto& install=m.at("install");
    const auto& files=install.at("files");
    if(files.type!=mj::Value::Array)return false;
    bool any=false;
    for(const auto& f:files.a) {
        auto raw=sys::wide(f.at("from").str());
        auto to=sys::wide(f.at("to").str("$INSTALL_DIR/"));
        if(raw.empty())throw std::runtime_error("Missing manifest 'from'");
        bool tree=raw.size()>=3&&raw.substr(raw.size()-3)==L"/**";
        fs::path input(tree?raw.substr(0,raw.size()-3):raw);
        if(!sys::safeRelative(input))throw std::runtime_error("Manifest source traversal rejected");
        auto actual=source/input;
        if(!fs::exists(actual))throw std::runtime_error("Missing manifest file: "+sys::utf8(actual.wstring()));
        std::wstring translated=replaceVar(to,target);
        fs::path baseDest(translated);
        if(baseDest.is_absolute()) {
            // Destinations must remain inside selected root.
            auto rel=baseDest.lexically_relative(target);
            if(!sys::safeRelative(rel))throw std::runtime_error("Manifest destination outside install dir");
            baseDest=rel;
        }
        if(!sys::safeRelative(baseDest))throw std::runtime_error("Invalid destination mapping");
        if(tree) {
            if(!fs::is_directory(actual))throw std::runtime_error("Expected directory for /**");
            for(const auto& e:fs::recursive_directory_iterator(actual,fs::directory_options::skip_permission_denied)) {
                if(e.is_symlink())throw std::runtime_error("Symlink in archive rejected");
                if(!e.is_regular_file())continue;
                auto rel=e.path().lexically_relative(actual);
                copyOne(e.path(),safeTarget(target,baseDest/rel),id,dry);any=true;
            }
        } else {
            bool folder=to.ends_with(L"/")||to.ends_with(L"\\")||to==L"$INSTALL_DIR";
            auto rel=folder?baseDest/input.filename():baseDest;
            copyOne(actual,safeTarget(target,rel),id,dry);any=true;
        }
    }
    return any;
}
static void applyManifest(const fs::path& file,const fs::path& source,const Args& args) {
    auto m=mj::parse(sys::read(file));
    if(m.type!=mj::Value::Object||m.at("schemaVersion").n!=1)throw std::runtime_error("Unsupported .mget schemaVersion");
    auto name=sys::wide(m.at("id").str(m.at("name").str("package")));
    auto install=m.at("install");
    std::wstring mode=sys::wide(install.at("mode").str("copy"));
    fs::path target=destination(args);
    bool dry=flag(args,L"--dry-run");
    std::wcout<<L"MGET manifest: "<<file.wstring()<<L"\nID: "<<name<<L"\nMode: "<<mode<<L"\nTarget: "<<target.wstring()<<L"\n";
    const auto& regs=install.at("registry"),links=install.at("shortcuts"), environment=install.at("environment");
    bool sensitive=(regs.type==mj::Value::Array&&!regs.a.empty())||
      (links.type==mj::Value::Array&&!links.a.empty())||environment.has("addToPath");
    if(sensitive&&!dry&&!sys::confirm(L"Manifest requests Windows system changes. Allow?"))
        throw std::runtime_error("Manifest changes declined");
    if(mode==L"copy") {
        if(!manifestFiles(m,source,target,name,dry))throw std::runtime_error("No files mapped in manifest");
    } else if(mode==L"installer") {
        auto exe=sys::wide(install.at("installer").str());
        if(!sys::safeRelative(exe))throw std::runtime_error("Invalid installer path");
        auto p=source/exe;
        if(!fs::is_regular_file(p)||fs::is_symlink(p))throw std::runtime_error("Installer not found");
        if(!dry){
            if(!sys::confirm(L"Run installation program "+p.wstring()+L"?"))throw std::runtime_error("Installer declined");
            std::wstring ext=lower(p.extension().wstring());
            if(ext==L".msi") {
                if(sys::run(L"msiexec.exe",{L"/i",p.wstring()})!=0)throw std::runtime_error("MSI installer returned failure");
            } else if(ext==L".exe") {
                SHELLEXECUTEINFOW sh{};sh.cbSize=sizeof(sh);sh.fMask=SEE_MASK_NOCLOSEPROCESS;sh.lpVerb=L"open";sh.lpFile=p.c_str();sh.nShow=SW_SHOWNORMAL;
                if(!ShellExecuteExW(&sh))throw std::runtime_error("Cannot launch installer");
                if(sh.hProcess){WaitForSingleObject(sh.hProcess,INFINITE);CloseHandle(sh.hProcess);}
            }else throw std::runtime_error("Unsupported installer type");
        }
    } else throw std::runtime_error("Unknown install mode");
    if(regs.type==mj::Value::Array)for(const auto& r:regs.a) {
        if(r.at("action").str("set")!="set"||r.at("type").str("REG_SZ")!="REG_SZ")
            throw std::runtime_error("Only REG_SZ set supported in v0.1");
        auto hive=sys::wide(r.at("hive").str("HKCU")),key=sys::wide(r.at("key").str());
        auto value=replaceVar(sys::wide(r.at("value").str()),target);
        if(key.empty())throw std::runtime_error("Missing registry key");
        std::wcout<<L"Registry: "<<hive<<L"\\"<<key<<L"\n";
        if(!dry)sys::setRegistry(hive,key,sys::wide(r.at("name").str()),value);
    }
    if(links.type==mj::Value::Array)for(const auto& l:links.a){
        auto title=sys::wide(l.at("name").str());
        auto targetLink=fs::path(replaceVar(sys::wide(l.at("target").str()),target));
        if(title.empty()||title.find_first_of(L"\\/:*?\"<>|")!=std::wstring::npos)
            throw std::runtime_error("Invalid shortcut name");
        if(!dry)sys::shortcut(title,targetLink);
    }
    if(environment.has("addToPath"))std::wcout<<L"PATH request recorded in manifest; not applied automatically in v0.1.\n";
    std::wcout<<L"Manifest installation complete.\n";
}
static void inspect(const fs::path& p) {
    auto m=mj::parse(sys::read(p));
    if(m.type!=mj::Value::Object)throw std::runtime_error("Expected manifest object");
    std::wcout<<L"Schema: "<<m.at("schemaVersion").n<<L"\nID: "<<sys::wide(m.at("id").str())
        <<L"\nName: "<<sys::wide(m.at("name").str())<<L"\nVersion: "<<sys::wide(m.at("version").str())
        <<L"\nInstall mode: "<<sys::wide(m.at("install").at("mode").str("copy"))<<L"\n";
}
static void cmdSearch(const Args& a) {
    if(a.size()<2)throw std::runtime_error("Usage: mget search <name>");
    auto src=lower(option(a,L"--source"));
    if(src.empty()||src==L"all"||src==L"winget"){
        if(sys::hasTool(L"winget.exe")) {std::wcout<<L"\n[WinGet]\n";sys::run(L"winget.exe",{L"search",a[1],L"--accept-source-agreements"});}
        else std::wcout<<L"WinGet not installed.\n";
    }
    if(src.empty()||src==L"all"||src==L"choco"){
        if(sys::hasTool(L"choco.exe")) {std::wcout<<L"\n[Chocolatey]\n";sys::run(L"choco.exe",{L"search",a[1],L"--limit-output"});}
        else std::wcout<<L"Chocolatey not installed.\n";
    }
    if(src==L"npm"){if(sys::hasTool(L"npm.cmd"))sys::run(L"cmd.exe",{L"/d",L"/c",L"npm",L"search",a[1]});else throw std::runtime_error("npm missing");}
}
static void installNamed(const Args& a) {
    std::wstring name=a[1],src=lower(option(a,L"--source"));
    if(name.rfind(L"npm:",0)==0){src=L"npm";name=name.substr(4);}
    if(src==L"npm") {
        if(!std::regex_match(name,std::wregex(LR"([@a-zA-Z0-9_./-]+)"))||name.find(L"..")!=std::wstring::npos)
            throw std::runtime_error("Invalid NPM package name");
        if(!sys::hasTool(L"npm.cmd"))throw std::runtime_error("Node.js/npm unavailable");
        auto project=option(a,L"--project");
        if(project.empty())throw std::runtime_error("Use --project=<path> for npm dependencies");
        if(flag(a,L"--dry-run")){std::wcout<<L"Dry run: npm install "<<name<<L"\n";return;}
        int result=sys::run(L"cmd.exe",{L"/d",L"/c",L"npm",L"install",name},sys::normalizeLocation(project));
        if(result)throw std::runtime_error("npm failed");return;
    }
    if(src.empty()||src==L"auto"){
        std::wcout<<L"Searching WinGet and Chocolatey for "<<name<<L"...\n";
        cmdSearch({L"search",name,L"--source=all"});
        std::wcout<<L"Install from: 1) winget 2) choco 0) cancel: ";
        std::wstring selection;std::getline(std::wcin,selection);
        if(selection==L"1")src=L"winget";
        else if(selection==L"2")src=L"choco";
        else throw std::runtime_error("Cancelled");
    }
    if(!option(a,L"--loc").empty())
        std::wcout<<L"Note: provider controls destination; --loc cannot override its installer settings.\n";
    if(flag(a,L"--dry-run")){std::wcout<<L"Dry run: "<<src<<L" install "<<name<<L"\n";return;}
    if(src==L"winget") {
        if(!sys::hasTool(L"winget.exe"))throw std::runtime_error("WinGet unavailable");
        int status=sys::run(L"winget.exe",{L"install",name,L"--accept-package-agreements",L"--accept-source-agreements"});
        if(status)throw std::runtime_error("WinGet installation failed");
    }else if(src==L"choco"){
        if(!sys::hasTool(L"choco.exe"))throw std::runtime_error("Chocolatey unavailable");
        int status=sys::run(L"choco.exe",{L"install",name,L"-y"});
        if(status)throw std::runtime_error("Chocolatey installation failed");
    }else throw std::runtime_error("Unknown provider");
}
static void installPath(const fs::path& path,const Args& a) {
    if(!fs::exists(path))throw std::runtime_error("Source path not found");
    auto custom=option(a,L"--manifest");
    fs::path root=fs::is_directory(path)?path:path.parent_path();
    fs::path manifest=custom.empty()?root/L".mget":fs::path(custom);
    if(flag(a,L"--skip-auto-manifest")&&custom.empty())manifest.clear();
    if(!manifest.empty()&&fs::exists(manifest)) {applyManifest(manifest,root,a);return;}
    if(flag(a,L"--use-.mget"))throw std::runtime_error(".mget required but not found");
    if(fs::is_directory(path)){
        auto target=destination(a);
        for(const auto& e:fs::recursive_directory_iterator(path)){
            if(e.is_symlink())throw std::runtime_error("Symlink in source rejected");
            if(!e.is_regular_file())continue;
            auto relative=e.path().lexically_relative(path);
            copyOne(e.path(),safeTarget(target,relative),makeId(path),flag(a,L"--dry-run"));
        }
        return;
    }
    std::wstring ext=lower(path.extension().wstring());
    if(ext==L".exe"||ext==L".msi") {
        if(flag(a,L"--dry-run")){std::wcout<<L"Dry run: launch "<<path.wstring()<<L"\n";return;}
        if(!sys::confirm(L"Open installer "+path.wstring()+L"?"))throw std::runtime_error("Declined");
        if(ext==L".msi"){if(sys::run(L"msiexec.exe",{L"/i",path.wstring()})!=0)throw std::runtime_error("MSI failed");}
        else {
            SHELLEXECUTEINFOW sh{};sh.cbSize=sizeof(sh);sh.fMask=SEE_MASK_NOCLOSEPROCESS;
            sh.lpVerb=L"open";sh.lpFile=path.c_str();sh.nShow=SW_SHOWNORMAL;
            if(!ShellExecuteExW(&sh))throw std::runtime_error("Installer launch failed");
            if(sh.hProcess){WaitForSingleObject(sh.hProcess,INFINITE);CloseHandle(sh.hProcess);}
        }
        return;
    }
    fs::path target=destination(a);
    if(ext==L".zip"){
        if(flag(a,L"--dry-run")){std::wcout<<L"Dry run: expand "<<path.wstring()<<L" to "<<target.wstring()<<L"\n";return;}
        if(!sys::confirm(L"Extract archive to "+target.wstring()+L"?"))throw std::runtime_error("Declined");
        fs::create_directories(target);
        std::wstring escSrc=path.wstring(),escDst=target.wstring();
        auto esc=[](std::wstring s){size_t p=0;while((p=s.find(L"'",p))!=std::wstring::npos){s.insert(p,L"'");p+=2;}return s;};
        std::wstring ps=L"Expand-Archive -LiteralPath '"+esc(escSrc)+L"' -DestinationPath '"+esc(escDst)+L"' -Force";
        if(sys::run(L"powershell.exe",{L"-NoProfile",L"-NonInteractive",L"-Command",ps})!=0)throw std::runtime_error("Unzip failed");
        std::wcout<<L"Extracted. Review files before running executables.\n";
        return;
    }
    copyOne(path,safeTarget(target,path.filename()),makeId(path),flag(a,L"--dry-run"));
}
static void cmdInstall(const Args& a){
    if(a.size()<2)throw std::runtime_error("Usage: mget install <name|url|path>");
    auto input=a[1];
    if(url(input)){
        if(gh::isGitHub(input)){
            gh::handle(input,option(a,L"--loc"),flag(a,L"--dry-run"),
                       flag(a,L"--all"),option(a,L"--select"));
            return;
        }
        if(flag(a,L"--dry-run")) {std::wcout<<L"Dry run: download "<<input<<L"\n";return;}
        auto target=destination(a);
        auto file=target/sys::urlFilename(input);
        if(fs::exists(file)&&!sys::confirm(L"Overwrite existing file "+file.wstring()+L"?"))
            throw std::runtime_error("Overwrite declined");
        std::wcout<<L"Downloading directly to "<<file.wstring()<<L"\n";
        sys::download(input,file);
        std::wcout<<L"Saved: "<<file.wstring()<<L"\n";
        auto ext=lower(file.extension().wstring());
        if(ext==L".exe"||ext==L".msi"||ext==L".zip") {
            Args next=a;next.push_back(L"--skip-auto-manifest");
            next.push_back(L"--loc="+target.wstring());
            installPath(file,next);
        }
        return;
    }
    if(input.rfind(L"http://",0)==0)throw std::runtime_error("HTTP not allowed; use HTTPS");
    if(fs::exists(input))installPath(fs::absolute(input),a);
    else installNamed(a);
}
static void cmdPull(const Args& a){
    if(a.size()<2||!url(a[1]))throw std::runtime_error("Usage: mget pull https://...");
    if(gh::isGitHub(a[1])){
        gh::handle(a[1],option(a,L"--loc"),flag(a,L"--dry-run"),
                   flag(a,L"--all"),option(a,L"--select"));
        return;
    }
    if(flag(a,L"--dry-run")){std::wcout<<L"Dry run: download "<<a[1]<<L"\n";return;}
    auto dir=destination(a);
    auto p=dir/sys::urlFilename(a[1]);
    if(fs::exists(p)&&!sys::confirm(L"Overwrite existing file "+p.wstring()+L"?"))
        throw std::runtime_error("Overwrite declined");
    sys::download(a[1],p);
    std::wcout<<L"Downloaded: "<<p.wstring()<<L"\nSHA256: "<<sys::wide(sys::sha256(p))<<L"\n";
}
static void cmdList(){
    if(!fs::exists(stateFile())){std::wcout<<L"No MGET-tracked files.\n";return;}
    std::wcout<<sys::wide(sys::read(stateFile()));
}
static void cmdRemove(const Args& a){
    if(a.size()<2)throw std::runtime_error("Usage: mget remove <name>");
    if(!fs::exists(stateFile()))throw std::runtime_error("No MGET-tracked packages");
    std::istringstream in(sys::read(stateFile()));std::string line;
    std::vector<std::string> keep;std::vector<fs::path> removeFiles;
    while(std::getline(in,line)) {
        auto tab=line.find('\t');
        if(tab==std::string::npos)continue;
        if(line.substr(0,tab)==sys::utf8(a[1]))removeFiles.push_back(fs::path(sys::wide(line.substr(tab+1))));
        else keep.push_back(line);
    }
    if(removeFiles.empty())throw std::runtime_error("No MGET-tracked files for package");
    std::wcout<<L"Tracked files to remove: "<<removeFiles.size()<<L"\n";
    for(const auto& p:removeFiles)std::wcout<<L"  "<<p.wstring()<<L"\n";
    if(!sys::confirm(L"Delete ONLY tracked files?"))throw std::runtime_error("Cancelled");
    for(const auto& p:removeFiles)if(fs::is_regular_file(p)&&!fs::is_symlink(p))fs::remove(p);
    std::ostringstream result;for(const auto& s:keep)result<<s<<"\n";sys::write(stateFile(),result.str());
    std::wcout<<L"Removed tracked files. External provider installations are not managed by this command.\n";
}

static fs::path channelFile(){return sys::appData()/L"channel.txt";}
static fs::path feedFile(){return sys::appData()/L"feed.txt";}
static std::wstring channel() {
    if(!fs::exists(channelFile()))return L"stable";
    auto c=sys::wide(sys::read(channelFile()));
    c=sys::trim(c);return c.empty()?L"stable":c;
}
static void cmdChannel(const Args& a) {
    if(a.size()<2){std::wcout<<L"Channel: "<<channel()<<L"\n";return;}
    auto c=lower(a[1]);
    if(c!=L"stable"&&c!=L"beta"&&c!=L"nightly")throw std::runtime_error("Channel must be stable, beta or nightly");
    fs::create_directories(sys::appData());
    sys::write(channelFile(),sys::utf8(c));
    auto feed=option(a,L"--feed");
    if(!feed.empty()){
        if(!url(feed))throw std::runtime_error("Update feed must use HTTPS");
        if(feed.back()!=L'/')feed+=L'/';
        sys::write(feedFile(),sys::utf8(feed));
    }
    std::wcout<<L"Update channel: "<<c<<L"\n";
    if(!fs::exists(feedFile()))std::wcout<<L"No release feed configured. Use --feed=https://YOUR-SERVER/releases/\n";
}
static std::vector<int> parseVersion(const std::string& v) {
    std::vector<int> out;std::istringstream in(v);std::string part;
    while(std::getline(in,part,'.')) {
        if(part.empty()||part.size()>7||!std::all_of(part.begin(),part.end(),[](char c){return c>='0'&&c<='9';}))
            throw std::runtime_error("Only numeric versions major.minor.patch supported");
        out.push_back(std::stoi(part));
    }
    if(out.size()!=3)throw std::runtime_error("Version must use major.minor.patch");
    return out;
}
static void applyUpdate(const Args& a){
    if(a.size()<4)throw std::runtime_error("Internal updater arguments missing");
    auto original=fs::path(a[1]),candidate=fs::path(a[2]);
    DWORD parent=std::stoul(a[3]);
    if(!fs::is_regular_file(candidate))throw std::runtime_error("Update candidate missing");
    HANDLE h=OpenProcess(SYNCHRONIZE,FALSE,parent);
    if(h){WaitForSingleObject(h,60000);CloseHandle(h);}
    fs::path backup=original;backup+=L".bak";
    bool moved=false;
    for(int i=0;i<30&&!moved;++i) {
        if(MoveFileExW(original.c_str(),backup.c_str(),MOVEFILE_REPLACE_EXISTING))moved=true;
        else std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    if(!moved)throw std::runtime_error("Cannot replace running mget.exe. Administrator access may be required.");
    if(!MoveFileExW(candidate.c_str(),original.c_str(),MOVEFILE_REPLACE_EXISTING)) {
        MoveFileExW(backup.c_str(),original.c_str(),MOVEFILE_REPLACE_EXISTING);
        throw std::runtime_error("Update failed; old version restored");
    }
    std::wcout<<L"MGET updated. Previous binary saved as "<<backup.wstring()<<L"\n";
}
static void cmdUpdate(const Args& a){
    if(!fs::exists(feedFile()))throw std::runtime_error("No update feed. Set with: mget channel stable --feed=https://YOUR-SERVER/releases/");
    std::wstring feed=sys::trim(sys::wide(sys::read(feedFile())));
    if(!url(feed))throw std::runtime_error("Configured feed is not HTTPS");
    auto c=option(a,L"--channel");if(c.empty())c=channel();
    if(c!=L"stable"&&c!=L"beta"&&c!=L"nightly")throw std::runtime_error("Invalid channel");
    fs::path tmp=sys::appData()/L"updates"/L"release.json";
    auto indexUrl=feed+c+L".json";
    std::wcout<<L"Checking MGET "<<c<<L" channel...\n";
    sys::download(indexUrl,tmp);
    auto m=mj::parse(sys::read(tmp));
    auto next=m.at("version").str();
    auto asset=sys::wide(m.at("url").str());
    auto digest=lower(sys::wide(m.at("sha256").str()));
    if(next.empty()||!url(asset)||digest.size()!=64)throw std::runtime_error("Invalid release metadata");
    if(parseVersion(next)<=parseVersion(VERSION)){std::wcout<<L"Already up to date ("<<sys::wide(VERSION)<<L").\n";return;}
    fs::path candidate=sys::appData()/L"updates"/L"mget-next.exe";
    sys::download(asset,candidate);
    if(sys::wide(sys::sha256(candidate))!=digest){fs::remove(candidate);throw std::runtime_error("Release checksum mismatch");}
    std::wcout<<L"Current: "<<sys::wide(VERSION)<<L" -> New: "<<sys::wide(next)<<L"\n";
    if(flag(a,L"--check")){std::wcout<<L"Update verified; --check will not install.\n";return;}
    if(!sys::confirm(L"Install verified MGET update?")){std::wcout<<L"Cancelled.\n";return;}
    wchar_t current[MAX_PATH];DWORD n=GetModuleFileNameW(nullptr,current,MAX_PATH);
    if(n==0||n==MAX_PATH)throw std::runtime_error("Cannot find current mget.exe");
    fs::path helper=sys::appData()/L"updates"/L"mget-updater.exe";
    fs::copy_file(current,helper,fs::copy_options::overwrite_existing);
    STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
    std::wstring cmd=sys::quote(helper.wstring())+L" --apply-update "+sys::quote(current)+L" "+sys::quote(candidate.wstring())+
      L" "+std::to_wstring(GetCurrentProcessId());
    std::vector<wchar_t> w(cmd.begin(),cmd.end());w.push_back(0);
    if(!CreateProcessW(nullptr,w.data(),nullptr,nullptr,FALSE,CREATE_NEW_CONSOLE,nullptr,nullptr,&si,&pi))
        throw std::runtime_error("Cannot start update helper");
    CloseHandle(pi.hThread);CloseHandle(pi.hProcess);
    std::wcout<<L"Update helper launched. Exiting to allow replacement.\n";
}
static void cmdUpgrade(const Args& a){
    std::wstring source=lower(option(a,L"--source"));
    if(source.empty())source=L"winget";
    if(source==L"winget"){
        if(!sys::hasTool(L"winget.exe"))throw std::runtime_error("WinGet unavailable");
        Args cmd={L"upgrade"};if(a.size()>1&&a[1].rfind(L"--",0)!=0)cmd.push_back(a[1]);
        cmd.push_back(L"--accept-package-agreements");cmd.push_back(L"--accept-source-agreements");
        if(sys::run(L"winget.exe",cmd)!=0)throw std::runtime_error("WinGet upgrade failed");
    } else if(source==L"choco"){
        if(!sys::hasTool(L"choco.exe"))throw std::runtime_error("Chocolatey unavailable");
        if(a.size()<2)throw std::runtime_error("Specify Chocolatey package for upgrade");
        if(sys::run(L"choco.exe",{L"upgrade",a[1],L"-y"})!=0)throw std::runtime_error("Chocolatey upgrade failed");
    } else throw std::runtime_error("Unsupported upgrade source");
}
static void cmdDeploy(const Args& a){
    if(a.size()<2)throw std::runtime_error("Usage: mget deploy <local-project-directory> --target=<directory>");
    auto target=option(a,L"--target");
    if(target.empty())throw std::runtime_error("Local deploy requires --target=<directory>");
    if(!fs::is_directory(a[1]))throw std::runtime_error("Deployment source must be a local directory");
    auto args=a;args.push_back(L"--loc="+target);
    installPath(fs::absolute(a[1]),args);
}
static void cmdInit(const Args& a){
    fs::path dir=a.size()>1?fs::path(a[1]):fs::current_path();
    if(!fs::is_directory(dir))throw std::runtime_error("Project directory missing");
    auto path=dir/L".mget";
    if(fs::exists(path))throw std::runtime_error(".mget already exists");
    const std::string tmpl=R"({
  "schemaVersion": 1,
  "id": "example.app",
  "name": "Example App",
  "version": "1.0.0",
  "install": {
    "mode": "copy",
    "files": [{"from": "bin/**", "to": "$INSTALL_DIR/"}],
    "registry": [],
    "shortcuts": []
  }
}
)";
    sys::write(path,tmpl);std::wcout<<L"Created: "<<path.wstring()<<L"\n";
}
static void doctor(){
    std::wcout<<L"MGET "<<sys::wide(VERSION)<<L"\n";
    for(auto name:{L"winget.exe",L"choco.exe",L"npm.cmd",L"powershell.exe"})
        std::wcout<<name<<L": "<<(sys::hasTool(name)?L"available":L"not installed")<<L"\n";
    std::wcout<<L"Channel: "<<channel()<<L"\nState: "<<sys::appData().wstring()<<L"\n";
    if(fs::exists(feedFile()))std::wcout<<L"Update feed: "<<sys::wide(sys::read(feedFile()))<<L"\n";
}
static void help(){
    std::wcout<<LR"(MGET v0.1.0 - native C++ Windows package manager
Commands:
  mget search <name> [--source=winget|choco|all|npm]
  mget install <name|https-url|local-path> [--source=auto|winget|choco|npm]
  mget install <path> --use-.mget [--loc{C:\MyApps\App}]
  mget install <path> --manifest=<path-to-mget> [--dry-run]
  mget install npm:<name> --project=<project-folder>
  mget pull <https-url> [--loc{folder}]
  mget pull https://github.com/owner/repo [--loc=folder] (source ZIP)
  mget pull https://github.com/owner/repo/releases [--select=1-2]
  mget pull https://github.com/owner/repo/releases/tag/v1.0 [--all]
  mget inspect <path-to-.mget>
  mget init [project-folder]
  mget list
  mget remove <tracked-package>
  mget upgrade [name] [--source=winget|choco]
  mget deploy <local-folder> --target=<folder>
  mget channel [stable|beta|nightly] [--feed=https://server/releases/]
  mget --update [--check] [--channel=stable|beta|nightly]
  mget doctor
  mget --version
Options:
  --loc{folder}  or  --loc=<folder>: target for copied files
  --use-.mget  require a .mget manifest
  --manifest=<file>  explicit .mget manifest
  --dry-run  show planned file operations without applying
The folder picker opens for local installs when --loc is omitted.
Remote executable installers require explicit confirmation.
)";
}
int wmain(int argc,wchar_t** argv){
    SetConsoleOutputCP(CP_UTF8);SetConsoleCP(CP_UTF8);
    Args a;for(int i=1;i<argc;i++)a.push_back(argv[i]);
    try {
        if(a.empty()||a[0]==L"help"||a[0]==L"--help"||a[0]==L"-h"){help();return 0;}
        const auto cmd=lower(a[0]);
        if(cmd==L"--version"||cmd==L"version"){std::wcout<<L"MGET "<<sys::wide(VERSION)<<L"\n";return 0;}
        if(cmd==L"search"){cmdSearch(a);return 0;}
        if(cmd==L"install"){cmdInstall(a);return 0;}
        if(cmd==L"pull"){cmdPull(a);return 0;}
        if(cmd==L"inspect"){if(a.size()<2)throw std::runtime_error("Usage: mget inspect <file>");inspect(a[1]);return 0;}
        if(cmd==L"init"){cmdInit(a);return 0;}
        if(cmd==L"list"){cmdList();return 0;}
        if(cmd==L"remove"){cmdRemove(a);return 0;}
        if(cmd==L"upgrade"){cmdUpgrade(a);return 0;}
        if(cmd==L"deploy"){cmdDeploy(a);return 0;}
        if(cmd==L"channel"){cmdChannel(a);return 0;}
        if(cmd==L"--update"){cmdUpdate(a);return 0;}
        if(cmd==L"--apply-update"){applyUpdate(a);return 0;}
        if(cmd==L"doctor"){doctor();return 0;}
        throw std::runtime_error("Unknown command. Run mget help");
    } catch(const std::exception& e) {
        std::cerr<<"[MGET ERROR] "<<e.what()<<"\n";return 1;
    }
}
