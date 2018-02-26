#ifndef _SOUND_TTS_MANAGER_H_
#define _SOUND_TTS_MANAGER_H_

#include <memory>
#include <string>

class MtESpeak;

enum MtTtsRequestType
{
	MT_TTS_REQUEST_TYPE_EXIT = 0,
	MT_TTS_REQUEST_TYPE_TEXT,
};

class MtTtsRequest
{
public:
	MtTtsRequestType m_type;

	int m_exit;
	std::string m_text;
};

class MtTtsManager
{
public:
	MtTtsManager();

	void requestEnqueueExit();
	void requestEnqueueText(const std::string &text);

private:
	std::unique_ptr<MtESpeak> m_espeak;
};

std::shared_ptr<MtTtsManager> createTtsManagerGlobal();

extern std::shared_ptr<MtTtsManager> g_tts;

#endif /* _SOUND_TTS_MANAGER_H_ */
