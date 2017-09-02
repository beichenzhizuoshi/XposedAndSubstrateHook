# XposedAndSubstrateHook
基于xposed和Substrate框架的通用hook
该仓库包含3个项目
XposedInjectSo:
    1. 需要安装Xposed框架
    2. 通过设置过滤的包名使应用启动被拦截,从而调用System.load函数来自动注入框架so到目标进程
    3. 通过配置包名和其它参数,想本机写入配置文件供框架层使用,因此改变框架层和Hook层也不需要软重启手机
Frame:
    1. 这是主要的实现框架层,其使用到了CydiaSubstrate的两个库
    2. 这里需要将libsubstrate.so, libsubstrate-dvm.so直接导入系统/system/lib目录下来脱离对Cydia的客户端要求
    3. 框架so层主要初始化了一些必要Hook函数的指针,并且Hook目标进程的dlopen函数来拦截目标so的加载
    4. 在拦截到目标so加载时调用libhookbridge.so中的导出函数key_hook_bridge
Interface:
    1. 主要Hook逻辑的实现, 每次更改此接口即可,而不必修改框架so和Xposed Java层代码
    2. 其中内置了U3D和Cocos2dLua的Hook 
    
注意: 
    1. 将框架so和接口so不要放在sd卡上,虽然采用System.load方式加载但还是报错,具体原因不明
    
