package com.beichen.xposeinjectso;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

public class MainActivity extends Activity {
    private EditText et_inject_pckname, et_inject_sopath, et_hook_soname, et_hook_keysopath;
    private Button btn;
    private SharedPreferences mSharePreferences;
    private SharedPreferences.Editor editor;
    private TextView tv_help;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        et_inject_pckname = (EditText) findViewById(R.id.et_inject_pckname);
        et_inject_sopath = (EditText) findViewById(R.id.et_inject_soname);
        et_hook_soname = (EditText) findViewById(R.id.et_hook_soname);
        et_hook_keysopath = (EditText) findViewById(R.id.et_hook_key_sopath);
        tv_help = (TextView) findViewById(R.id.tv_help);
        btn = (Button) findViewById(R.id.btn);
        mSharePreferences = getSharedPreferences(GlobalConfig.Setting_FileName, MODE_WORLD_READABLE);
        editor = mSharePreferences.edit();
        init();
        btn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!et_inject_pckname.getText().toString().equals("") && !et_inject_sopath.getText().toString().equals("")) {
                    editor.putString(GlobalConfig.Setting_Key_inject_pckname, et_inject_pckname.getText().toString());
                    editor.putString(GlobalConfig.Setting_Key_inject_soname, et_inject_sopath.getText().toString());
                    editor.putString(GlobalConfig.Setting_Key_hook_soname, et_hook_soname.getText().toString());
                    editor.putString(GlobalConfig.Setting_Key_hook_keysopath, et_hook_keysopath.getText().toString());
                    editor.commit();
                    writeSaveFile();
                    Toast.makeText(MainActivity.this, "保存设置成功", Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(MainActivity.this, "请完整输入路径和包名", Toast.LENGTH_SHORT).show();
                }
            }
        });

        tv_help.setText("帮助:\n" +
                "1. 包名为待Hook应用的完整包名\n" +
                "2. 框架so的路径不能在sd卡,需要绝对路径\n" +
                "3. 要Hook的so的名字,如\"libmono.so\",如果该so不在应用的libs目录下则需要绝对路径\n" +
                "4. 我们实现具体Hook的绝对路径,该so中包含了约定的导出函数供框架so调用\n");
    }

    public void init() {
        et_inject_pckname.setText(mSharePreferences.getString(GlobalConfig.Setting_Key_inject_pckname, ""));
        et_inject_sopath.setText(mSharePreferences.getString(GlobalConfig.Setting_Key_inject_soname, ""));
        et_hook_soname.setText(mSharePreferences.getString(GlobalConfig.Setting_Key_hook_soname, ""));
        et_hook_keysopath.setText(mSharePreferences.getString(GlobalConfig.Setting_Key_hook_keysopath, ""));
    }

/** 向 /sdcard/my_hookso.txt写入配置供注入的框架so读取
 *  第一行: 要注入的包名
 *  第二行: 要挂钩(Hook)的so名字,这里加载应用默认的库不需要完整路径,如果该应用采用其它方式加载则需要完整路径
 *  第三行: 实现实际hook功能的so,这个so里面包含了和框架so约定的导出函数
 */
    public void writeSaveFile() {
        String sdStatus = Environment.getExternalStorageState();
        if (!sdStatus.equals(Environment.MEDIA_MOUNTED)) {
            Log.e(GlobalConfig.Log_TAG, "SD card is not avaiable/writeable right now.");
            Toast.makeText(MainActivity.this, "SD card is not avaiable/writeable right now", Toast.LENGTH_SHORT);
            return;
        }
        try {
            String absoultPath = Environment.getExternalStorageDirectory() + File.separator + GlobalConfig.Setting_SaveFileName;
            File file = new File(absoultPath);
            if (file.exists()) {
                file.delete();
            }
            Log.e(GlobalConfig.Log_TAG, "Create the file : " + absoultPath);
            file.createNewFile();
            FileOutputStream out = new FileOutputStream(file);
            String context = mSharePreferences.getString(GlobalConfig.Setting_Key_inject_pckname, "") + "\n";
            byte[] buf = context.getBytes();
            out.write(buf);
            context = mSharePreferences.getString(GlobalConfig.Setting_Key_hook_soname, "") + "\n";
            buf = context.getBytes();
            out.write(buf);
            context = mSharePreferences.getString(GlobalConfig.Setting_Key_hook_keysopath, "") + "\n";
            buf = context.getBytes();
            out.write(buf);
            out.close();
        } catch (IOException e) {
            e.printStackTrace();
            Log.e(GlobalConfig.Log_TAG, "write file failed!");
        }
    }
}