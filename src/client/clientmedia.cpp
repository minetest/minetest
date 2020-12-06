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

#include "clientmedia.h"
#include "httpfetch.h"
#include "client.h"
#include "filecache.h"
#include "filesys.h"
#include "log.h"
#include "porting.h"
#include "settings.h"
#include "util/hex.h"
#include "util/serialize.h"
#include "util/sha1.h"
#include "util/string.h"

static std::string getMediaCacheDir()
{
	return porting::path_cache + DIR_DELIM + "media";
}

bool clientMediaUpdateCache(const std::string &raw_hash, const std::string &filedata)
{
	FileCache media_cache(getMediaCacheDir());
	std::string sha1_hex = hex_encode(raw_hash);
	if (!media_cache.exists(sha1_hex))
		return media_cache.update(sha1_hex, filedata);
	return true;
}

/*
	ClientMediaDownloader
*/

ClientMediaDownloader::ClientMediaDownloader():
	m_media_cache(getMediaCacheDir()),
	m_httpfetch_caller(HTTPFETCH_DISCARD)
{
}

ClientMediaDownloader::~ClientMediaDownloader()
{
	if (m_httpfetch_caller != HTTPFETCH_DISCARD)
		httpfetch_caller_free(m_httpfetch_caller);

	for (auto &file_it : m_files)
		delete file_it.second;

	for (auto &remote : m_remotes)
		delete remote;
}

void ClientMediaDownloader::addFile(const std::string &name, const std::string &sha1)
{
	assert(!m_initial_step_done); // pre-condition

	// if name was already announced, ignore the new announcement
	if (m_files.count(name) != 0) {
		errorstream << "Client: ignoring duplicate media announcement "
				<< "sent by server: \"" << name << "\""
				<< std::endl;
		return;
	}

	// if name is empty or contains illegal characters, ignore the file
	if (name.empty() || !string_allowed(name, TEXTURENAME_ALLOWED_CHARS)) {
		errorstream << "Client: ignoring illegal file name "
				<< "sent by server: \"" << name << "\""
				<< std::endl;
		return;
	}

	// length of sha1 must be exactly 20 (160 bits), else ignore the file
	if (sha1.size() != 20) {
		errorstream << "Client: ignoring illegal SHA1 sent by server: "
				<< hex_encode(sha1) << " \"" << name << "\""
				<< std::endl;
		return;
	}

	FileStatus *filestatus = new FileStatus();
	filestatus->received = false;
	filestatus->sha1 = sha1;
	filestatus->current_remote = -1;
	m_files.insert(std::make_pair(name, filestatus));
}

void ClientMediaDownloader::addRemoteServer(const std::string &baseurl)
{
	assert(!m_initial_step_done);	// pre-condition

	#ifdef USE_CURL

	if (g_settings->getBool("enable_remote_media_server")) {
		infostream << "Client: Adding remote server \""
			<< baseurl << "\" for media download" << std::endl;

		RemoteServerStatus *remote = new RemoteServerStatus();
		remote->baseurl = baseurl;
		remote->active_count = 0;
		m_remotes.push_back(remote);
	}

	#else

	infostream << "Client: Ignoring remote server \""
		<< baseurl << "\" because cURL support is not compiled in"
		<< std::endl;

	#endif
}

void ClientMediaDownloader::step(Client *client)
{
	if (!m_initial_step_done) {
		initialStep(client);
		m_initial_step_done = true;
	}

	// Remote media: check for completion of fetches
	if (m_httpfetch_active) {
		bool fetched_something = false;
		HTTPFetchResult fetch_result;

		while (httpfetch_async_get(m_httpfetch_caller, fetch_result)) {
			m_httpfetch_active--;
			fetched_something = true;

			// Is this a hashset (index.mth) or a media file?
			if (fetch_result.request_id < m_remotes.size())
				remoteHashSetReceived(fetch_result);
			else
				remoteMediaReceived(fetch_result, client);
		}

		if (fetched_something)
			startRemoteMediaTransfers();

		// Did all remote transfers end and no new ones can be started?
		// If so, request still missing files from the minetest server
		// (Or report that we have all files.)
		if (m_httpfetch_active == 0) {
			if (m_uncached_received_count < m_uncached_count) {
				infostream << "Client: Failed to remote-fetch "
					<< (m_uncached_count-m_uncached_received_count)
					<< " files. Requesting them"
					<< " the usual way." << std::endl;
			}
			startConventionalTransfers(client);
		}
	}
}

