#include "hookbridge.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int G_HookState = 0;
int G_LogSegment = 0;
PTRMSHookFunction G_MSHookFunction = NULL;
char G_packageName[520] = {0};
const char *G_savePath = NULL;

extern "C" void key_hook_bridge(const char* libname, void *handle, JNIEnv *env, const char* packageName, PTRdlopen olddlopen, PTRMSHookFunction mMSHookFunction)
{
	DUALLOGD("[+] Hook kernel %s begin\n", __FUNCTION__);
	if(mMSHookFunction == NULL)
	{
		DUALLOGE("[-] MSHookFunction address is null\n");
		return;
	}
	G_MSHookFunction = mMSHookFunction;
	if(packageName == NULL || strlen(packageName) < 4)
	{
		DUALLOGE("[-] get package name  error\n");
		return;
	}
	strcpy(G_packageName, packageName);
	if(strstr(libname, "libmono.so") != NULL && G_HookState != 1)	//这里已经添加了对libmono.so的Hook
	{
		G_HookState = hookU3D(handle, olddlopen);
	}else if(strstr(libname, "libcocos2dlua.so") != NULL && G_HookState != 1)
	{
		G_HookState = hookCocos2dLua(handle, olddlopen);
	}else
	{
		DUALLOGE("[-] No %s name has been added\n", libname);
	}
	DUALLOGD("[+] Hook kernel %s executable end\n", __FUNCTION__);
}

/* 从内存镜像中写入到文件,通常获得了解密后的文件
 * 文件保存在默认的路径"/sdcard/myhook/"目录下
 * 创建对应的包名文件夹,然后再对应目录下写入文件,其总目录中包含日志
 * 这里的name是绝对路径如:/data/app/com.game.sgz.uc-1.apk/assets/bin/Data/Managed/UnityEngine.dll,所以需要提取文件名
 * 名字不能为空,避免非法内存访问,还有len度为0的情况都应考虑
 * 考虑到写入的文件没有记录完整的路径因此在目录下添加日志来记录
 */
void saveFile(const char *path, const char *data, size_t data_len,const char* name)
{
	char final_path[1024] = {0};
	char copyname[1024] = {0};
	strcpy(final_path, path);	//这里传入的路径没有包名,因此我们手动加上,已知包名类型为com.game.unity,需转化为com.game.unity/添加在路径中
	strcat(final_path, G_packageName);
	strcat(final_path, "/");
	strcpy(copyname, name);
	char *temp = NULL;	//切割字符串获得最终的文件名
	char *filename = NULL;
	char tempstr[20];
		
	if((int)data_len <= 0)		//之所以转换为int型是为了防止这个参数错误,在现在的情况下还没有单个脚本或库大于2GB
	{
		DUALLOGE("write file size error: %d\n", data_len);
		return;
	}
	/*************************************************/
	//写入日志到对应包名的根目录,以便后面分析使用
	char logtext[2048] = {0};
	if (0 == G_LogSegment)
	{
		strcat(logtext, "*****segmenet*****\n");
		G_LogSegment = 1;
	}
	strcat(logtext, "name: ");
	strcat(logtext, copyname);
	strcat(logtext, "\nlength: 0x");
	sprintf(tempstr, "%08x", data_len);		
	strcat(logtext, tempstr);
	strcat(logtext, " \n");
	saveToFile(final_path, "log.txt", "at+", logtext, strlen(logtext));
	/*****************************************/
	
	
	//过滤掉/data/app/包名.apk这样的前缀,通常在Unity游戏名字如此,而cocos2d则不会
	temp = strstr(copyname, ".apk/");
	if (temp != NULL)
	{
		temp = temp + 5;
	}else
	{
		temp = copyname;
	}
	
	//提取文件名创建相应内存
	filename = strtok(temp,"/");	
	strcat(final_path, filename);
	strcat(final_path, "/");
	while ((temp = strtok(NULL, "/")) != NULL)	//这里提取的路径里面最后包含了文件名,最终文件名保存在filename中
	{
		filename = temp;
		strcat(final_path, filename);
		strcat(final_path, "/");
	}
	//再过滤掉路径中的文件名,注意这里可能存在文件夹名字和文件名重复的问题,而我们要查找到匹配的最后一个
	temp = strstr(final_path, filename);
	*temp = 0;
	saveToFile(final_path, filename, "w+", data, data_len);
}
//注意这里不允许传空字符串而导致非法访问,需调用方确认
void saveToFile(const char *path, const char *filename, const char *mode, const char *data, size_t data_len)
{
	if(checkDir(path))
	{
		DUALLOGD("Dir %s not exit and create it failed, please check it!\n", path);
		return;
	}	
	char absolutPath[1024] = {0};
	strcpy(absolutPath, path);
	strcat(absolutPath, filename);
	FILE* fp = fopen(absolutPath, mode);
	if(fp == NULL)
	{
		DUALLOGE("[-] write file failed: %s\n", absolutPath);
		return;
	}
	fwrite(data, data_len, 1, fp);
	fclose(fp);
	DUALLOGD("[+] write file success: %s size: %d\n", absolutPath, data_len);
	return;
}

