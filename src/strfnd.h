/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef STRFND_HEADER
#define STRFND_HEADER

#include <string>

std::string trim(const std::string &str);

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
	void skip_over(std::string chars){
		while(p < tek.size()){
			bool is = false;
			for(unsigned int i=0; i<chars.size(); i++){
				if(chars[i] == tek[p]){
					is = true;
					break;
				}
			}
			if(!is) break;
			p++;
		}
	}
    bool atend(){
        if(p>=tek.size()) return true;
        return false;
    }
    Strfnd(std::string s){
        start(s);
    }
};

class WStrfnd{
    std::wstring tek;
    unsigned int p;
public:
    void start(std::wstring niinq){
        tek = niinq;
        p=0;
    }
    unsigned int where(){
        return p;
    }
    void to(unsigned int i){
        p = i;
    }
    std::wstring what(){
        return tek;
    }
    std::wstring next(std::wstring plop){
        //std::cout<<"tek=\""<<tek<<"\" plop=\""<<plop<<"\""<<std::endl;
        size_t n;
        std::wstring palautus;
        if (p < tek.size())
        {  
            //std::cout<<"\tp<tek.size()"<<std::endl;
            if ((n = tek.find(plop, p)) == std::wstring::npos || plop == L"")
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
    WStrfnd(std::wstring s){
        start(s);
    }
};

inline std::string trim(const std::string &s)
{
	std::string str = s;
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

