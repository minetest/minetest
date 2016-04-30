#include "clientmedia.h"

#include <iostream>
#include <string>
using namespace std;

int main(int argc, char *argv[])
{
  string hhash;
  ClientMediaDownloader store;
  while(!cin.eof()) {
	getline(cin,hhash);
	cout << "ummm " << hhash << endl;
	if(hhash.size()<=41) {
	  cerr << " error " << hhash << endl;
	  return 23;
	}
	char hash[20];
	for(int i=0;i<20;++i) {
	  hex_digit_decode(hhash[2*i],hash+i);
	}
	store.addFile(hash,hash);
  }
  cout << store.serializeRequiredHashSet();
  return 0;
}

