/*
Minetest
Copyright (C) 2022 DS
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
OpenAL support based on work by:
Copyright (C) 2011 Sebastian 'Bahamada' Rühl
Copyright (C) 2011 Cyriaque 'Cisoun' Skrapits <cysoun@gmail.com>
Copyright (C) 2011 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; ifnot, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include "log.h"
#include "porting.h"
#include "sound_openal.h"
#include "../sound.h"
#include "threading/thread.h"
#include "util/basic_macros.h"
#include "util/container.h"

#if defined(_WIN32)
	#include <al.h>
	#include <alc.h>
	//#include <alext.h>
#elif defined(__APPLE__)
	#define OPENAL_DEPRECATED
	#include <OpenAL/al.h>
	#include <OpenAL/alc.h>
	//#include <OpenAL/alext.h>
#else
	#include <AL/al.h>
	#include <AL/alc.h>
	#include <AL/alext.h>
#endif
#include <vorbis/vorbisfile.h>

#include <optional>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>


/*
 *
 * The coordinate space for sounds (sound-space):
 * ----------------------------------------------
 *
 * * The functions from ISoundManager (see sound.h) take spatial vectors in node-space.
 * * All other `v3f`s here are, if not told otherwise, in sound-space, which is
 *   defined as node-space mirrored along the x-axis.
 *   (This is needed because OpenAL uses a right-handed coordinate system.)
 * * Use `swap_handedness()` to convert between those two coordinate spaces.
 *
 *
 * How sounds are loaded:
 * ----------------------
 *
 * * Step 1:
 *   `loadSoundFile` or `loadSoundFile` is called. This adds an unopen sound with
 *   the given name to `m_sound_datas_unopen`.
 *   Unopen / lazy sounds (`ISoundDataUnopen`) are ogg-vorbis files that we did not yet
 *   start to decode. (Decoding an unopen sound does not fail under normal circumstances
 *   (because we check whether the file exists at least), if it does fail anyways,
 *   we should notify the user.)
 * * Step 2:
 *   `addSoundToGroup` is called, to add the name from step 1 to a group. If the
 *   group does not yet exist, a new one is created. A group can later be played.
 *   (The mapping is stored in `m_sound_groups`.)
 * * Step 3:
 *   `playSound` or `playSoundAt` is called.
 *   * Step 3.1:
 *     If the group with the name `spec.name` does not exist, and `spec.use_local_fallback`
 *     is true, a new group is created using the user's sound-pack.
 *   * Step 3.2:
 *     We choose one random sound name from the given group.
 *   * Step 3.3:
 *     We open the sound (see `openSingleSound`).
 *     If the sound is already open (in `m_sound_datas_open`), we take that one.
 *     Otherwise we open it by calling `ISoundDataUnopen::open`. We choose (by
 *     sound length), whether it's a single-buffer (`SoundDataOpenBuffer`) or
 *     streamed (`SoundDataOpenStream`) sound.
 *     Single-buffer sounds are always completely loaded. Streamed sounds can be
 *     partially loaded.
 *     The sound is erased from `m_sound_datas_unopen` and added to `m_sound_datas_open`.
 *     Open sounds are kept forever.
 *   * Step 3.4:
 *     We create the new `PlayingSound`. It has a `shared_ptr` to its open sound.
 *     If the open sound is streaming, the playing sound needs to be stepped using
 *     `PlayingSound::stepStream` for enqueuing buffers. For this purpose, the sound
 *     is added to `m_sounds_streaming` (as `weak_ptr`).
 *     If the sound is fading, it is added to `m_sounds_fading` for regular fade-stepping.
 *     The sound is also added to `m_sounds_playing`, so that one can access it
 *     via its sound handle.
 * * Step 4:
 *     Streaming sounds are updated. For details see [Streaming of sounds].
 * * Step 5:
 *     At deinitialization, we can just let the destructors do their work.
 *     Sound sources are deleted (and with this also stopped) by ~PlayingSound.
 *     Buffers can't be deleted while sound sources using them exist, because
 *     PlayingSound has a shared_ptr to its ISoundData.
 *
 *
 * Streaming of sounds:
 * --------------------
 *
 * In each "bigstep", all streamed sounds are stepStream()ed. This means a
 * sound can be stepped at any point in time in the bigstep's interval.
 *
 * In the worst case, a sound is stepped at the start of one bigstep and in the
 * end of the next bigstep. So between two stepStream()-calls lie at most
 * 2 * STREAM_BIGSTEP_TIME seconds.
 * As there are always 2 sound buffers enqueued, at least one untouched full buffer
 * is still available after the first stepStream().
 * If we take a MIN_STREAM_BUFFER_LENGTH > 2 * STREAM_BIGSTEP_TIME, we can hence
 * not run into an empty queue.
 *
 * The MIN_STREAM_BUFFER_LENGTH needs to be a little bigger because of dtime jitter,
 * other sounds that may have taken long to stepStream(), and sounds being played
 * faster due to Doppler effect.
 *
 */

