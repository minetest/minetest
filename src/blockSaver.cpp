#include "blockSaver.h"
#include "log.h"

void* BlockSaver::Thread() {
	ThreadStarted();
	log_register_thread("BlockSaverThread");

	DSTACK(__FUNCTION_NAME);
        
	BEGIN_DEBUG_EXCEPTION_HANDLER

		map.beginSave();
	for(;;) {
		if(rand()%0x1000==0) {
			map.endSave();
			map.beginSave();
		}

		{
			JMutexAutoLock al(lock);
			if(queue.size()==0) {
				if(getRun()==false) 
					break;
				sleep(1);
				continue;
			}
			MapBlock* block = queue.front();
			map.saveBlock(block);
			queued.erase(block->getPos());
			queue.pop_front();
		}
	}
	map.endSave();

	END_DEBUG_EXCEPTION_HANDLER(errorstream);
	return NULL;
		
}
