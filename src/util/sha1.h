/* sha1.h

Copyright (c) 2005 Michael D. Leonhard

http://tamale.net/

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef SHA1_HEADER
typedef unsigned int Uint32;

class SHA1
{
	private:
		// fields
		Uint32 H0, H1, H2, H3, H4;
		unsigned char bytes[64];
		int unprocessedBytes;
		Uint32 size;
		void process();
	public:
		SHA1();
		~SHA1();
		void addBytes( const char* data, int num );
		unsigned char* getDigest();
		// utility methods
		static Uint32 lrot( Uint32 x, int bits );
		static void storeBigEndianUint32( unsigned char* byte, Uint32 num );
		static void hexPrinter( unsigned char* c, int l );
};

#define SHA1_HEADER
#endif
