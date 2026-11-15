#pragma once
#include <cctype>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>

namespace mj {
struct Value {
    enum Type { Null, Bool, Number, String, Array, Object } type = Null;
    bool b = false;
    double n = 0;
    std::string s;
    std::vector<Value> a;
    std::map<std::string, Value> o;
    const Value& at(const std::string& k) const {
        static const Value empty;
        auto it = o.find(k);
        return type == Object && it != o.end() ? it->second : empty;
    }
    std::string str(const std::string& d = "") const { return type == String ? s : d; }
    bool boolean(bool d = false) const { return type == Bool ? b : d; }
    bool has(const std::string& k) const { return type == Object && o.count(k) > 0; }
};
class Parser {
    const std::string& t;
    size_t p=0;
    void ws() { while(p<t.size() && std::isspace((unsigned char)t[p])) ++p; }
    [[noreturn]] void fail(const char* why) const {
        throw std::runtime_error(std::string("JSON ")+why+" at offset "+std::to_string(p));
    }
    char take() { if(p>=t.size()) fail("unexpected end"); return t[p++]; }
    void expect(char c) { ws(); if(take()!=c) fail("unexpected character"); }
    unsigned hex4() {
        unsigned v=0;
        for(int i=0;i<4;i++) {
            char c=take(); v<<=4;
            if(c>='0'&&c<='9') v|=c-'0';
            else if(c>='a'&&c<='f') v|=c-'a'+10;
            else if(c>='A'&&c<='F') v|=c-'A'+10;
            else fail("invalid unicode escape");
        }
        return v;
    }
    static void utf8(std::string& s, unsigned u) {
        if(u<=0x7f) s+=char(u);
        else if(u<=0x7ff) {s+=char(0xc0|(u>>6));s+=char(0x80|(u&63));}
        else if(u<=0xffff) {s+=char(0xe0|(u>>12));s+=char(0x80|((u>>6)&63));s+=char(0x80|(u&63));}
        else {s+=char(0xf0|(u>>18));s+=char(0x80|((u>>12)&63));s+=char(0x80|((u>>6)&63));s+=char(0x80|(u&63));}
    }
    std::string string() {
        if(take()!='"') fail("expected string");
        std::string out;
        while(true) {
            char c=take();
            if(c=='"') return out;
            if((unsigned char)c<0x20) fail("control character");
            if(c!='\\') {out+=c;continue;}
            c=take();
            switch(c) {
                case '"':out+='"';break;case '\\':out+='\\';break;
                case '/':out+='/';break;case 'b':out+='\b';break;
                case 'f':out+='\f';break;case 'n':out+='\n';break;
                case 'r':out+='\r';break;case 't':out+='\t';break;
                case 'u': {
                    unsigned u=hex4();
                    if(u>=0xd800&&u<=0xdbff) {
                        if(take()!='\\'||take()!='u') fail("missing surrogate");
                        unsigned low=hex4();
                        if(low<0xdc00||low>0xdfff) fail("invalid surrogate");
                        u=0x10000+((u-0xd800)<<10)+(low-0xdc00);
                    } else if(u>=0xdc00&&u<=0xdfff) fail("unexpected low surrogate");
                    utf8(out,u);break;
                }
                default:fail("invalid escape");
            }
        }
    }
    Value value(int depth) {
        if(depth>64) fail("nesting limit exceeded");
        ws();
        if(p>=t.size()) fail("unexpected end");
        Value v;
        if(t[p]=='"'){v.type=Value::String;v.s=string();return v;}
        if(t[p]=='{') {
            v.type=Value::Object;++p;ws();
            if(p<t.size()&&t[p]=='}'){++p;return v;}
            while(true){
                ws();if(p>=t.size()||t[p]!='"') fail("expected key");
                auto k=string();expect(':');v.o[k]=value(depth+1);ws();
                char c=take();if(c=='}')return v;if(c!=',')fail("expected comma");
            }
        }
        if(t[p]=='[') {
            v.type=Value::Array;++p;ws();
            if(p<t.size()&&t[p]==']'){++p;return v;}
            while(true){v.a.push_back(value(depth+1));ws();char c=take();if(c==']')return v;if(c!=',')fail("expected comma");}
        }
        if(t.compare(p,4,"true")==0){p+=4;v.type=Value::Bool;v.b=true;return v;}
        if(t.compare(p,5,"false")==0){p+=5;v.type=Value::Bool;return v;}
        if(t.compare(p,4,"null")==0){p+=4;return v;}
        if(t[p]=='-'||std::isdigit((unsigned char)t[p])) {
            size_t begin=p; if(t[p]=='-')++p;
            if(p>=t.size())fail("bad number");
            if(t[p]=='0')++p;else {if(!std::isdigit((unsigned char)t[p]))fail("bad number");while(p<t.size()&&std::isdigit((unsigned char)t[p]))++p;}
            if(p<t.size()&&t[p]=='.'){++p;if(p>=t.size()||!std::isdigit((unsigned char)t[p]))fail("bad number");while(p<t.size()&&std::isdigit((unsigned char)t[p]))++p;}
            if(p<t.size()&&(t[p]=='e'||t[p]=='E')){++p;if(p<t.size()&&(t[p]=='+'||t[p]=='-'))++p;if(p>=t.size()||!std::isdigit((unsigned char)t[p]))fail("bad exponent");while(p<t.size()&&std::isdigit((unsigned char)t[p]))++p;}
            v.type=Value::Number;v.n=std::strtod(t.c_str()+begin,nullptr);return v;
        }
        fail("invalid value");
    }
public:
    explicit Parser(const std::string& text):t(text){}
    Value parse(){Value v=value(0);ws();if(p!=t.size())fail("trailing input");return v;}
};
inline Value parse(const std::string& s){return Parser(s).parse();}
}