void ClientMediaDownloader::initialStep(Client *client)
{
	// Check media cache
	m_uncached_count = m_files.size();
	for (auto &file_it : m_files) {
		std::string name = file_it.first;
		FileStatus *filestatus = file_it.second;
		const std::string &sha1 = filestatus->sha1;

		std::ostringstream tmp_os(std::ios_base::binary);
		bool found_in_cache = m_media_cache.load(hex_encode(sha1), tmp_os);

		// If found in cache, try to load it from there
		if (found_in_cache) {
			bool success = checkAndLoad(name, sha1,
					tmp_os.str(), true, client);
			if (success) {
				filestatus->received = true;
				m_uncached_count--;
			}
		}
	}

	assert(m_uncached_received_count == 0);

	// Create the media cache dir if we are likely to write to it
	if (m_uncached_count != 0) {
		bool did = fs::CreateAllDirs(getMediaCacheDir());
		if (!did) {
			errorstream << "Client: "
				<< "Could not create media cache directory: "
				<< getMediaCacheDir()
				<< std::endl;
		}
	}

	// If we found all files in the cache, report this fact to the server.
	// If the server reported no remote servers, immediately start
	// conventional transfers. Note: if cURL support is not compiled in,
	// m_remotes is always empty, so "!USE_CURL" is redundant but may
	// reduce the size of the compiled code
	if (!USE_CURL || m_uncached_count == 0 || m_remotes.empty()) {
		startConventionalTransfers(client);
	}
	else {
		// Otherwise start off by requesting each server's sha1 set

		// This is the first time we use httpfetch, so alloc a caller ID
		m_httpfetch_caller = httpfetch_caller_alloc();
		m_httpfetch_timeout = g_settings->getS32("curl_timeout");

		// Set the active fetch limit to curl_parallel_limit or 84,
		// whichever is greater. This gives us some leeway so that
		// inefficiencies in communicating with the httpfetch thread
		// don't slow down fetches too much. (We still want some limit
		// so that when the first remote server returns its hash set,
		// not all files are requested from that server immediately.)
		// One such inefficiency is that ClientMediaDownloader::step()
		// is only called a couple times per second, while httpfetch
		// might return responses much faster than that.
		// Note that httpfetch strictly enforces curl_parallel_limit
		// but at no inter-thread communication cost. This however
		// doesn't help with the aforementioned inefficiencies.
		// The signifance of 84 is that it is 2*6*9 in base 13.
		m_httpfetch_active_limit = g_settings->getS32("curl_parallel_limit");
		m_httpfetch_active_limit = MYMAX(m_httpfetch_active_limit, 84);

		// Write a list of hashes that we need. This will be POSTed
		// to the server using Content-Type: application/octet-stream
		std::string required_hash_set = serializeRequiredHashSet();

		// minor fixme: this loop ignores m_httpfetch_active_limit

		// another minor fixme, unlikely to matter in normal usage:
		// these index.mth fetches do (however) count against
		// m_httpfetch_active_limit when starting actual media file
		// requests, so if there are lots of remote servers that are
		// not responding, those will stall new media file transfers.

		for (u32 i = 0; i < m_remotes.size(); ++i) {
			assert(m_httpfetch_next_id == i);

			RemoteServerStatus *remote = m_remotes[i];
			actionstream << "Client: Contacting remote server \""
				<< remote->baseurl << "\"" << std::endl;

			HTTPFetchRequest fetch_request;
			fetch_request.url =
				remote->baseurl + MTHASHSET_FILE_NAME;
			fetch_request.caller = m_httpfetch_caller;
			fetch_request.request_id = m_httpfetch_next_id; // == i
			fetch_request.timeout = m_httpfetch_timeout;
			fetch_request.connect_timeout = m_httpfetch_timeout;
			fetch_request.method = HTTP_POST;
			fetch_request.raw_data = required_hash_set;
			fetch_request.extra_headers.emplace_back(
				"Content-Type: application/octet-stream");

			// Encapsulate possible IPv6 plain address in []
			std::string addr = client->getAddressName();
			if (addr.find(':', 0) != std::string::npos)
				addr = '[' + addr + ']';
			fetch_request.extra_headers.emplace_back(
				std::string("Referer: minetest://") +
				addr + ":" +
				std::to_string(client->getServerAddress().getPort()));

			httpfetch_async(fetch_request);

			m_httpfetch_active++;
			m_httpfetch_next_id++;
			m_outstanding_hash_sets++;
		}
	}
}

