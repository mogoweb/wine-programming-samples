# PageOffice ActiveX 内嵌示例

PageOffice 5.x 客户端控件在 IE / WebBrowser 中的内嵌演示，用于研究 Wine 下 ActiveX 控件（OLE 文档站点）的兼容性。

PageOffice 是国产商业中间件，通过 ActiveX 控件在 IE 页面内在线编辑/保存 Office 文档。本示例不含源码，只包含演示页面和客户端安装程序，用于在 Wine 中验证「第三方 ActiveX 控件 + MSHTML」的完整链路。

## 文件说明

| 文件 | 说明 |
|------|------|
| `test.html` | 演示页面：`<object>` 标签内嵌 PageOffice 控件 + 初始化脚本 |
| `posetup.exe` | PageOffice 5.x 客户端安装程序（32 位 PE，负责在系统中注册控件） |

## 页面工作原理

```
┌────────────────────────────────────────────┐
│           IE / WebBrowser (MSHTML)         │
│                                            │
│   test.html                                │
│   ├─ <meta X-UA-Compatible IE=8>           │
│   ├─ <script src="/pageoffice.js">         │
│   │    （由后端 poserver.zz 输出）           │
│   └─ <object id="PageOfficeCtrl1"          │
│        classid="CLSID:9E49D698-...">       │
│         │                                  │
│         ▼                                  │
│   PageOffice ActiveX 控件（Word 内嵌窗口）   │
└────────────────────────────────────────────┘
```

关键代码（test.html）：

```html
<!-- 强制 IE8 文档模式 -->
<meta http-equiv="X-UA-Compatible" content="IE=8">

<!-- 核心 ActiveX 标签，网页内联 Office 窗口 -->
<object id="PageOfficeCtrl1"
    classid="CLSID:9E49D698-C5C6-47BE-8C99-432B5E5D2A91"
    width="100%" height="850px">
    <param name="ServerPage" value="/poserver.zz" />
    <param name="SaveFilePage" value="/saveDoc" />
</object>
```

```javascript
// 控件初始化回调：添加自定义工具栏按钮
function OnPageOfficeCtrlInit() {
    var po = document.getElementById("PageOfficeCtrl1");
    po.AddCustomToolButton("保存文档", "Save()", 1);
}

// 保存回调：调用控件 WebSave() 方法
function Save() {
    document.getElementById("PageOfficeCtrl1").WebSave();
}
```

## 运行

### 1. 安装控件（Windows 或 Wine）

```bash
wine posetup.exe
```

安装程序会在系统（Wine prefix）中注册 PageOffice 控件并安装依赖的 Office 文档支持组件。

### 2. 打开页面

页面依赖 PageOffice 服务端环境（`/pageoffice.js` 由后端 `poserver.zz` 输出，文档保存提交到 `/saveDoc`），完整运行需配合服务端。本目录仅含客户端部分，可用以下方式打开：

```bash
# Wine 自带 IE
wine iexplore http://<server>/test.html
```

## Wine 测试要点

| 项目 | 测试目标 |
|------|----------|
| 安装程序 | posetup.exe 能否在 Wine prefix 中完成安装与控件注册 |
| ActiveX 实例化 | MSHTML 能否通过 CLSID 创建控件对象（CoCreateInstance 路径） |
| 控件窗口嵌入 | 就地激活、子窗口宿主、尺寸同步是否正常 |
| 脚本交互 | JS → 控件方法调用（AddCustomToolButton / WebSave）是否畅通 |
| 文档渲染/保存 | Word 内嵌窗口显示与保存流程是否完整 |

## 已知限制

- 无内置后端服务，单独打开 `test.html` 缺少 `pageoffice.js`，无法完整运行
- PageOffice 为商业软件，控件本身依赖较多系统组件，在 Wine 中可能因缺少 Office 组件而功能受限