// constants

// in seconds
constexpr f32 REMOVE_DEAD_SOUNDS_INTERVAL = 2.0f;
// maximum length in seconds that a sound can have without being streamed
constexpr f32 SOUND_DURATION_MAX_SINGLE = 3.0f;
// minimum time in seconds of a single buffer in a streamed sound
constexpr f32 MIN_STREAM_BUFFER_LENGTH = 1.0f;
// duration in seconds of one bigstep
constexpr f32 STREAM_BIGSTEP_TIME = 0.3f;
// step duration for the ProxySoundManager, in seconds
constexpr f32 PROXYSOUNDMGR_DTIME = 0.016f;

static_assert(MIN_STREAM_BUFFER_LENGTH > STREAM_BIGSTEP_TIME * 2.0f,
		"See [Streaming of sounds].");
static_assert(SOUND_DURATION_MAX_SINGLE >= MIN_STREAM_BUFFER_LENGTH * 2.0f,
		"There's no benefit in streaming if we can't queue more than 2 buffers.");


/**
 * RAII wrapper for openal sound buffers.
 */
struct RAIIALSoundBuffer final
{
	RAIIALSoundBuffer() noexcept = default;
	explicit RAIIALSoundBuffer(ALuint buffer) noexcept : m_buffer(buffer) {};

	~RAIIALSoundBuffer() noexcept { reset(0); }

	DISABLE_CLASS_COPY(RAIIALSoundBuffer)

	RAIIALSoundBuffer(RAIIALSoundBuffer &&other) noexcept : m_buffer(other.release()) {}
	RAIIALSoundBuffer &operator=(RAIIALSoundBuffer &&other) noexcept;

	ALuint get() noexcept { return m_buffer; }

	ALuint release() noexcept { return std::exchange(m_buffer, 0); }

	void reset(ALuint buf) noexcept;

	static RAIIALSoundBuffer generate() noexcept;

private:
	// According to openal specification:
	// > Deleting buffer name 0 is a legal NOP.
	//
	// and:
	// > [...] the NULL buffer (i.e., 0) which can always be queued.
	ALuint m_buffer = 0;
};

/**
 * For vorbisfile to read from our buffer instead of from a file.
 */
struct OggVorbisBufferSource {
	std::string buf;
	size_t cur_offset = 0;

	static size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource) noexcept;
	static int seek_func(void *datasource, ogg_int64_t offset, int whence) noexcept;
	static int close_func(void *datasource) noexcept;
	static long tell_func(void *datasource) noexcept;

	static const ov_callbacks s_ov_callbacks;
};

/**
 * Metadata of an Ogg-Vorbis file, used for decoding.
 * We query this information once and store it in this struct.
 */
struct OggFileDecodeInfo {
	std::string name_for_logging;
	bool is_stereo;
	ALenum format; // AL_FORMAT_MONO16 or AL_FORMAT_STEREO16
	size_t bytes_per_sample;
	ALsizei freq;
	ALuint length_samples = 0;
	f32 length_seconds = 0.0f;
};

/**
 * RAII wrapper for OggVorbis_File.
 */
struct RAIIOggFile {
	bool m_needs_clear = false;
	OggVorbis_File m_file;

	RAIIOggFile() = default;

	DISABLE_CLASS_COPY(RAIIOggFile)

	~RAIIOggFile() noexcept
	{
		if (m_needs_clear)
			ov_clear(&m_file);
	}

	OggVorbis_File *get() { return &m_file; }

	std::optional<OggFileDecodeInfo> getDecodeInfo(const std::string &filename_for_logging);

