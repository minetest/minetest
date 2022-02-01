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

#include "serialization.h"

#include "util/serialize.h"

#include <zlib.h>
#include <zstd.h>

/* report a zlib or i/o error */
void zerr(int ret)
{
    dstream<<"zerr: ";
    switch (ret) {
    case Z_ERRNO:
        if (ferror(stdin))
            dstream<<"error reading stdin"<<std::endl;
        if (ferror(stdout))
            dstream<<"error writing stdout"<<std::endl;
        break;
    case Z_STREAM_ERROR:
        dstream<<"invalid compression level"<<std::endl;
        break;
    case Z_DATA_ERROR:
        dstream<<"invalid or incomplete deflate data"<<std::endl;
        break;
    case Z_MEM_ERROR:
        dstream<<"out of memory"<<std::endl;
        break;
    case Z_VERSION_ERROR:
        dstream<<"zlib version mismatch!"<<std::endl;
		break;
	default:
		dstream<<"return value = "<<ret<<std::endl;
    }
}

void compressZlib(const u8 *data, size_t data_size, std::ostream &os, int level)
{
	z_stream z;
	const s32 bufsize = 16384;
	char output_buffer[bufsize];
	int status = 0;
	int ret;

	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = Z_NULL;

	ret = deflateInit(&z, level);
	if(ret != Z_OK)
		throw SerializationError("compressZlib: deflateInit failed");

	// Point zlib to our input buffer
	z.next_in = (Bytef*)&data[0];
	z.avail_in = data_size;
	// And get all output
	for(;;)
	{
		z.next_out = (Bytef*)output_buffer;
		z.avail_out = bufsize;

		status = deflate(&z, Z_FINISH);
		if(status == Z_NEED_DICT || status == Z_DATA_ERROR
				|| status == Z_MEM_ERROR)
		{
			zerr(status);
			throw SerializationError("compressZlib: deflate failed");
		}
		int count = bufsize - z.avail_out;
		if(count)
			os.write(output_buffer, count);
		// This determines zlib has given all output
		if(status == Z_STREAM_END)
			break;
	}

	deflateEnd(&z);
}

void compressZlib(const std::string &data, std::ostream &os, int level)
{
	compressZlib((u8*)data.c_str(), data.size(), os, level);
}

void decompressZlib(std::istream &is, std::ostream &os, size_t limit)
{
	z_stream z;
	const s32 bufsize = 16384;
	char input_buffer[bufsize];
	char output_buffer[bufsize];
	int status = 0;
	int ret;
	int bytes_read = 0;
	int bytes_written = 0;
	int input_buffer_len = 0;

	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = Z_NULL;

	ret = inflateInit(&z);
	if(ret != Z_OK)
		throw SerializationError("dcompressZlib: inflateInit failed");

	z.avail_in = 0;

	//dstream<<"initial fail="<<is.fail()<<" bad="<<is.bad()<<std::endl;

	for(;;)
	{
		int output_size = bufsize;
		z.next_out = (Bytef*)output_buffer;
		z.avail_out = output_size;

		if (limit) {
			int limit_remaining = limit - bytes_written;
			if (limit_remaining <= 0) {
				// we're aborting ahead of time - throw an error?
				break;
			}
			if (limit_remaining < output_size) {
				z.avail_out = output_size = limit_remaining;
			}
		}

		if(z.avail_in == 0)
		{
			z.next_in = (Bytef*)input_buffer;
			is.read(input_buffer, bufsize);
			input_buffer_len = is.gcount();
			z.avail_in = input_buffer_len;
			//dstream<<"read fail="<<is.fail()<<" bad="<<is.bad()<<std::endl;
		}
		if(z.avail_in == 0)
		{
			//dstream<<"z.avail_in == 0"<<std::endl;
			break;
		}

		//dstream<<"1 z.avail_in="<<z.avail_in<<std::endl;
		status = inflate(&z, Z_NO_FLUSH);
		//dstream<<"2 z.avail_in="<<z.avail_in<<std::endl;
		bytes_read += is.gcount() - z.avail_in;
		//dstream<<"bytes_read="<<bytes_read<<std::endl;

		if(status == Z_NEED_DICT || status == Z_DATA_ERROR
				|| status == Z_MEM_ERROR)
		{
			zerr(status);
			throw SerializationError("decompressZlib: inflate failed");
		}
		int count = output_size - z.avail_out;
		//dstream<<"count="<<count<<std::endl;
		if(count)
			os.write(output_buffer, count);
		bytes_written += count;
		if(status == Z_STREAM_END)
		{
			//dstream<<"Z_STREAM_END"<<std::endl;

			//dstream<<"z.avail_in="<<z.avail_in<<std::endl;
			//dstream<<"fail="<<is.fail()<<" bad="<<is.bad()<<std::endl;
			// Unget all the data that inflate didn't take
			is.clear(); // Just in case EOF is set
			for(u32 i=0; i < z.avail_in; i++)
			{
				is.unget();
				if(is.fail() || is.bad())
				{
					dstream<<"unget #"<<i<<" failed"<<std::endl;
					dstream<<"fail="<<is.fail()<<" bad="<<is.bad()<<std::endl;
					throw SerializationError("decompressZlib: unget failed");
				}
			}

			break;
		}
	}

	inflateEnd(&z);
}

