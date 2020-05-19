#include "debug.h"
#include "constants.h"
#include "util/serialize.h"
#include "network/networkpacket.h"

class SerializeStream {
public:
	SerializeStream(std::istream *is) : m_is(is) {}
	SerializeStream(std::ostream *os) : m_os(os) {}
	SerializeStream(std::istream *is, std::ostream *os) :
		m_is(is), m_os(os) {}

	std::istream *getReader() const
	{ return m_is; }
	std::ostream *getWriter() const
	{ return m_os; }

	bool eof() const { return m_is->eof(); }

	std::string readAll()
	{
		std::streamsize n = m_is->tellg();
		std::string data;
		data.resize(n);
		m_is->read(&data[0], n);
		return data;
	}

	// Operator overloading

#define MAKE_STREAM_RW(T, N, S)  \
	SerializeStream &operator>>(T &val) \
	{                               \
		char buf[S] = {0};          \
		m_is->read(buf, S);         \
		val = read ## N((u8 *)buf); \
		if (m_is->eof())            \
			throw SerializationError("Read ## T: EOF reached"); \
		return *this;               \
	}                               \
	SerializeStream &operator<<(T val) \
	{                               \
		char buf[S];                \
		write ## N((u8 *)buf, val); \
		m_os->write(buf, S);        \
		return *this;               \
	}

MAKE_STREAM_RW(u8,    U8,       1);
MAKE_STREAM_RW(u16,   U16,      2);
MAKE_STREAM_RW(u32,   U32,      4);
MAKE_STREAM_RW(u64,   U64,      8);

MAKE_STREAM_RW(s8,    S8,       1);
MAKE_STREAM_RW(s16,   S16,      2);
MAKE_STREAM_RW(s32,   S32,      4);
MAKE_STREAM_RW(s64,   S64,      8);
#undef MAKE_STREAM_RW

	std::string readString16()
	{
		u16 len = 0;
		*this >> len;
		return readStringPlain(len);
	}

	std::string readString32()
	{
		u32 len = 0;
		*this >> len;
		return readStringPlain(len);
	}

	void writeStringPlain(const std::string &str)
	{
		*m_os << str;
	}

	void writeString16(const std::string &str)
	{
		if (str.size() > U16_MAX)
			throw SerializationError("String too long for writeString16");

		*this << (u16)str.size();
		writeStringPlain(str);
	}

	void writeString32(const std::string &str)
	{
		if (str.size() > U32_MAX)
			throw SerializationError("String too long for writeString32");

		*this << (u32)str.size();
		writeStringPlain(str);
	}

protected:
	void setStreams(std::istream *is, std::ostream *os)
	{
		m_is = is;
		m_os = os;
	}

	void readRaw(char *dst, size_t n)
	{
		m_is->read(dst, n);
		if (m_is->eof())
			throw SerializationError("Read Raw: EOF reached");
	}

	void writeRaw(char *src, size_t n)
	{
		m_os->write(src, n);
	}

private:
	std::string readStringPlain(size_t len)
	{
		std::string data;
		data.resize(len);
		m_is->read(&data[0], len);
		return data;
	}

	std::istream *m_is = nullptr;
	std::ostream *m_os = nullptr;
};

#include <sstream>
class NetworkPacket_ : public SerializeStream {
public:
	NetworkPacket_(u16 command = 0, u32 datasize = 0,
		session_t peer_id = PEER_ID_INEXISTENT) : SerializeStream(nullptr, nullptr)
	{
		m_data = new std::stringstream(
			std::ios_base::binary | std::ios_base::in | std::ios_base::out);

		setStreams((std::istream *)m_data, (std::ostream *)m_data);

		m_command = 0;
		m_peer_id = peer_id;
	}

	~NetworkPacket_()
	{
		delete m_data;
	}

	u16 getCommand() { return m_command; }

	void putRawPacket(u8 *data, u32 datasize, session_t peer_id)
	{
		assert(m_command == 0);

		writeRaw((char *)data, datasize);
		*this >> m_command;
		m_peer_id = peer_id;
	}

	void clear()
	{
		m_data->str("");
		m_command = 0;
		m_peer_id = 0;
	}

private:
	std::stringstream *m_data;

	u16 m_command = 0;
	session_t m_peer_id = 0;
};

void testit()
{
	std::string data;
	// Output stream
	{
		std::ostringstream os;
		writeU16(os, 0x1234);
		os << serializeString("FooBar");
		writeS64(os, -5328507);
		os << serializeLongString("Covfefe");

		SerializeStream ss(&os);
		ss << (u16)0x2060;
		ss << (s32)-54;
		ss.writeString16("testing");
		data = os.str();
		std::cout << "Written!" << std::endl;
	}

	// Input stream
	{
		std::istringstream is(data);

		SerializeStream ss(&is);
		std::cout << "Input stream" << std::endl;
		u16 num = 0;
		ss >> num;
		std::cout << num << std::endl; // 0x1234 = 4660
		std::cout << ss.readString16() << std::endl; // FooBar
		std::cout << readS64(*ss.getReader()) << std::endl; // -5328507
		std::cout << ss.readString32() << std::endl; // Covfefe

		std::cout << readU16(is) << std::endl;
		std::cout << readS32(is) << std::endl;
		std::cout << deSerializeString(is) << std::endl;
		try {
			ss >> num;
			FATAL_ERROR("Unreachable");
		} catch (SerializationError &e) {
			std::cout << "Exception caught!" << std::endl;
		}
	}

	// Network stream
	{
		NetworkPacket_ pkt;
		std::cout << "Network stream" << std::endl;
		pkt.putRawPacket((u8 *)data.c_str(), data.size(), PEER_ID_SERVER);

		std::cout << pkt.getCommand() << std::endl; // 0x1234 = 4660
		std::cout << pkt.readString16() << std::endl; // FooBar
		// etc
	}
}