	/**
	 * Main function for loading ogg vorbis sounds.
	 * Loads exactly the specified interval of PCM-data, and creates an OpenAL
	 * buffer with it.
	 *
	 * @param decode_info Cached meta information of the file.
	 * @param pcm_start First sample in the interval.
	 * @param pcm_end One after last sample of the interval (=> exclusive).
	 * @return An AL sound buffer, or a 0-buffer on failure.
	 */
	RAIIALSoundBuffer loadBuffer(const OggFileDecodeInfo &decode_info, ALuint pcm_start,
			ALuint pcm_end);
};


/**
 * Class for the openal device and context
 */
class SoundManagerSingleton
{
public:
	struct AlcDeviceDeleter {
		void operator()(ALCdevice *p)
		{
			alcCloseDevice(p);
		}
	};

	struct AlcContextDeleter {
		void operator()(ALCcontext *p)
		{
			alcMakeContextCurrent(nullptr);
			alcDestroyContext(p);
		}
	};

	using unique_ptr_alcdevice = std::unique_ptr<ALCdevice, AlcDeviceDeleter>;
	using unique_ptr_alccontext = std::unique_ptr<ALCcontext, AlcContextDeleter>;

	unique_ptr_alcdevice  m_device;
	unique_ptr_alccontext m_context;

public:
	bool init();

	~SoundManagerSingleton();
};


/**
 * Stores sound pcm data buffers.
 */
struct ISoundDataOpen
{
	OggFileDecodeInfo m_decode_info;

	explicit ISoundDataOpen(const OggFileDecodeInfo &decode_info) :
			m_decode_info(decode_info) {}

	virtual ~ISoundDataOpen() = default;

	/**
	 * Iff the data is streaming, there is more than one buffer.
	 * @return Whether it's streaming data.
	 */
	virtual bool isStreaming() const noexcept = 0;

	/**
	 * Load a buffer containing data starting at the given offset. Or just get it
	 * if it was already loaded.
	 *
	 * This function returns multiple values:
	 * * `buffer`: The OpenAL buffer.
	 * * `buffer_end`: The offset (in the file) where `buffer` ends (exclusive).
	 * * `offset_in_buffer`: Offset relative to `buffer`'s start where the requested
	 *       `offset` is.
	 *       `offset_in_buffer == 0` is guaranteed if some loaded buffer ends at
	 *       `offset`.
	 *
	 * @param offset The start of the buffer.
	 * @return `{buffer, buffer_end, offset_in_buffer}` or `{0, sound_data_end, 0}`
	 *         if `offset` is invalid.
	 */
	virtual std::tuple<ALuint, ALuint, ALuint> getOrLoadBufferAt(ALuint offset) = 0;

	static std::shared_ptr<ISoundDataOpen> fromOggFile(std::unique_ptr<RAIIOggFile> oggfile,
		const std::string &filename_for_logging);
};

/**
 * Will be opened lazily when first used.
 */
struct ISoundDataUnopen
{
	virtual ~ISoundDataUnopen() = default;

	// Note: The ISoundDataUnopen is moved (see &&). It is not meant to be kept
	// after opening.
	virtual std::shared_ptr<ISoundDataOpen> open(const std::string &sound_name) && = 0;
};

/**
 * Sound file is in a memory buffer.
 */
struct SoundDataUnopenBuffer final : ISoundDataUnopen
{
	std::string m_buffer;

	explicit SoundDataUnopenBuffer(std::string &&buffer) : m_buffer(std::move(buffer)) {}

	std::shared_ptr<ISoundDataOpen> open(const std::string &sound_name) && override;
};

/**
 * Sound file is in file system.
 */
struct SoundDataUnopenFile final : ISoundDataUnopen
{
	std::string m_path;

	explicit SoundDataUnopenFile(const std::string &path) : m_path(path) {}

	std::shared_ptr<ISoundDataOpen> open(const std::string &sound_name) && override;
};

/**
 * Non-streaming opened sound data.
 * All data is completely loaded in one buffer.
 */
struct SoundDataOpenBuffer final : ISoundDataOpen
{
	RAIIALSoundBuffer m_buffer;

	SoundDataOpenBuffer(std::unique_ptr<RAIIOggFile> oggfile,
			const OggFileDecodeInfo &decode_info);

	bool isStreaming() const noexcept override { return false; }

