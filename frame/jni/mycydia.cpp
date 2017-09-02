#include <stdio.h>
#include "substrate.h"
#include <android/log.h>
#include <unistd.h>
#include "mycydia.h"
#include <jni.h>


char target_soname[512];
char hook_sopath[512];
char G_packageName[200];
JNIEnv* G_env = NULL;
PTRMSHookFunction _MSHookFunction = NULL;
PTRdlopen olddlopen = NULL;

MSImageRef(*_MSGetImageByName)(const char *file) = NULL;
void *(*_MSFindSymbol)(MSImageRef image, const char *name) = NULL;
void(*_MSJavaHookMethod)(JNIEnv *jni, jclass _class, jmethodID methodID, void *function, void **result) = NULL;
void(*_MSJavaHookClassLoad)(JNIEnv *jni, const char *name, void(*callback)(JNIEnv *, jclass, void *), void *data) = NULL;	
void* (*olddlsym)(void* handle, const char* symbol) = NULL;



//获取模块的基址
void* get_module_base(int pid, const char* module_name)	
{
	FILE *fp;
	long addr = 0;
	char *pch;
	char filename[32];
	char line[1024];
	if(pid < 0)
	{
		snprintf(filename, sizeof(filename), "/proc/self/maps");//获取当前进程的maps文件
	}else{
		snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);//获取其它进程下的maps文件
	}
	fp = fopen(filename, "r");
	if (fp != NULL)
	{
		while (fgets(line, sizeof(line), fp))
		{
			if (strstr(line, module_name))	//判断当前行是否包含模块名
			{
				pch = strtok(line, "-");	//分割字符串得到基址的字符串
				addr = strtoul(pch, NULL, 16);	//字符串转16进制
				if (addr == 0x8000)
					addr = 0;
				DUALLOGD("[+] get module name: %s  base address: %08x\n", module_name, addr);
				break;
			}
		}
		fclose(fp);
	}else
	{
		DUALLOGE("[-] get module base address failed: %s\n", module_name);
	}
	return (void *)addr;
}


/** 向 /sdcard/my_hookso.txt读取配置
 *  第一行: 要注入的包名
 *  第二行: 要挂钩(Hook)的so名字,这里加载应用默认的库不需要完整路径,如果该应用采用其它方式加载则需要完整路径
 *  第三行: 实现实际hook功能的so,这个so里面包含了和框架so约定的导出函数
 */
void loadConfig()
{	
	FILE *fp;
	fp = fopen("/sdcard/my_hookso.txt", "r");
	char line[256];

	if (fp != NULL)
	{
		fgets(line, sizeof(line), fp);		//把换行符也获取到了,因需要去掉换行符
		line[strlen(line) - 1] = 0;
		if(line != NULL)
		{
			strcpy(G_packageName, line);
		}else
		{
			DUALLOGE("[-] read file target package name configure failed!\n");
			return;
		}
		fgets(line, sizeof(line), fp);
		line[strlen(line) - 1] = 0;
		if(line != NULL)
		{
			strcpy(target_soname, line);
		}else
		{
			DUALLOGE("[-] read file target_soname configure failed!\n");
			return;
		}
		fgets(line, sizeof(line), fp);
		line[strlen(line) - 1] = 0;
		if(line != NULL)
		{
			strcpy(hook_sopath, line);
			
		}else
		{
			DUALLOGE("[-] read file hook_sopath set failed!\n");
			return;
		}
		fclose(fp);
		DUALLOGD("[+] read config target_soname = %s , hook_sopath = %s\n", target_soname, hook_sopath);
	}else
	{
		DUALLOGE("[-] open file %s failed!\n", "/sdcard/my_hookso.txt");
	}
	return;
}

//获取远程地址
void* get_remote_addr(int target_pid, const char* module_name, void* local_addr)
{
	void* local_handle, *remote_handle;
	local_handle = get_module_base(-1, module_name);		//得到模块在当前进程的基址
	remote_handle = get_module_base(target_pid, module_name);	//得到模块在远程进程的基址
	DUALLOGD("[+] get_module_base: local[%08x], remote[%08x]\n", local_handle, remote_handle);
	void* ret_addr = (void *)((uint32_t)local_addr + (uint32_t)remote_handle - (uint32_t)local_handle);	//根据地址=基址+偏移得到在远程空间的实际地址
	#if defined(__i386__)
		if (!strcmp(module_name, "/system/lib/libc.so"))
		{
			ret_addr += 2;
		}
	#endif
	return ret_addr;
}


