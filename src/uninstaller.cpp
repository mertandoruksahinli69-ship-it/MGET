#define NOMINMAX
#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cwctype>
namespace fs=std::filesystem;
namespace {
const wchar_t* KEY=L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MGET";
std::wstring env(const wchar_t* name) {
 DWORD n=GetEnvironmentVariableW(name,nullptr,0);
 if(!n)throw std::runtime_error("Environment variable unavailable");
 std::wstring s(n,L'\0');DWORD got=GetEnvironmentVariableW(name,s.data(),n);
 if(!got||got>=n)throw std::runtime_error("Cannot read environment variable");
 s.resize(got);return s;
}
std::wstring exePath() {
 std::vector<wchar_t> out(32768);
 DWORD len=GetModuleFileNameW(nullptr,out.data(),DWORD(out.size()));
 if(!len||len>=out.size())throw std::runtime_error("Cannot determine executable path");
 return std::wstring(out.data(),len);
}
std::wstring normalize(std::wstring v) {
 std::replace(v.begin(),v.end(),L'/',L'\\');
 while(v.size()>3&&v.back()==L'\\')v.pop_back();
 std::transform(v.begin(),v.end(),v.begin(),[](wchar_t c){return std::towlower(c);});return v;
}
std::string utf8(const std::wstring& s) {
 int n=WideCharToMultiByte(CP_UTF8,0,s.data(),(int)s.size(),nullptr,0,nullptr,nullptr);
 std::string out(n,'\0');
 if(n)WideCharToMultiByte(CP_UTF8,0,s.data(),(int)s.size(),out.data(),n,nullptr,nullptr);
 return out;
}
std::wstring trim(std::wstring s) {
 const auto i=s.find_first_not_of(L" \t\r\n\""),j=s.find_last_not_of(L" \t\r\n\"");
 return i==std::wstring::npos?L"":s.substr(i,j-i+1);
}
bool confirm(){
 for(;;){
  std::cout<<"Uninstall MGET? / MGET kaldirilsin mi? [Y/N]: "<<std::flush;
  std::string s;
  if(!std::getline(std::cin,s))return false;
  if(s=="Y"||s=="y")return true;
  if(s=="N"||s=="n")return false;
  std::cout<<"Please type Y or N.\n";
 }
}
void pauseInteractive() {
 DWORD mode=0;
 if(GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),&mode)){
  std::cout<<"Press Enter to close / Kapatmak icin Enter... "<<std::flush;
  std::string s;std::getline(std::cin,s);
 }
}
std::wstring registryValue(HKEY key,const wchar_t* name) {
 DWORD type=0,len=0;
 if(RegQueryValueExW(key,name,nullptr,&type,nullptr,&len)!=ERROR_SUCCESS ||
    (type!=REG_SZ&&type!=REG_EXPAND_SZ))return {};
 std::vector<BYTE> bytes(len+sizeof(wchar_t),0);
 if(RegQueryValueExW(key,name,nullptr,&type,bytes.data(),&len)!=ERROR_SUCCESS)return {};
 return std::wstring((const wchar_t*)bytes.data());
}
bool matchesInstalledLocation(const fs::path& dir) {
 HKEY key=nullptr;
 if(RegOpenKeyExW(HKEY_CURRENT_USER,KEY,0,KEY_READ,&key)!=ERROR_SUCCESS)return false;
 std::wstring loc=registryValue(key,L"InstallLocation");RegCloseKey(key);
 return !loc.empty()&&normalize(loc)==normalize(dir.wstring());
}
void removePath(const fs::path& dir) {
 HKEY key=nullptr;
 if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Environment",0,KEY_READ|KEY_SET_VALUE,&key)!=ERROR_SUCCESS)
   throw std::runtime_error("Cannot open user environment");
 try{
  DWORD type=REG_EXPAND_SZ,len=0;
  LSTATUS rc=RegQueryValueExW(key,L"Path",nullptr,&type,nullptr,&len);
  if(rc==ERROR_FILE_NOT_FOUND){RegCloseKey(key);return;}
  if(rc!=ERROR_SUCCESS||(type!=REG_SZ&&type!=REG_EXPAND_SZ))
    throw std::runtime_error("Cannot read user PATH");
  std::vector<BYTE> raw(len+sizeof(wchar_t),0);
  if(RegQueryValueExW(key,L"Path",nullptr,&type,raw.data(),&len)!=ERROR_SUCCESS)
    throw std::runtime_error("Cannot fetch user PATH");
  std::wstring current((const wchar_t*)raw.data()),updated;
  size_t pos=0;bool changed=false;
  while(pos<current.size()){
   size_t end=current.find(L';',pos);
   if(end==std::wstring::npos)end=current.size();
   std::wstring token=current.substr(pos,end-pos);
   if(normalize(trim(token))==normalize(dir.wstring()))changed=true;
   else if(!token.empty()){if(!updated.empty())updated+=L';';updated+=token;}
   if(end==current.size())break;pos=end+1;
  }
  if(changed){
   DWORD bytes=DWORD((updated.size()+1)*sizeof(wchar_t));
   if(RegSetValueExW(key,L"Path",0,type,(const BYTE*)updated.c_str(),bytes)!=ERROR_SUCCESS)
    throw std::runtime_error("Cannot update user PATH");
  }
 }catch(...){RegCloseKey(key);throw;}
 RegCloseKey(key);
 DWORD_PTR ignored=0;
 SendMessageTimeoutW(HWND_BROADCAST,WM_SETTINGCHANGE,0,(LPARAM)L"Environment",
                     SMTO_ABORTIFHUNG,3000,&ignored);
}
void removeInstalledFiles(const fs::path& dir) {
 // Only own binaries are touched. Packages and user data are preserved.
 for(auto name:{L"mget.exe",L"MGET-Uninstall.exe",L"mget.old"}){
  fs::path p=dir/name;
  if(fs::is_regular_file(p)&&!fs::is_symlink(p)){
   std::error_code ec;fs::remove(p,ec);
   if(ec)throw std::runtime_error("Cannot remove file (close MGET and retry): "+utf8(p.wstring()));
  }
 }
 std::error_code ec;fs::remove(dir,ec); // only removes empty directories
}
int cleanup(const fs::path& dir,DWORD parent) {
 if(dir.empty()||dir==dir.root_path())throw std::runtime_error("Unsafe target directory");
 if(parent){
  HANDLE h=OpenProcess(SYNCHRONIZE,FALSE,parent);
  if(h){DWORD r=WaitForSingleObject(h,30000);CloseHandle(h);
    if(r!=WAIT_OBJECT_0)throw std::runtime_error("Parent uninstaller still running");}
 }
 std::cout<<"\nRemoving MGET binaries...\n";
 removeInstalledFiles(dir);
 // Check a marker before modifying the system registration or user PATH.
 if(matchesInstalledLocation(dir)){
  removePath(dir);
  if(RegDeleteKeyW(HKEY_CURRENT_USER,KEY)!=ERROR_SUCCESS)
   throw std::runtime_error("Cannot remove uninstall registration");
  std::cout<<"User PATH and uninstall registration cleaned.\n";
 } else std::cout<<"No matching uninstall registration; leaving user PATH untouched.\n";
 std::cout<<"MGET removed. User package data was preserved.\n";
 return 0;
}
std::wstring quote(const std::wstring& s){return L"\""+s+L"\"";}
int launchHelper(const fs::path& original){
 fs::path temp=fs::path(env(L"TEMP"))/(L"MGET-Uninstall-"+std::to_wstring(GetCurrentProcessId())+L".exe");
 if(!CopyFileW(exePath().c_str(),temp.c_str(),FALSE))
  throw std::runtime_error("Cannot create temporary cleanup helper");
 std::wstring command=quote(temp.wstring())+L" --cleanup "+quote(original.wstring())+
                      L" "+std::to_wstring(GetCurrentProcessId());
 STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
 std::vector<wchar_t> cmd(command.begin(),command.end());cmd.push_back(0);
 if(!CreateProcessW(temp.c_str(),cmd.data(),nullptr,nullptr,FALSE,CREATE_NEW_CONSOLE,
                    nullptr,nullptr,&si,&pi))
  throw std::runtime_error("Cannot launch cleanup helper");
 CloseHandle(pi.hThread);CloseHandle(pi.hProcess);
 std::cout<<"Cleanup started in a new terminal. The original window can close.\n";
 return 0;
}
}
int wmain(int argc,wchar_t** argv) {
 SetConsoleOutputCP(CP_UTF8);SetConsoleCP(CP_UTF8);
 try {
  if(argc>=2&&std::wstring(argv[1])==L"--cleanup"){
   if(argc!=4)throw std::runtime_error("Invalid internal cleanup request");
   fs::path dir=fs::path(argv[2]).lexically_normal();
   DWORD parent=std::stoul(argv[3]);
   int result=cleanup(dir,parent);
   // Temporary executable is not persistent cache; schedule deletion after process exit.
   MoveFileExW(exePath().c_str(),nullptr,MOVEFILE_DELAY_UNTIL_REBOOT);
   pauseInteractive();
   return result;
  }
  if(argc>=2&&std::wstring(argv[1])==L"--help"){
   std::cout<<"MGET-Uninstall.exe : interactive Windows uninstaller\n";return 0;
  }
  if(argc!=1)throw std::runtime_error("Unrecognized command line");
  fs::path installed=fs::path(exePath()).parent_path().lexically_normal();
  if(!fs::is_regular_file(installed/L"mget.exe"))
   throw std::runtime_error("mget.exe missing next to uninstaller");
  std::cout<<"\nMGET Uninstaller\nInstallation: "<<utf8(installed.wstring())<<"\n"
           <<"Only MGET binaries and matching PATH/registry entries are removed.\n"
           <<"Downloaded packages, local files and user data are kept.\n\n";
  if(!confirm()){std::cout<<"Cancelled. No changes made.\n";pauseInteractive();return 0;}
  // Exit immediately so the helper can wait for this process, then remove files.
  return launchHelper(installed);
 }catch(const std::exception& e){
  std::cerr<<"[ERROR] "<<e.what()<<"\n";pauseInteractive();return 1;
 }
}
