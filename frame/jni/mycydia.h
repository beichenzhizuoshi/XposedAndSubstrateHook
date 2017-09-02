#if !defined (myhook)
	#define TAG "Hook"
	#define DUALLOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__);printf(__VA_ARGS__)
	#define DUALLOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__);printf(__VA_ARGS__)
	#define DUALLOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__);printf(__VA_ARGS__)
	#define DUALLOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__);printf(__VA_ARGS__)
	
	
	
	typedef void* (*PTRdlopen)(const char* filename, int flags);
	typedef void(*PTRMSHookFunction)(void *symbol, void *replace, void **result);
	typedef void (*HOOK_FUN_PTR)(const char* libname, void *handle, JNIEnv *env, const char* packageName, PTRdlopen olddlopen, PTRMSHookFunction mMSHookFunction);
	
#endif

void* get_module_base(int pid, const char* module_name);
void* get_remote_addr(int target_pid, const char* module_name, void* local_addr);
void* newdlopen(const char* filename, int flags);
void* newdlsym(void* handle, const char* symbol);
bool hook_dlopen();
void loadConfig();