struct ZSTD_Deleter {
	void operator() (ZSTD_CStream* cstream) {
		ZSTD_freeCStream(cstream);
	}

	void operator() (ZSTD_DStream* dstream) {
		ZSTD_freeDStream(dstream);
	}
};

#if defined(__MINGW32__) && !defined(__MINGW64__)
/*
 * This is exactly as dumb as it looks.
 * Yes, this is a memory leak. No, we don't have better solution right now.
 */
template<typename T> class leaky_ptr
{
	T *value;
public:
	leaky_ptr(T *value) : value(value) {};
	T *get() { return value; }
};
#endif

void compressZstd(const u8 *data, size_t data_size, std::ostream &os, int level)
{
#if defined(__MINGW32__) && !defined(__MINGW64__)
	// leaks one context per thread but doesn't crash :shrug:
	thread_local leaky_ptr<ZSTD_CStream> stream(ZSTD_createCStream());
#else
	// reusing the context is recommended for performance
	// it will destroyed when the thread ends
	thread_local std::unique_ptr<ZSTD_CStream, ZSTD_Deleter> stream(ZSTD_createCStream());
#endif


	ZSTD_initCStream(stream.get(), level);

	const size_t bufsize = 16384;
	char output_buffer[bufsize];

	ZSTD_inBuffer input = { data, data_size, 0 };
	ZSTD_outBuffer output = { output_buffer, bufsize, 0 };

	while (input.pos < input.size) {
		size_t ret = ZSTD_compressStream(stream.get(), &output, &input);
		if (ZSTD_isError(ret)) {
			dstream << ZSTD_getErrorName(ret) << std::endl;
			throw SerializationError("compressZstd: failed");
		}
		if (output.pos) {
			os.write(output_buffer, output.pos);
			output.pos = 0;
		}
	}

	size_t ret;
	do {
		ret = ZSTD_endStream(stream.get(), &output);
		if (ZSTD_isError(ret)) {
			dstream << ZSTD_getErrorName(ret) << std::endl;
			throw SerializationError("compressZstd: failed");
		}
		if (output.pos) {
			os.write(output_buffer, output.pos);
			output.pos = 0;
		}
	} while (ret != 0);

}

void compressZstd(const std::string &data, std::ostream &os, int level)
{
	compressZstd((u8*)data.c_str(), data.size(), os, level);
}