//检查文件夹是否存在，不存在则创建
int checkDir(const char *dir)									//这里传入的路径是多级路径如:"/sdcard/myhook/U3d/com.game.u3d/assets/bin/Data/Managed/"格式,因此需要循环判断是否存在
{		
	char absolutePath[1024] = {0};
	char checkpath[1024] = {0};
	char *temp;
	mode_t myMode = 777 ;
	
	strcpy(absolutePath, dir);
	temp = strtok(absolutePath, "/");
	strcat(checkpath, "/");
	strcat(checkpath, temp);
	strcat(checkpath, "/");										//这里路径为绝对路径,因此默认一级目录存在
	
	while ((temp = strtok(NULL, "/")) != NULL)
	{
		strcat(checkpath, temp);
		strcat(checkpath, "/");
		if(0 != access(checkpath,0)) 							//目录存在 00 只存在 02 写权限 04 读权限 06 读和写权限,有效返回0,无效返回1		
		{	
			if(0 != mkdir(checkpath,myMode)) {
				return 1;
			}	
		}
	}
	return 0;	
}



//hook mono_image_open_from_data_with_name
int (*mono_image_open_from_data_with_name_orig)(char *data, int data_len, int need_copy, void *status, int refonly, const char *name) = NULL;

int mono_image_open_from_data_with_name_mod(char *data, int data_len, int need_copy, void *status, int refonly, const char *name) {
    DUALLOGD("[dumpdll] mono_image_open_from_data_with_name, name: %s, len: %d, buff: %p\n", name, data_len, data);
    int ret = mono_image_open_from_data_with_name_orig(data, data_len, need_copy, status, refonly, name);   
	G_savePath = "/sdcard/myhook/U3D/";
	
	if(name == NULL)		//在这里判断字符串为空是为了避免传入空的指针而导致无效的内存引用(SIGSEGV 11 C)
	{
		char filename[16] = {0};
		sprintf(filename, "%d", data_len);	//没有名字使用长度做名字
		DUALLOGD("[+] modify null file name \n");		
		saveFile(G_savePath, data, (size_t)data_len, filename);
	}else
	{
		saveFile(G_savePath, data, (size_t)data_len, name);
	}	
    return ret;
}


