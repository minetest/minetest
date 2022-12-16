#include "csm_message.h"
#include "exceptions.h"
#include <utility>

static bool read_complete(FILE *f, size_t size, void *buf)
{
	size_t r;
	while ((r = fread(buf, 1, size, f)) < size) {
		if (r == 0 || feof(f) || ferror(f))
			return false;
		size -= r;
		buf = (u8 *)buf + r;
	}
	return true;
}

static bool write_complete(FILE *f, size_t size, const void *buf)
{
	size_t w;
	while ((w = fwrite(buf, 1, size, f)) < size) {
		if (w == 0 || ferror(f))
			return false;
		size -= w;
		buf = (u8 *)buf + w;
	}
	return true;
}

bool csm_send_msg(FILE *f, CSMMsgType type, size_t size, const void *data)
{
	if (!write_complete(f, sizeof(type), &type))
		return false;
	if (!write_complete(f, sizeof(size), &size))
		return false;
	if (data && !write_complete(f, size, data))
		return false;
	fflush(f);
	return true;
}

Optional<CSMRecvMsg> csm_recv_msg(FILE *f)
{
	CSMRecvMsg msg;
	if (!read_complete(f, sizeof(msg.type), &msg.type))
		return nullopt;
	size_t size;
	if (!read_complete(f, sizeof(size), &size))
		return nullopt;
	msg.data.resize(size);
	if (!read_complete(f, size, msg.data.data()))
		return nullopt;
	return Optional<CSMRecvMsg>(std::move(msg));
}

void csm_send_msg_ex(FILE *f, CSMMsgType type, size_t size, const void *data)
{
	if (!csm_send_msg(f, type, size, data))
		throw BaseException("Unable to send CSM message");
}

CSMRecvMsg csm_recv_msg_ex(FILE *f)
{
	Optional<CSMRecvMsg> msg = csm_recv_msg(f);
	if (!msg)
		throw BaseException("Unable to read CSM message");
	return std::move(*msg);
}