	std::tuple<ALuint, ALuint, ALuint> getOrLoadBufferAt(ALuint offset) override
	{
		if (offset >= m_decode_info.length_samples)
			return {0, m_decode_info.length_samples, 0};
		return {m_buffer.get(), m_decode_info.length_samples, offset};
	}
};

/**
 * Streaming opened sound data.
 *
 * Uses a sorted list of contiguous sound data regions (`ContiguousBuffers`s) for
 * efficient seeking.
 */
struct SoundDataOpenStream final : ISoundDataOpen
{
	/**
	 * An OpenAL buffer that goes until `m_end` (exclusive).
	 */
	struct SoundBufferUntil final
	{
		ALuint m_end;
		RAIIALSoundBuffer m_buffer;
	};

	/**
	 * A sorted non-empty vector of contiguous buffers.
	 * The start (inclusive) of each buffer is the end of its predecessor, or
	 * `m_start` for the first buffer.
	 */
	struct ContiguousBuffers final
	{
		ALuint m_start;
		std::vector<SoundBufferUntil> m_buffers;
	};

	std::unique_ptr<RAIIOggFile> m_oggfile;
	// A sorted vector of non-overlapping, non-contiguous `ContiguousBuffers`s.
	std::vector<ContiguousBuffers> m_bufferss;

	SoundDataOpenStream(std::unique_ptr<RAIIOggFile> oggfile,
			const OggFileDecodeInfo &decode_info);

	bool isStreaming() const noexcept override { return true; }

	std::tuple<ALuint, ALuint, ALuint> getOrLoadBufferAt(ALuint offset) override;

private:
	// offset must be before after_it's m_start and after (after_it-1)'s last m_end
	// new buffer will be inserted into m_bufferss before after_it
	// returns same as getOrLoadBufferAt
	std::tuple<ALuint, ALuint, ALuint> loadBufferAt(ALuint offset,
			std::vector<ContiguousBuffers>::iterator after_it);
};


/**
 * A sound that is currently played.
 * Can be streaming.
 * Can be fading.
 */
class PlayingSound final
{
	struct FadeState {
		f32 step;
		f32 target_gain;
	};

	ALuint m_source_id;
	std::shared_ptr<ISoundDataOpen> m_data;
	ALuint m_next_sample_pos = 0;
	bool m_looping;
	bool m_is_positional;
	bool m_stopped_means_dead = true;
	std::optional<FadeState> m_fade_state = std::nullopt;

public:
	PlayingSound(ALuint source_id, std::shared_ptr<ISoundDataOpen> data, bool loop,
			f32 volume, f32 pitch, f32 start_time,
			const std::optional<std::pair<v3f, v3f>> &pos_vel_opt);

	~PlayingSound() noexcept
	{
		alDeleteSources(1, &m_source_id);
	}

	DISABLE_CLASS_COPY(PlayingSound)

	// return false means streaming finished
	bool stepStream();

	// retruns true if it wasn't fading already
	bool fade(f32 step, f32 target_gain) noexcept;

	// returns true if more fade is needed later
	bool doFade(f32 dtime) noexcept;

	void updatePosVel(const v3f &pos, const v3f &vel) noexcept;

	void setGain(f32 gain) noexcept;

	f32 getGain() noexcept;

	void setPitch(f32 pitch) noexcept { alSourcef(m_source_id, AL_PITCH, pitch); }

	bool isStreaming() const noexcept { return m_data->isStreaming(); }

	void play() noexcept { alSourcePlay(m_source_id); }

	// returns one of AL_INITIAL, AL_PLAYING, AL_PAUSED, AL_STOPPED
	ALint getState() noexcept
	{
		ALint state;
		alGetSourcei(m_source_id, AL_SOURCE_STATE, &state);
		return state;
	}

	bool isDead() noexcept
	{
		// streaming sounds can (but should not) stop because the queue runs empty
		return m_stopped_means_dead && getState() == AL_STOPPED;
	}

	void pause() noexcept
	{
		// this is a NOP if state != AL_PLAYING
		alSourcePause(m_source_id);
	}

	void resume() noexcept
	{
		if (getState() == AL_PAUSED)
			play();
	}
};


/*
 * The SoundManager thread
 */

namespace sound_manager_messages_to_mgr {
	struct PauseAll {};
	struct ResumeAll {};

	struct UpdateListener { v3f pos_; v3f vel_; v3f at_; v3f up_; };
	struct SetListenerGain { f32 gain; };