void decompressZstd(std::istream &is, std::ostream &os)
{
#if defined(__MINGW32__) && !defined(__MINGW64__)
	// leaks one context per thread but doesn't crash :shrug:
	thread_local leaky_ptr<ZSTD_DStream> stream(ZSTD_createDStream());
#else
	// reusing the context is recommended for performance
	// it will destroyed when the thread ends
	thread_local std::unique_ptr<ZSTD_DStream, ZSTD_Deleter> stream(ZSTD_createDStream());
#endif

	ZSTD_initDStream(stream.get());

	const size_t bufsize = 16384;
	char output_buffer[bufsize];
	char input_buffer[bufsize];

	ZSTD_outBuffer output = { output_buffer, bufsize, 0 };
	ZSTD_inBuffer input = { input_buffer, 0, 0 };
	size_t ret;
	do
	{
		if (input.size == input.pos) {
			is.read(input_buffer, bufsize);
			input.size = is.gcount();
			input.pos = 0;
		}

		ret = ZSTD_decompressStream(stream.get(), &output, &input);
		if (ZSTD_isError(ret)) {
			dstream << ZSTD_getErrorName(ret) << std::endl;
			throw SerializationError("decompressZstd: failed");
		}
		if (output.pos) {
			os.write(output_buffer, output.pos);
			output.pos = 0;
		}
	} while (ret != 0);

	// Unget all the data that ZSTD_decompressStream didn't take
	is.clear(); // Just in case EOF is set
	for (u32 i = 0; i < input.size - input.pos; i++) {
		is.unget();
		if (is.fail() || is.bad())
			throw SerializationError("decompressZstd: unget failed");
	}
}

void compress(u8 *data, u32 size, std::ostream &os, u8 version, int level)
{
	if(version >= 29)
	{
		// map the zlib levels [0,9] to [1,10]. -1 becomes 0 which indicates the default (currently 3)
		compressZstd(data, size, os, level + 1);
		return;
	}

	if(version >= 11)
	{
		compressZlib(data, size, os, level);
		return;
	}

	if(size == 0)
		return;

	// Write length (u32)

	u8 tmp[4];
	writeU32(tmp, size);
	os.write((char*)tmp, 4);

	// We will be writing 8-bit pairs of more_count and byte
	u8 more_count = 0;
	u8 current_byte = data[0];
	for(u32 i=1; i<size; i++)
	{
		if(
			data[i] != current_byte
			|| more_count == 255
		)
		{
			// write count and byte
			os.write((char*)&more_count, 1);
			os.write((char*)&current_byte, 1);
			more_count = 0;
			current_byte = data[i];
		}
		else
		{
			more_count++;
		}
	}
	// write count and byte
	os.write((char*)&more_count, 1);
	os.write((char*)&current_byte, 1);
}

void compress(const SharedBuffer<u8> &data, std::ostream &os, u8 version, int level)
{
	compress(*data, data.getSize(), os, version, level);
}

void compress(const std::string &data, std::ostream &os, u8 version, int level)
{
	compress((u8*)data.c_str(), data.size(), os, version, level);
}

void decompress(std::istream &is, std::ostream &os, u8 version)
{
	if(version >= 29)
	{
		decompressZstd(is, os);
		return;
	}

	if(version >= 11)
	{
		decompressZlib(is, os);
		return;
	}

	// Read length (u32)

	u8 tmp[4];
	is.read((char*)tmp, 4);
	u32 len = readU32(tmp);

	// We will be reading 8-bit pairs of more_count and byte
	u32 count = 0;
	for(;;)
	{
		u8 more_count=0;
		u8 byte=0;

		is.read((char*)&more_count, 1);

		is.read((char*)&byte, 1);

		if(is.eof())
			throw SerializationError("decompress: stream ended halfway");

		for(s32 i=0; i<(u16)more_count+1; i++)
			os.write((char*)&byte, 1);

		count += (u16)more_count+1;

		if(count == len)
			break;
	}
}


