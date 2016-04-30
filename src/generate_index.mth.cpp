#include "clientmedia.h"
#include "util/hex.h"

#include <iostream>
#include <string>
using namespace std;

int main(int argc, char *argv[])
{
  string hhash;
  ClientMediaDownloader store;
  while(!cin.eof()) {
	getline(cin,hhash);
	if(hhash.size()==0) continue;
	if(hhash.size()<40) {
	  cerr << " ummm " << hhash.size() << endl;
	  continue;
	}
	unsigned char hash[20];
	for(int i=0;i<20;++i) {
	  hex_digit_decode(hhash[2*i],hash[i]);
	}
	string bhash((const char*)hash,20);
	store.addFile(hhash,bhash);
  }
  cout << store.serializeRequiredHashSet();
  return 0;
}

