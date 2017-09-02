package com.beichen.xposeinjectso;

import android.util.Log;

import java.lang.reflect.Method;

import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XSharedPreferences;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.XposedHelpers;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

/**
 * Created by beichen on 8/31 0031.
 */

public class XposeInject implements IXposedHookLoadPackage {
   public boolean off = false;
    @Override
    public void handleLoadPackage(XC_LoadPackage.LoadPackageParam loadPackageParam) throws Throwable {
        if (off == false)
        {
            flushConfig();

        }
        if (!loadPackageParam.packageName.equals(GlobalConfig.Inject_PackageName))
            return;
        Class clazz = XposedHelpers.findClass(GlobalConfig.Inject_ClassName, loadPackageParam.classLoader); //使用注入包的类加载器来反射查找System.load函数,从而调用加载自己的框架so
        if(clazz == null)
        {
            XposedBridge.log("[-] not found class: " + GlobalConfig.Inject_ClassName);
            Log.e(GlobalConfig.Log_TAG, "[-] can't found class: " + GlobalConfig.Inject_ClassName);
            return;
        }
        Log.e(GlobalConfig.Log_TAG, "[+] found class: " + GlobalConfig.Inject_ClassName);
        Method method = XposedHelpers.findMethodExact(clazz, GlobalConfig.Inject_Invoke_FunName, String.class);
        if (method == null)
        {
            Log.e(GlobalConfig.Log_TAG, "[-] can't found function: " + method.getName());
        }
        Log.e(GlobalConfig.Log_TAG, "[+] found function: " + method.getName());
        method.invoke(null, GlobalConfig.Inject_SoPath);
    }

    public boolean flushConfig()
    {
        XSharedPreferences ms = new XSharedPreferences(GlobalConfig.Current_PackageName, GlobalConfig.Setting_FileName);

        GlobalConfig.Inject_PackageName = ms.getString(GlobalConfig.Setting_Key_inject_pckname, "");
        GlobalConfig.Inject_SoPath = ms.getString(GlobalConfig.Setting_Key_inject_soname, "");
        if(GlobalConfig.Inject_PackageName.equals("") || GlobalConfig.Inject_SoPath.equals("") )
        {
            XposedBridge.log("[-] not  package or so name is set");
            Log.e(GlobalConfig.Log_TAG,"[-] not set package:" + GlobalConfig.Inject_PackageName + " or so: " + GlobalConfig.Inject_SoPath );
            off = false;
            return false;
        }
        Log.e(GlobalConfig.Log_TAG,"[+] set package:" + GlobalConfig.Inject_PackageName + " and so: " + GlobalConfig.Inject_SoPath );
        off = true;
        return true;

    }
}
