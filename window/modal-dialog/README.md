# Modal Dialog Sample

演示 Windows 程序中创建模态对话框的四种方法。

## 功能说明

本示例展示了四种常见的模态对话框实现方式，每种方法都有其适用场景：

### 1. MessageBox Modal
- 使用系统提供的 `MessageBox` API
- 最简单的模态对话框，适合简单的确认/提示消息
- 按钮可点击测试模态效果

### 2. DialogBox Modal
- 使用 `DialogBox` API 从资源脚本创建模态对话框
- 支持自定义对话框布局和控件
- 通过资源文件 `resource.rc` 定义对话框界面

### 3. DialogBoxParam (Pass Text)
- 使用 `DialogBoxParam` API 向模态对话框传递参数
- 演示了如何在 `WM_INITDIALOG` 中接收和显示传递的数据
- 将输入框中的文本传给对话框显示

### 4. EnableWindow Manual Modal
- 手动实现模态效果：禁用父窗口 (`EnableWindow(FALSE)`)
- 创建自定义子窗口并使用 `WS_EX_TOPMOST` 置顶
- 关闭时必须先重新启用父窗口 (`EnableWindow(TRUE)`)

## 编译

```bash
cd window/modal-dialog
make
```

生成的可执行文件：`ModalDialog.exe`

## 运行

```bash
# 普通 Wine
wine ModalDialog.exe

# Wine Wayland
DISPLAY= WAYLAND_DISPLAY=wayland-1 WINEFSYNC=1 wine ModalDialog.exe
```

## 使用方法

1. 在输入框中输入测试文本
2. 点击四个按钮分别测试不同的模态对话框创建方式
3. 观察每种方式下的模态行为和父窗口状态

## 技术要点

| 方法 | API | 适用场景 | 数据传递 |
|------|-----|---------|---------|
| MessageBox | `MessageBox` | 简单提示 | 无 |
| DialogBox | `DialogBox` | 标准对话框 | 无 |
| DialogBoxParam | `DialogBoxParam` | 需要传参的对话框 | `lParam` |
| Manual Modal | `EnableWindow` + `CreateWindow` | 完全自定义控制 | 任意方式 |

## 文件说明

- `main.c` - 主程序源码，实现四种模态对话框
- `resource.h` - 资源标识符定义
- `resource.rc` - 对话框资源脚本
- `Makefile` - MinGW 编译配置
