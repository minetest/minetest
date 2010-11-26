#ifndef LOADSTATUS_HEADER
#define LOADSTATUS_HEADER

class LoadStatus
{
	bool ready;
	JMutex ready_mutex;
	
	u32 done;
	JMutex done_mutex;

	u32 todo;
	JMutex todo_mutex;

	wchar_t *text;
	JMutex text_mutex;

public:

	LoadStatus(bool a_ready=false, u32 a_done=0, u32 a_todo=0)
	{
		ready = a_ready;
		done = a_done;
		todo = a_todo;
		text = NULL;
		ready_mutex.Init();
		done_mutex.Init();
		todo_mutex.Init();
		text_mutex.Init();
	}

	void setReady(bool a_ready)
	{
		ready_mutex.Lock();
		ready = a_ready;
		ready_mutex.Unlock();
	}

	bool getReady(void)
	{
		ready_mutex.Lock();
		bool a_ready = ready;
		ready_mutex.Unlock();
		return a_ready;
	}

	void setDone(u32 a_done)
	{
		done_mutex.Lock();
		done = a_done;
		done_mutex.Unlock();
	}

	u32 getDone(void)
	{
		done_mutex.Lock();
		u32 a_done = done;
		done_mutex.Unlock();
		return a_done;
	}

	void setTodo(u32 a_todo)
	{
		todo_mutex.Lock();
		todo = a_todo;
		todo_mutex.Unlock();
	}

	u32 getTodo(void)
	{
		todo_mutex.Lock();
		u32 a_todo = todo;
		todo_mutex.Unlock();
		return a_todo;
	}

	/*
		Copies the text if not NULL,
		If NULL; sets text to NULL.
	*/
	void setText(const wchar_t *a_text)
	{
		text_mutex.Lock();
		if(text != NULL)
			free(text);
		if(a_text == NULL){
			text = NULL;
			text_mutex.Unlock();
			return;
		}
		u32 len = wcslen(a_text);
		text = (wchar_t*)malloc(sizeof(wchar_t) * (len+1));
		if(text == NULL) throw;
		swprintf(text, len+1, L"%ls", a_text);
		text_mutex.Unlock();
	}
	
	/*
		Return value must be free'd
		Return value can be NULL
	*/
	wchar_t * getText()
	{
		text_mutex.Lock();
		if(text == NULL){
			text_mutex.Unlock();
			return NULL;
		}
		u32 len = wcslen(text);
		wchar_t *b_text = (wchar_t*)malloc(sizeof(wchar_t) * (len+1));
		if(b_text == NULL) throw;
		swprintf(b_text, len+1, L"%ls", text);
		text_mutex.Unlock();
		return b_text;
	}
	
	/*
		Return value must be free'd
	*/
	wchar_t * getNiceText()
	{
		const wchar_t *defaulttext = L"Loading";
		wchar_t *t = getText();
		u32 maxlen = 20; // " (%i/%i)"
		if(t != NULL)
			maxlen += wcslen(t);
		else
			maxlen += wcslen(defaulttext);
		wchar_t *b_text = (wchar_t*)malloc(sizeof(wchar_t) * (maxlen+1));
		if(b_text == NULL) throw;
		if(t != NULL)
			swprintf(b_text, maxlen+1, L"%ls (%i/%i)",
					t, getDone(), getTodo());
		else
			swprintf(b_text, maxlen+1, L"%ls (%i/%i)",
					defaulttext, getDone(), getTodo());
		if(t != NULL)
			free(t);
		return b_text;
	}
};

#endif