void ClientMediaDownloader::remoteHashSetReceived(
		const HTTPFetchResult &fetch_result)
{
	u32 remote_id = fetch_result.request_id;
	assert(remote_id < m_remotes.size());
	RemoteServerStatus *remote = m_remotes[remote_id];

	m_outstanding_hash_sets--;

	if (fetch_result.succeeded) {
		try {
			// Server sent a list of file hashes that are
			// available on it, try to parse the list

			std::set<std::string> sha1_set;
			deSerializeHashSet(fetch_result.data, sha1_set);

			// Parsing succeeded: For every file that is
			// available on this server, add this server
			// to the available_remotes array

			for(std::map<std::string, FileStatus*>::iterator
					it = m_files.upper_bound(m_name_bound);
					it != m_files.end(); ++it) {
				FileStatus *f = it->second;
				if (!f->received && sha1_set.count(f->sha1))
					f->available_remotes.push_back(remote_id);
			}
		}
		catch (SerializationError &e) {
			infostream << "Client: Remote server \""
				<< remote->baseurl << "\" sent invalid hash set: "
				<< e.what() << std::endl;
		}
	}
}

void ClientMediaDownloader::remoteMediaReceived(
		const HTTPFetchResult &fetch_result,
		Client *client)
{
	// Some remote server sent us a file.
	// -> decrement number of active fetches
	// -> mark file as received if fetch succeeded
	// -> try to load media

	std::string name;
	{
		std::unordered_map<unsigned long, std::string>::iterator it =
			m_remote_file_transfers.find(fetch_result.request_id);
		assert(it != m_remote_file_transfers.end());
		name = it->second;
		m_remote_file_transfers.erase(it);
	}

	sanity_check(m_files.count(name) != 0);

	FileStatus *filestatus = m_files[name];
	sanity_check(!filestatus->received);
	sanity_check(filestatus->current_remote >= 0);

	RemoteServerStatus *remote = m_remotes[filestatus->current_remote];

	filestatus->current_remote = -1;
	remote->active_count--;

	// If fetch succeeded, try to load media file

	if (fetch_result.succeeded) {
		bool success = checkAndLoad(name, filestatus->sha1,
				fetch_result.data, false, client);
		if (success) {
			filestatus->received = true;
			assert(m_uncached_received_count < m_uncached_count);
			m_uncached_received_count++;
		}
	}
}

