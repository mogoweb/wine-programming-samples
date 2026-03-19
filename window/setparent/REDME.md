# 演示跨进程 SetParent

parent_app.c: 父窗口进程
child_app.c:  子窗口进程

## 运行

首先在一个终端中父窗口启动：

```
wine parent_app.exe
```

记下终端中的输出，比如：

```
please run at another terminal:
./child_app.exe 0002004e
```

然后新开一个终端，在里面输入：

```
wine child_app.exe <hwnd>
```