	struct LoadSoundFile { std::string name; std::string filepath; };
	struct LoadSoundData { std::string name; std::string filedata; };
	struct AddSoundToGroup { std::string sound_name; std::string group_name; };

	struct PlaySound { sound_handle_t id; SoundSpec spec; };
	struct PlaySoundAt { sound_handle_t id; SoundSpec spec; v3f pos_; v3f vel_; };
	struct StopSound { sound_handle_t sound; };
	struct FadeSound { sound_handle_t soundid; f32 step; f32 target_gain; };
	struct UpdateSoundPosVel { sound_handle_t sound; v3f pos_; v3f vel_; };

	struct PleaseStop {};
}

using SoundManagerMsgToMgr = std::variant<
		std::monostate,

		sound_manager_messages_to_mgr::PauseAll,
		sound_manager_messages_to_mgr::ResumeAll,

		sound_manager_messages_to_mgr::UpdateListener,
		sound_manager_messages_to_mgr::SetListenerGain,

		sound_manager_messages_to_mgr::LoadSoundFile,
		sound_manager_messages_to_mgr::LoadSoundData,
		sound_manager_messages_to_mgr::AddSoundToGroup,

		sound_manager_messages_to_mgr::PlaySound,
		sound_manager_messages_to_mgr::PlaySoundAt,
		sound_manager_messages_to_mgr::StopSound,
		sound_manager_messages_to_mgr::FadeSound,
		sound_manager_messages_to_mgr::UpdateSoundPosVel,

		sound_manager_messages_to_mgr::PleaseStop
	>;

namespace sound_manager_messages_to_proxy {
	struct ReportRemovedSound { sound_handle_t id; };

	struct Stopped {};
}

using SoundManagerMsgToProxy = std::variant<
		std::monostate,

		sound_manager_messages_to_proxy::ReportRemovedSound,

		sound_manager_messages_to_proxy::Stopped
	>;

// not an ISoundManager. doesn't allocate ids, and doesn't accept id 0
class OpenALSoundManager final : public Thread
{
private:
	std::unique_ptr<SoundFallbackPathProvider> m_fallback_path_provider;

	ALCdevice *m_device;
	ALCcontext *m_context;

	// time in seconds until which removeDeadSounds will be called again
	f32 m_time_until_dead_removal = REMOVE_DEAD_SOUNDS_INTERVAL;

	// loaded sounds
	std::unordered_map<std::string, std::unique_ptr<ISoundDataUnopen>> m_sound_datas_unopen;
	std::unordered_map<std::string, std::shared_ptr<ISoundDataOpen>> m_sound_datas_open;
	// sound groups
	std::unordered_map<std::string, std::vector<std::string>> m_sound_groups;

	// currently playing sounds
	std::unordered_map<sound_handle_t, std::shared_ptr<PlayingSound>> m_sounds_playing;

	// streamed sounds
	std::vector<std::weak_ptr<PlayingSound>> m_sounds_streaming_current_bigstep;
	std::vector<std::weak_ptr<PlayingSound>> m_sounds_streaming_next_bigstep;
	// time left until current bigstep finishes
	f32 m_stream_timer = STREAM_BIGSTEP_TIME;

	std::vector<std::weak_ptr<PlayingSound>> m_sounds_fading;

	// if true, all sounds will be directly paused after creation
	bool m_is_paused = false;

public:
	// used for communication with ProxySoundManager
	MutexedQueue<SoundManagerMsgToMgr> m_queue_to_mgr;
	MutexedQueue<SoundManagerMsgToProxy> m_queue_to_proxy;

private:
	void stepStreams(f32 dtime);
	void doFades(f32 dtime);

	/**
	 * Gives the open sound for a loaded sound.
	 * Opens the sound if currently unopened.
	 *
	 * @param sound_name Name of the sound.
	 * @return The open sound.
	 */
	std::shared_ptr<ISoundDataOpen> openSingleSound(const std::string &sound_name);

	/**
	 * Gets a random sound name from a group.
	 *
	 * @param group_name The name of the sound group.
	 * @return The name of a sound in the group, or "" on failure. Getting the
	 *         sound with `openSingleSound` directly afterwards will not fail.
	 */
	std::string getLoadedSoundNameFromGroup(const std::string &group_name);