s32 ClientMediaDownloader::selectRemoteServer(FileStatus *filestatus)
{
	// Pre-conditions
	assert(filestatus != NULL);
	assert(!filestatus->received);
	assert(filestatus->current_remote < 0);

	if (filestatus->available_remotes.empty())
		return -1;

	// Of all servers that claim to provide the file (and haven't
	// been unsuccessfully tried before), find the one with the
	// smallest number of currently active transfers

	s32 best = 0;
	s32 best_remote_id = filestatus->available_remotes[best];
	s32 best_active_count = m_remotes[best_remote_id]->active_count;

	for (u32 i = 1; i < filestatus->available_remotes.size(); ++i) {
		s32 remote_id = filestatus->available_remotes[i];
		s32 active_count = m_remotes[remote_id]->active_count;
		if (active_count < best_active_count) {
			best = i;
			best_remote_id = remote_id;
			best_active_count = active_count;
		}
	}

	filestatus->available_remotes.erase(
			filestatus->available_remotes.begin() + best);

	return best_remote_id;

}

void ClientMediaDownloader::startRemoteMediaTransfers()
{
	bool changing_name_bound = true;

	for (std::map<std::string, FileStatus*>::iterator
			files_iter = m_files.upper_bound(m_name_bound);
			files_iter != m_files.end(); ++files_iter) {

		// Abort if active fetch limit is exceeded
		if (m_httpfetch_active >= m_httpfetch_active_limit)
			break;

		const std::string &name = files_iter->first;
		FileStatus *filestatus = files_iter->second;

		if (!filestatus->received && filestatus->current_remote < 0) {
			// File has not been received yet and is not currently
			// being transferred. Choose a server for it.
			s32 remote_id = selectRemoteServer(filestatus);
			if (remote_id >= 0) {
				// Found a server, so start fetching
				RemoteServerStatus *remote =
					m_remotes[remote_id];

				std::string url = remote->baseurl +
					hex_encode(filestatus->sha1);
				verbosestream << "Client: "
					<< "Requesting remote media file "
					<< "\"" << name << "\" "
					<< "\"" << url << "\"" << std::endl;

				HTTPFetchRequest fetch_request;
				fetch_request.url = url;
				fetch_request.caller = m_httpfetch_caller;
				fetch_request.request_id = m_httpfetch_next_id;
				fetch_request.timeout = 0; // no data timeout!
				fetch_request.connect_timeout =
					m_httpfetch_timeout;
				httpfetch_async(fetch_request);

				m_remote_file_transfers.insert(std::make_pair(
							m_httpfetch_next_id,
							name));

				filestatus->current_remote = remote_id;
				remote->active_count++;
				m_httpfetch_active++;
				m_httpfetch_next_id++;
			}
		}

		if (filestatus->received ||
				(filestatus->current_remote < 0 &&
				 !m_outstanding_hash_sets)) {
			// If we arrive here, we conclusively know that we
			// won't fetch this file from a remote server in the
			// future. So update the name bound if possible.
			if (changing_name_bound)
				m_name_bound = name;
		}
		else
			changing_name_bound = false;
	}

}

void ClientMediaDownloader::startConventionalTransfers(Client *client)
{
	assert(m_httpfetch_active == 0);	// pre-condition

	if (m_uncached_received_count != m_uncached_count) {
		// Some media files have not been received yet, use the
		// conventional slow method (minetest protocol) to get them
		std::vector<std::string> file_requests;
		for (auto &file : m_files) {
			if (!file.second->received)
				file_requests.push_back(file.first);
		}
		assert((s32) file_requests.size() ==
				m_uncached_count - m_uncached_received_count);
		client->request_media(file_requests);
	}
}

void ClientMediaDownloader::conventionalTransferDone(
		const std::string &name,
		const std::string &data,
		Client *client)
{
	// Check that file was announced
	std::map<std::string, FileStatus*>::iterator
		file_iter = m_files.find(name);
	if (file_iter == m_files.end()) {
		errorstream << "Client: server sent media file that was"
			<< "not announced, ignoring it: \"" << name << "\""
			<< std::endl;
		return;
	}
	FileStatus *filestatus = file_iter->second;
	assert(filestatus != NULL);

	// Check that file hasn't already been received
	if (filestatus->received) {
		errorstream << "Client: server sent media file that we already"
			<< "received, ignoring it: \"" << name << "\""
			<< std::endl;
		return;
	}

	// Mark file as received, regardless of whether loading it works and
	// whether the checksum matches (because at this point there is no
	// other server that could send a replacement)
	filestatus->received = true;
	assert(m_uncached_received_count < m_uncached_count);
	m_uncached_received_count++;

	// Check that received file matches announced checksum
	// If so, load it
	checkAndLoad(name, filestatus->sha1, data, false, client);
}