int hookU3D(void *handlelibMonoSo, PTRdlopen olddlopen) {
    void *handle = NULL;
    if ( handlelibMonoSo==NULL ) {
        if ( olddlopen==NULL ) {
            handle = dlopen("libmono.so", RTLD_NOW);
        }else{
            handle = olddlopen("libmono.so", RTLD_NOW);
        }
        if (handle == NULL) {
            DUALLOGE("[dumpdll]dlopen err: %s.\n", dlerror());
            return 0;
        }
    }else{
        handle = handlelibMonoSo;
        DUALLOGD("[dumpdll] libmono.so handle: %p\n", handle);
    }
    void *mono_image_open_from_data_with_name = dlsym(handle, "mono_image_open_from_data_with_name");
    if (mono_image_open_from_data_with_name == NULL){
        DUALLOGE("[-] [dumpdll] mono_image_open_from_data_with_name not found! dlsym err: %s\n", dlerror());   
		return 0;		
    }
	DUALLOGD("[+] [dumpdll] mono_image_open_from_data_with_name found: %p\n", mono_image_open_from_data_with_name);
	G_MSHookFunction(mono_image_open_from_data_with_name, (void *)&mono_image_open_from_data_with_name_mod, (void **)&mono_image_open_from_data_with_name_orig);
   
    return 1;
}

int (*luaL_loadbuffer_orig)(void *L, const char *buff, size_t size, const char *name) = NULL;

int luaL_loadbuffer_mod(void *L, const char *buff, size_t size, const char *name)
{
	G_savePath = "/sdcard/myhook/Cocos2dLua/";
	if (name == NULL)	//为了以防万一没有名字导致后面内存非法访问
	{
		char filename[16] = {0};
		sprintf(filename, "%d", size);	//没有名字使用长度做名字
		DUALLOGD("[+] modify null file name \n");
		saveFile(G_savePath, buff, size, filename);		
	}else
	{	
		/* 在此处遇到了动态加载,name是一个完整的lua脚本(非期望的名字),导致传入参数后名字过长无法创建文件或文件夹而出现超出内存访问错误
		 * 因此我们需要手动处理判断文件名是否是想要的脚本文件,采用常见的(lua, luac等关键字,当然还要考虑到脚本中也存在这样的关键字),但此处我采用限定name的长度
		 * 如果name的长度大于200则判断不是我们要的文件,当然可以根据实际情况改写
		 */
			
		if (strlen(name) < 200)
		{
			DUALLOGD("[+] found lua file name: %s buff: %p\n", name, buff);	
			saveFile(G_savePath, buff, size, name);
		}else
		{
			//确认name不是文件名时我们总是希望保存下脚本来查看
			char dynamicname[50] = {0};
			char temp[16] = {0};
			strcpy(dynamicname, "dynamicsrcipt-");
			sprintf(temp, "%08x", strlen(name));
			strcat(dynamicname, temp);
			strcat(dynamicname, ".lua");			
			saveFile(G_savePath, name, strlen(name), dynamicname);
		}
		
	}

	return luaL_loadbuffer_orig(L, buff, size, name);
}

int hookCocos2dLua(void *handlelibcocos2dlua, PTRdlopen olddlopen)
{
	void *handle = NULL;
	if (handlelibcocos2dlua == NULL)
	{
		if (olddlopen == NULL)
		{
			handle = dlopen("libcocos2dlua.so", RTLD_NOW);	
		}else
		{
			handle = olddlopen("libcocos2dlua.so", RTLD_NOW);
		}
		if (handle == NULL)
		{
			DUALLOGE("[dumplua]dlopen err: %s.\n", dlerror());
            return 0;
		}
	}else
	{
		handle = handlelibcocos2dlua;
		DUALLOGD("[dumplua] libcocos2dlua.so handle: %p\n", handle);
	}
	void *luaL_loadbuffer = dlsym(handle, "luaL_loadbuffer");
	if (luaL_loadbuffer == NULL)
	{
		DUALLOGE("[-] [dumplua] luaL_loadbuffer not found! dlsym err: %s\n", dlerror());
		return 0;
	}
	G_MSHookFunction(luaL_loadbuffer, (void *)&luaL_loadbuffer_mod, (void **)&luaL_loadbuffer_orig);
	DUALLOGD("[+] [dumplua] luaL_loadbuffer found : %p\n", luaL_loadbuffer);
		
}
