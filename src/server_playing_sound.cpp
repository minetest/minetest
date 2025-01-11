#include "server_playing_sound.h"
#include "servermap.h"
#include "server/serveractiveobject.h"
#include "server/serverinventorymgr.h"

v3f ServerPlayingSound::getPos(ServerEnvironment *env, bool *pos_exists) const
{
	if (pos_exists)
		*pos_exists = false;

	switch (type ){
	case SoundLocation::Local:
		return v3f(0,0,0);
	case SoundLocation::Position:
		if (pos_exists)
			*pos_exists = true;
		return pos;
	case SoundLocation::Object:
		{
			if (object == 0)
				return v3f(0,0,0);
			ServerActiveObject *sao = env->getActiveObject(object);
			if (!sao)
				return v3f(0,0,0);
			if (pos_exists)
				*pos_exists = true;
			return sao->getBasePosition();
		}
	}

	return v3f(0,0,0);
}