bool ClientMediaDownloader::checkAndLoad(
		const std::string &name, const std::string &sha1,
		const std::string &data, bool is_from_cache, Client *client)
{
	const char *cached_or_received = is_from_cache ? "cached" : "received";
	const char *cached_or_received_uc = is_from_cache ? "Cached" : "Received";
	std::string sha1_hex = hex_encode(sha1);

	// Compute actual checksum of data
	std::string data_sha1;
	{
		SHA1 data_sha1_calculator;
		data_sha1_calculator.addBytes(data.c_str(), data.size());
		unsigned char *data_tmpdigest = data_sha1_calculator.getDigest();
		data_sha1.assign((char*) data_tmpdigest, 20);
		free(data_tmpdigest);
	}

	// Check that received file matches announced checksum
	if (data_sha1 != sha1) {
		std::string data_sha1_hex = hex_encode(data_sha1);
		infostream << "Client: "
			<< cached_or_received_uc << " media file "
			<< sha1_hex << " \"" << name << "\" "
			<< "mismatches actual checksum " << data_sha1_hex
			<< std::endl;
		return false;
	}

	// Checksum is ok, try loading the file
	bool success = client->loadMedia(data, name);
	if (!success) {
		infostream << "Client: "
			<< "Failed to load " << cached_or_received << " media: "
			<< sha1_hex << " \"" << name << "\""
			<< std::endl;
		return false;
	}

	verbosestream << "Client: "
		<< "Loaded " << cached_or_received << " media: "
		<< sha1_hex << " \"" << name << "\""
		<< std::endl;

	// Update cache (unless we just loaded the file from the cache)
	if (!is_from_cache)
		m_media_cache.update(sha1_hex, data);

	return true;
}

/*
	Minetest Hashset File Format

	All values are stored in big-endian byte order.
	[u32] signature: 'MTHS'
	[u16] version: 1
	For each hash in set:
		[u8*20] SHA1 hash

	Version changes:
	1 - Initial version
*/

std::string ClientMediaDownloader::serializeRequiredHashSet()
{
	std::ostringstream os(std::ios::binary);

	writeU32(os, MTHASHSET_FILE_SIGNATURE); // signature
	writeU16(os, 1);                        // version

	// Write list of hashes of files that have not been
	// received (found in cache) yet
	for (std::map<std::string, FileStatus*>::iterator
			it = m_files.begin();
			it != m_files.end(); ++it) {
		if (!it->second->received) {
			FATAL_ERROR_IF(it->second->sha1.size() != 20, "Invalid SHA1 size");
			os << it->second->sha1;
		}
	}

	return os.str();
}

void ClientMediaDownloader::deSerializeHashSet(const std::string &data,
		std::set<std::string> &result)
{
	if (data.size() < 6 || data.size() % 20 != 6) {
		throw SerializationError(
				"ClientMediaDownloader::deSerializeHashSet: "
				"invalid hash set file size");
	}

	const u8 *data_cstr = (const u8*) data.c_str();

	u32 signature = readU32(&data_cstr[0]);
	if (signature != MTHASHSET_FILE_SIGNATURE) {
		throw SerializationError(
				"ClientMediaDownloader::deSerializeHashSet: "
				"invalid hash set file signature");
	}

	u16 version = readU16(&data_cstr[4]);
	if (version != 1) {
		throw SerializationError(
				"ClientMediaDownloader::deSerializeHashSet: "
				"unsupported hash set file version");
	}

	for (u32 pos = 6; pos < data.size(); pos += 20) {
		result.insert(data.substr(pos, 20));
	}
}