//Hook目标程序的dlopen,替换为现在的执行
void* newdlopen(const char* filename, int flags)		//Hook后执行
{
	DUALLOGD("[+] [newdlopen] The dlopen name : %s\n", filename);
	void *handle = olddlopen(filename, flags);
	if(strstr(filename, target_soname) != NULL)								//判断是否是要Hook的目标so
	{//如果加载的so是目标so则加载我们的注入so,并获取到约定Hook函数的地址
		DUALLOGD("[+] found target so: %s[%p]\n", filename, handle);
		void *handle_injectSO = olddlopen(hook_sopath, RTLD_NOW);
		HOOK_FUN_PTR key_hook = (HOOK_FUN_PTR)olddlsym(handle_injectSO, "key_hook_bridge");
		if (key_hook != NULL)
		{
			key_hook(target_soname, handle, G_env, G_packageName, olddlopen, _MSHookFunction);
			DUALLOGD("[+] key_hook_bridge function success was invoked\n");
		}else
		{
			DUALLOGE("[-] get key_hook_bridge failed, please cheak so path!\n");
		}
	}
	return handle;
}

void* newdlsym(void* handle, const char* symbol)
{
	//DUALLOGD("[+][newdlysm] The handle [%08x] dlsym name : %s\n", handle, symbol);
	return olddlsym(handle, symbol);
}

bool hook_dlopen()
{
	/*****************Hookdlopen函数***************************/
	void  *dlopen_addr, *dlsym_addr;												// *dlclose_addr, *dlerror_addr,*mmap_addr,;暂时只用2个
	dlopen_addr = get_remote_addr(getpid(), "/system/bin/linker", (void *)dlopen);
	dlsym_addr = get_remote_addr(getpid(), "/system/bin/linker", (void *)dlsym);
	if(_MSHookFunction != NULL)
	{
		_MSHookFunction((void*)dlopen_addr, (void*)&newdlopen, (void**)&olddlopen);
		_MSHookFunction((void*)dlsym_addr, (void*)&newdlsym, (void**)&olddlsym);
		DUALLOGD("[+] Hook function[dlopen] success, original addr:%p new addr:%p old addr:%p\n", dlopen_addr, newdlopen, olddlopen);
		DUALLOGD("[+] Hook function[dlsym] success, original addr:%p new addr:%p old addr:%p\n", dlsym_addr, newdlsym, olddlsym);
		return true;		
	}
	return false;
}


__attribute__((__constructor__)) static void _MSInitialize()
{	
	void* handle_subsub = dlopen("/system/lib/libsubstrate.so", RTLD_NOW);
	void* handle_subdvm = dlopen("/system/lib/libsubstrate-dvm.so", RTLD_NOW);
	if(handle_subsub != NULL)
	{
		DUALLOGD("[+] open library successful :%p\n", handle_subsub);
		DUALLOGD("[+] ******* start dlsym methond *******\n");
		_MSGetImageByName = (MSImageRef(*)(const char *file))dlsym(handle_subsub, "MSGetImageByName");
		_MSFindSymbol = (void*(*)(MSImageRef image, const char *name))dlsym(handle_subsub, "MSFindSymbol");
		_MSHookFunction = (void(*)(void *symbol, void *replace, void **result))dlsym(handle_subsub, "MSHookFunction");
		_MSJavaHookClassLoad = (void(*)(JNIEnv *jni, const char *name, void(*callback)(JNIEnv *, jclass, void *), void *data))dlsym(handle_subdvm, "MSJavaHookClassLoad");
		_MSJavaHookMethod = (void(*)(JNIEnv *jni, jclass _class, jmethodID methodID, void *function, void **result)) dlsym(handle_subdvm, "MSJavaHookMethod");
		DUALLOGD("[+] ***end dlsym methond***\n");
	}else
	{
		DUALLOGE("[-] open library failed : %s\n", "/system/lib/libsubstrate.so");
	}
	DUALLOGD("[+] MSGetImageByName=%p, MSFindSymbol=%p, MSHookFunction=%p, MSJavaHookMethod=%p, MSJavaHookClassLoad=%p\n", _MSGetImageByName, _MSFindSymbol, _MSHookFunction, _MSJavaHookMethod, _MSJavaHookClassLoad);	
}


//注入后初始化并读取配置
extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv* env = NULL;
	jint result = -1;
	DUALLOGD("[+] [hookso] %s begin", __FUNCTION__);
	
	if(vm->GetEnv((void**)&env, JNI_VERSION_1_4) != JNI_OK)
	{
		DUALLOGE("[-] [injectso] utility GetEnv error\n");
		return -1;
	}
	G_env = env;
	//下面应读取相应的配置信息
	loadConfig();
	//当拦截的目标不为空时就应hook dlopen
	if(target_soname != NULL)
	{
		if(_MSHookFunction != NULL)
		{
			hook_dlopen();
		}else{
			DUALLOGE("[-] init MSHookFunction point failed\n");
		}
	}else{
		DUALLOGE("[-] not config target so\n");
	}
	DUALLOGD("[+] [hookso] %s end", __FUNCTION__);
	return JNI_VERSION_1_4;
}
