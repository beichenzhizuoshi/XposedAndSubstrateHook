package com.beichen.xposeinjectso;

/**
 * Created by beichen on 8/31 0031.
 */

public   class GlobalConfig {
    public static String Inject_PackageName = "";                                   //待注入的包名
    public static String Inject_SoPath = "";                                        //待注入的框架so的完整路径,此处不能在sd卡

    public static String Inject_ClassName = "java.lang.System";                   //查找我们要调用的System.load
    public static String Inject_Invoke_FunName = "load";

    public static String Setting_FileName = "setting";
    public static String Setting_Key_inject_pckname = "pckname";
    public static String Setting_Key_inject_soname = "inject_sopath";
    public static String Setting_Key_hook_soname = "hook_soname";
    public static String Setting_Key_hook_keysopath = "hook_keysopath";

    public static String Current_PackageName = "com.beichen.xposeinjectso";         //当前的包名,供Xposed读取配置用
    public static String Log_TAG = "TestInject";
    public static String Setting_SaveFileName = "my_hookso.txt";                    //保存的文件名,共框架so读取配置
}
