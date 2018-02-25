#include <memory>
#include <string>

#include <config.h>
#include <sound_espeakng.h>
#include <sound_tts_manager.h>

std::shared_ptr<MtTtsManager> g_tts;

MtTtsManager::MtTtsManager() :
	m_espeak()
{
#ifdef USE_ESPEAKNG
	m_espeak = std::unique_ptr<MtESpeak>(new MtESpeak());
	m_espeak->start();
#endif
}

void MtTtsManager::requestEnqueueExit()
{
	MtESpeakRequest req;
	req.m_type = MT_ESPEAK_REQUEST_TYPE_EXIT;
	if (m_espeak)
		m_espeak->requestEnqueue(std::move(req));
}

void MtTtsManager::requestEnqueueText(const std::string &text)
{
	MtESpeakRequest req;
	req.m_type = MT_ESPEAK_REQUEST_TYPE_TEXT;
	req.m_text = text;
	if (m_espeak)
		m_espeak->requestEnqueue(std::move(req));
}

std::shared_ptr<MtTtsManager> createTtsManagerGlobal()
{
	return std::shared_ptr<MtTtsManager>(new MtTtsManager());
}