	/**
	 * Same as `getLoadedSoundNameFromGroup`, but if sound does not exist, try to
	 * load from local files.
	 */
	std::string getOrLoadLoadedSoundNameFromGroup(const std::string &group_name);

	std::shared_ptr<PlayingSound> createPlayingSound(const std::string &sound_name,
			bool loop, f32 volume, f32 pitch, f32 start_time,
			const std::optional<std::pair<v3f, v3f>> &pos_vel_opt);

	void playSoundGeneric(sound_handle_t id, const std::string &group_name, bool loop,
			f32 volume, f32 fade, f32 pitch, bool use_local_fallback, f32 start_time,
			const std::optional<std::pair<v3f, v3f>> &pos_vel_opt);

	/**
	 * Deletes sounds that are dead (=finished).
	 *
	 * @return Number of removed sounds.
	 */
	int removeDeadSounds();

public:
	OpenALSoundManager(SoundManagerSingleton *smg,
			std::unique_ptr<SoundFallbackPathProvider> fallback_path_provider);

	~OpenALSoundManager() override;

	DISABLE_CLASS_COPY(OpenALSoundManager)

private:
	/* Similar to ISoundManager */

	void step(f32 dtime);
	void pauseAll();
	void resumeAll();

	void updateListener(const v3f &pos_, const v3f &vel_, const v3f &at_, const v3f &up_);
	void setListenerGain(f32 gain);

	bool loadSoundFile(const std::string &name, const std::string &filepath);
	bool loadSoundData(const std::string &name, std::string &&filedata);
	void loadSoundFileNoCheck(const std::string &name, const std::string &filepath);
	void loadSoundDataNoCheck(const std::string &name, std::string &&filedata);
	void addSoundToGroup(const std::string &sound_name, const std::string &group_name);

	void playSound(sound_handle_t id, const SoundSpec &spec);
	void playSoundAt(sound_handle_t id, const SoundSpec &spec, const v3f &pos_,
			const v3f &vel_);
	void stopSound(sound_handle_t sound);
	void fadeSound(sound_handle_t soundid, f32 step, f32 target_gain);
	void updateSoundPosVel(sound_handle_t sound, const v3f &pos_, const v3f &vel_);

protected:
	/* Thread stuff */

	void *run() override;

private:
	void send(SoundManagerMsgToProxy msg)
	{
		m_queue_to_proxy.push_back(std::move(msg));
	}

	void reportRemovedSound(sound_handle_t id)
	{
		send(sound_manager_messages_to_proxy::ReportRemovedSound{id});
	}
};


/*
 * The public ISoundManager interface
 */

class ProxySoundManager final : public ISoundManager
{
	OpenALSoundManager m_sound_manager;
	// sound names from loadSoundData and loadSoundFile
	std::unordered_set<std::string> m_known_sound_names;

	void send(SoundManagerMsgToMgr msg)
	{
		m_sound_manager.m_queue_to_mgr.push_back(std::move(msg));
	}

	enum class MsgResult { Ok, Empty, Stopped};
	MsgResult handleMsg(SoundManagerMsgToProxy &&msg);

public:
	ProxySoundManager(SoundManagerSingleton *smg,
			std::unique_ptr<SoundFallbackPathProvider> fallback_path_provider) :
		m_sound_manager(smg, std::move(fallback_path_provider))
	{
		m_sound_manager.start();
	}

	~ProxySoundManager() override;

	/* Interface */

	void step(f32 dtime) override;
	void pauseAll() override;
	void resumeAll() override;

	void updateListener(const v3f &pos_, const v3f &vel_, const v3f &at_, const v3f &up_) override;
	void setListenerGain(f32 gain) override;

	bool loadSoundFile(const std::string &name, const std::string &filepath) override;
	bool loadSoundData(const std::string &name, std::string &&filedata) override;
	void addSoundToGroup(const std::string &sound_name, const std::string &group_name) override;

	void playSound(sound_handle_t id, const SoundSpec &spec) override;
	void playSoundAt(sound_handle_t id, const SoundSpec &spec, const v3f &pos_,
			const v3f &vel_) override;
	void stopSound(sound_handle_t sound) override;
	void fadeSound(sound_handle_t soundid, f32 step, f32 target_gain) override;
	void updateSoundPosVel(sound_handle_t sound, const v3f &pos_, const v3f &vel_) override;
};
