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
#include "filecache.h"
#include <ostream>
#include <map>
#include <set>
#include <vector>
#include <unordered_map>

class Client;
struct HTTPFetchResult;

#define MTHASHSET_FILE_SIGNATURE 0x4d544853 // 'MTHS'
#define MTHASHSET_FILE_NAME "index.mth"

class ClientMediaDownloader
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

	bool isStarted() const {
		return m_initial_step_done;
	}

	// If this returns true, the downloader is done and can be deleted
	bool isDone() const {
		return m_initial_step_done &&
			m_uncached_received_count == m_uncached_count;
	}

	// Add a file to the list of required file (but don't fetch it yet)
	void addFile(const std::string &name, const std::string &sha1);

	// Add a remote server to the list; ignored if not built with cURL
	void addRemoteServer(const std::string &baseurl);

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
	void step(Client *client);

	// Must be called for each file received through TOCLIENT_MEDIA
	void conventionalTransferDone(
			const std::string &name,
			const std::string &data,
			Client *client);

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

	bool checkAndLoad(const std::string &name, const std::string &sha1,
			const std::string &data, bool is_from_cache,
			Client *client);

	std::string serializeRequiredHashSet();
	static void deSerializeHashSet(const std::string &data,
			std::set<std::string> &result);

	// Maps filename to file status
	std::map<std::string, FileStatus*> m_files;

	// Array of remote media servers
	std::vector<RemoteServerStatus*> m_remotes;

	// Filesystem-based media cache
	FileCache m_media_cache;

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
	long m_httpfetch_timeout = 0;
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
