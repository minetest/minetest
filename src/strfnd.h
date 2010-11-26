#ifndef STRFND_HEADER
#define STRFND_HEADER

#include <string>

std::string trim(std::string str);

class Strfnd{
    std::string tek;
    unsigned int p;
public:
    void start(std::string niinq){
        tek = niinq;
        p=0;
    }
    unsigned int where(){
        return p;
    }
    void to(unsigned int i){
        p = i;
    }
    std::string what(){
        return tek;
    }
    std::string next(std::string plop){
        //std::cout<<"tek=\""<<tek<<"\" plop=\""<<plop<<"\""<<std::endl;
        size_t n;
        std::string palautus;
        if (p < tek.size())
        {  
            //std::cout<<"\tp<tek.size()"<<std::endl;
            if ((n = tek.find(plop, p)) == std::string::npos || plop == "")
            {  
                //std::cout<<"\t\tn == string::npos || plop == \"\""<<std::endl;
                n = tek.size();
            }
            else
            {  
                //std::cout<<"\t\tn != string::npos"<<std::endl;
            }
            palautus = tek.substr(p, n-p);
            p = n + plop.length();
        }
        //else
            //std::cout<<"\tp>=tek.size()"<<std::endl;
		//std::cout<<"palautus=\""<<palautus<<"\""<<std::endl;
        return palautus;
    }
    bool atend(){
        if(p>=tek.size()) return true;
        return false;
    }
    Strfnd(std::string s){
        start(s);
    }
};

inline std::string trim(std::string str)
{
    while( 
            str.length()>0
            &&
            (
             str.substr(0,               1)==" "     ||
             str.substr(0,               1)=="\t"    ||
             str.substr(0,               1)=="\r"    ||
             str.substr(0,               1)=="\n"    ||
             str.substr(str.length()-1,  1)==" "     ||
             str.substr(str.length()-1,  1)=="\t"    ||
             str.substr(str.length()-1,  1)=="\r"    ||
             str.substr(str.length()-1,  1)=="\n"
            )
         )
    {  
        if      (str.substr(0,              1)==" ")
			str = str.substr(1,str.length()-1);
        else if (str.substr(0,              1)=="\t")
			str = str.substr(1,str.length()-1);
        else if (str.substr(0,              1)=="\r")
			str = str.substr(1,str.length()-1);
        else if (str.substr(0,              1)=="\n")
			str = str.substr(1,str.length()-1);
        else if (str.substr(str.length()-1, 1)==" ")
			str = str.substr(0,str.length()-1);
        else if (str.substr(str.length()-1, 1)=="\t")
			str = str.substr(0,str.length()-1);
        else if (str.substr(str.length()-1, 1)=="\r")
			str = str.substr(0,str.length()-1);
        else if (str.substr(str.length()-1, 1)=="\n")
			str = str.substr(0,str.length()-1);
    }
    return str;
}

#endif

