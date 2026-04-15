# 显示各种情况下的 SetParent 调用

## 1. 演示跨进程 SetParent

parent_app.c: 父窗口进程
child_app.c:  子窗口进程

### 运行

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

## 2. 演示将一个主窗口变为第二个窗口的子窗口

```
wine single_app.exe
```

## 3. 演示将一个子窗口，变成一个主窗口

```
wine child_to_main.exe
```
