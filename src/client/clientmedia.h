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

#pragma once

#include "irrlichttypes.h"
#include "util/basic_macros.h"
#include <ctime>
#include <ostream>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <unordered_map>

class Client;
struct HTTPFetchResult;
class MediaCacheDatabaseSQLite3;
struct MediaCacheUpdateFileArg;

#define MTHASHSET_FILE_SIGNATURE 0x4d544853 // 'MTHS'
#define MTHASHSET_FILE_NAME "index.mth"

// cache-classes
// each has a setting to define its longevity
enum class MediaCacheClass : u16 {
	Unknown = 0, // used if the server used a value that we don't know
	Legacy = 1, // used for old(=current) version when the server didn't specify a value
	Max = 2,
};

// how the media is sent over network or stored in cache
enum class MediaDataFormat : u16 {
	Raw = 0, // uncompressed
	LibZ = 1, // libz-compressed
	Max = 2,
};

// Store file into media cache (unless it exists already)
// Validating the hash is responsibility of the caller
void clientMediaUpdateCache(const std::string &raw_hash,
	const std::string &filedata);

// more of a base class than an interface but this name was most convenient...
class IClientMediaDownloader
{
public:
	DISABLE_CLASS_COPY(IClientMediaDownloader)

	virtual bool isStarted() const = 0;

	// If this returns true, the downloader is done and can be deleted
	virtual bool isDone() const = 0;

	// Add a file to the list of required file (but don't fetch it yet)
	virtual void addFile(const std::string &name, const std::string &sha1) = 0;

	// Add a remote server to the list; ignored if not built with cURL
	virtual void addRemoteServer(const std::string &baseurl) = 0;

	// Steps the media downloader:
	// - May load media into client by calling client->loadMedia()
	// - May check media cache for files
	// - May add files to media cache
	// - May start remote transfers by calling httpfetch_async
	// - May check for completion of current remote transfers
	// - May start conventional transfers by calling client->request_media()
	// - May inform server that all media has been loaded
	//   by calling client->received_media()
	// After step has been called once, don't call addFile/addRemoteServer.
	virtual void step(Client *client) = 0;

	// Must be called for each file received through TOCLIENT_MEDIA
	// returns true if this file belongs to this downloader
	virtual bool conventionalTransferDone(const std::string &name,
			const std::string &data, Client *client) = 0;

protected:
	IClientMediaDownloader();
	virtual ~IClientMediaDownloader() = default;

	// Forwards the call to the appropriate Client method
	virtual bool loadMedia(Client *client, const std::string &data,
		const std::string &name) = 0;

	virtual void cacheUpdateFile(MediaCacheUpdateFileArg &&file) = 0;

	bool checkAndLoad(const std::string &name, const std::string &sha1,
			const std::string &data, bool is_from_cache, Client *client,
			MediaDataFormat = MediaDataFormat::Raw);

	// Media cache database
	std::unique_ptr<MediaCacheDatabaseSQLite3> m_media_cache;
	bool m_write_to_cache;
};

class ClientMediaDownloader : public IClientMediaDownloader
{
public:
	ClientMediaDownloader();
	~ClientMediaDownloader();

	float getProgress() const {
		if (m_uncached_count >= 1)
			return 1.0f * m_uncached_received_count /
				m_uncached_count;

		return 0.0f;
	}

	bool isStarted() const override {
		return m_initial_step_done;
	}

	bool isDone() const override {
		return m_initial_step_done &&
			m_uncached_received_count == m_uncached_count;
	}

	void addFile(const std::string &name, const std::string &sha1) override;

	void addRemoteServer(const std::string &baseurl) override;

	void step(Client *client) override;

	bool conventionalTransferDone(
			const std::string &name,
			const std::string &data,
			Client *client) override;

	// Writes new cache entries and removes very old ones
	// The ClientMediaDownloader becomes unusable
	void updateAndCleanCache(Client *client);

protected:
	bool loadMedia(Client *client, const std::string &data,
			const std::string &name) override;

	void cacheUpdateFile(MediaCacheUpdateFileArg &&file) override;

private:
	struct FileStatus {
		bool received;
		std::string sha1;
		s32 current_remote;
		std::vector<s32> available_remotes;
	};

	struct RemoteServerStatus {
		std::string baseurl;
		s32 active_count;
	};

	void initialStep(Client *client);
	void remoteHashSetReceived(const HTTPFetchResult &fetch_result);
	void remoteMediaReceived(const HTTPFetchResult &fetch_result,
			Client *client);
	s32 selectRemoteServer(FileStatus *filestatus);
	void startRemoteMediaTransfers();
	void startConventionalTransfers(Client *client);

	static void deSerializeHashSet(const std::string &data,
			std::set<std::string> &result);
	std::string serializeRequiredHashSet();

	std::vector<std::time_t> getCacheExpiryTimes();

	// Maps filename to file status
	std::map<std::string, FileStatus*> m_files;

	// Array of remote media servers
	std::vector<RemoteServerStatus*> m_remotes;

	// Files to fill into the cache
	std::vector<MediaCacheUpdateFileArg> m_files_to_update_in_cache;

	// Has an attempt been made to load media files from the file cache?
	// Have hash sets been requested from remote servers?
	bool m_initial_step_done = false;

	// Total number of media files to load
	s32 m_uncached_count = 0;

	// Number of media files that have been received
	s32 m_uncached_received_count = 0;

	// Status of remote transfers
	unsigned long m_httpfetch_caller;
	unsigned long m_httpfetch_next_id = 0;
	s32 m_httpfetch_active = 0;
	s32 m_httpfetch_active_limit = 0;
	s32 m_outstanding_hash_sets = 0;
	std::unordered_map<unsigned long, std::string> m_remote_file_transfers;

	// All files up to this name have either been received from a
	// remote server or failed on all remote servers, so those files
	// don't need to be looked at again
	// (use m_files.upper_bound(m_name_bound) to get an iterator)
	std::string m_name_bound = "";

};

// A media downloader that only downloads a single file.
// It does/doesn't do several things the normal downloader does:
// - won't fetch hash sets from remote servers
// - will mark loaded media as coming from file push
// - writing to file cache is optional
class SingleMediaDownloader : public IClientMediaDownloader
{
public:
	SingleMediaDownloader(bool write_to_cache);
	~SingleMediaDownloader();

	bool isStarted() const override {
		return m_stage > STAGE_INIT;
	}

	bool isDone() const override {
		return m_stage >= STAGE_DONE;
	}

	void addFile(const std::string &name, const std::string &sha1) override;

	void addRemoteServer(const std::string &baseurl) override;

	void step(Client *client) override;

	bool conventionalTransferDone(const std::string &name,
			const std::string &data, Client *client) override;

protected:
	bool loadMedia(Client *client, const std::string &data,
			const std::string &name) override;

	void cacheUpdateFile(MediaCacheUpdateFileArg &&file) override;

private:
	void initialStep(Client *client);
	void remoteMediaReceived(const HTTPFetchResult &fetch_result, Client *client);
	void startRemoteMediaTransfer();
	void startConventionalTransfer(Client *client);

	enum Stage {
		STAGE_INIT,
		STAGE_CACHE_CHECKED, // we have tried to load the file from cache
		STAGE_DONE
	};

	// Information about the one file we want to fetch
	std::string m_file_name;
	std::string m_file_sha1;
	s32 m_current_remote;

	// Array of remote media servers
	std::vector<std::string> m_remotes;

	enum Stage m_stage = STAGE_INIT;

	// Status of remote transfers
	unsigned long m_httpfetch_caller;
	unsigned long m_httpfetch_next_id = 0;

};
