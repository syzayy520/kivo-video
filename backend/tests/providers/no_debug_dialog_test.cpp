#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
static int fail(const std::string& msg){std::cerr<<"FAIL: "<<msg<<"\n";return 1;}
static std::string read_file(const std::string& path){
    std::ifstream f(path); if(!f) return "";
    std::string content((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());
    return content;
}
int main(){
    const char* root = "../../../..";
    const char* dirs[] = {
        "backend/include/kivo/video/providers/http",
        "backend/src/providers/http",
        "backend/tests/providers/http",
        "backend/include/kivo/video/providers/webdav",
        "backend/src/providers/webdav",
        "backend/tests/providers/webdav",
    };
    const char* banned[] = {
        "<cassert>", "assert(", "abort(", "std::abort", "terminate(", "std::terminate",
        ".value()", ".at(", "std::stoull", "std::stoi", "std::stringstream",
        "std::regex", "std::locale", "std::codecvt",
    };
    int failures = 0;
    const char* common[] = {"http_range_provider","content_range_parser","http_url",
        "webdav_bridge","test_fixture",
        "open_test","read_test","seek_test","close_test"};
    for(auto d: dirs){
        std::string base = std::string(root)+"/"+d+"/";
        std::string ext = (std::string(d).find("include")!=std::string::npos)?".hpp":".cpp";
        for(auto cn: common){
            std::string p=base+cn+ext;
            std::string c=read_file(p);
            if(c.empty()) continue;
            for(auto bn: banned){
                if(c.find(bn)!=std::string::npos){
                    auto pos=c.find(bn);
                    auto ls=c.rfind('\n',pos);
                    ls=(ls==std::string::npos)?0:ls+1;
                    if(c.find("//",ls)!=std::string::npos && c.find("//",ls)<pos) continue;
                    std::cerr<<"BANNED: "<<bn<<" in "<<p<<"\n"; failures++;
                }
            }
        }
    }
    if(failures>0) return fail("no_debug_dialog gate: "+std::to_string(failures)+" banned tokens found");
    std::cout<<"NO_DEBUG_DIALOG_GATE: PASS ("<<(sizeof(dirs)/sizeof(dirs[0]))<<" dirs scanned)"<<std::endl;
    return 0;
}
