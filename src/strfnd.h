/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
    
    // Returns substr of tek up to the next occurence of plop that isn't escaped with '\'
    std::string next_esc(std::string plop) {
		size_t n, realp;
		
    	if (p >= tek.size())
    		return "";
		
		realp = p;
		do {
			n = tek.find(plop, p);
			if (n == std::string::npos || plop == "")
				n = tek.length();
			p = n + plop.length();
		} while (n > 0 && tek[n - 1] == '\\');
		
		return tek.substr(realp, n - realp);
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
    
    std::wstring next_esc(std::wstring plop) {
		size_t n, realp;
		
    	if (p >= tek.size())
    		return L"";
		
		realp = p;
		do {
			n = tek.find(plop, p);
			if (n == std::wstring::npos || plop == L"")
				n = tek.length();
			p = n + plop.length();
		} while (n > 0 && tek[n - 1] == '\\');
		
		return tek.substr(realp, n - realp);
    }
    
    bool atend(){
        if(p>=tek.size()) return true;
        return false;
    }
    WStrfnd(std::wstring s){
        start(s);
    }
};

#endif

