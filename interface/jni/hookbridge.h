#if !defined(hook_kernel)
	#include <stdio.h>
	#include <jni.h>
	#include <string.h>
	#include <dlfcn.h>
	#include <android/log.h>

	#define TAG "Hook"
	#define DUALLOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__);printf(__VA_ARGS__)
	#define DUALLOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__);printf(__VA_ARGS__)
	#define DUALLOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__);printf(__VA_ARGS__)
	#define DUALLOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__);printf(__VA_ARGS__)
#endif
typedef void* (*PTRdlopen)(const char* filename, int flags);
typedef void(*PTRMSHookFunction)(void *symbol, void *replace, void **result);
int hookU3D(void *handlelibMonoSo, PTRdlopen olddlopen);
int hookCocos2dLua(void *handlelibcocos2dlua, PTRdlopen olddlopen);
void saveFile(const char* path, const char *data, size_t data_len, const char* name);
int mono_image_open_from_data_with_name_mod(char *data, int data_len, int need_copy, void *status, int refonly, const char *name);
int checkDir(const char* dir);
void saveToFile(const char *path, const char* filename, const char *mode, const char *data, size_t data_len);
int luaL_loadbuffer_mod(void *L, const char *buff, size_t size, const char *name);