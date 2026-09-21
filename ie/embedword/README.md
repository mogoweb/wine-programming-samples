# DsoFramer 内嵌 Word 文档示例

DsoFramer（dsoframer.ocx，Framer Control 1.3）方案在 IE / WebBrowser 中内嵌 Word 文档的演示页面，用于研究 Wine/Wine Wayland 下「网页 ActiveX + 嵌套 OLE 文档容器」的兼容性。

DsoFramer 源自微软 KB311745 示例代码，本身是一个完整的 OLE 文档容器控件，可将 Word/Excel/PPT 以 OLE 就地激活方式嵌入窗口，是各类「网页内嵌 Office」方案（含商业化的 PageOffice，见 [../pageoffice/](../pageoffice/)）的鼻祖。

本目录只含演示页面，不含 ocx/cab 二进制，控件需自行获取。

## 文件说明

| 文件 | 说明 |
|------|------|
| `index.html` | 演示页面：`<object>` 标签内嵌控件 + 全套操作按钮 |

## 工作原理

与 PageOffice 直接在页面中放 Office 控件不同，DsoFramer 的嵌套层级更深——浏览器宿主 ActiveX 控件，控件内部再作为 OLE 容器宿主 Word：

```
IE / WebBrowser (MSHTML)
└─ DsoFramer 控件 (dsoframer.ocx, ActiveX)
   └─ 控件内部 OLE 文档容器
      ├─ IOleClientSite / IOleInPlaceSite / IOleInPlaceFrame
      └─ Word.Document (IOleObject, 就地激活)
         └─ Word 进程 / Office 组件
```

关键代码（index.html）：

```html
<!-- Framer Control 1.3 固定 CLSID；codebase 指向 cab 时 IE 可自动安装 -->
<object id="FramerControl1"
    classid="clsid:00460182-9E5E-11d5-B7C8-B8269041DD57"
    width="100%" height="640px"
    codebase="dsoframer.cab#version=1,3,0,1">
    <param name="TitleBar" value="0"/>
    <param name="Menubar" value="1"/>
    <param name="Toolbars" value="1"/>
</object>
```

```javascript
// 新建空白文档（无需任何文件，适合快速验证控件）
FramerControl1.CreateNew("Word.Document");

// 打开文档（远程 URL 或本地路径）
FramerControl1.Open("http://server/sample.doc");
FramerControl1.Open("C:\\test.doc");

// 保存：本地直接 Save，远程以 POST 方式提交到 HTTP 端点
FramerControl1.Save();
FramerControl1.SaveAs("http://server/saveDoc");

// 获取 Word 自动化对象，用完整对象模型操作文档
var doc = FramerControl1.GetDocumentObject();
doc.Application.Selection.TypeText("Hello DsoFramer");
```

## 控件 API 速查

| 方法 | 说明 |
|------|------|
| `CreateNew(progId)` | 新建文档，如 `"Word.Document"`、`"Excel.Sheet"` |
| `Open(url[, readOnly][, progId][, user][, pwd])` | 打开远程/本地文档 |
| `Save()` | 保存本地文档 |
| `SaveAs(url[, progId][, user][, pwd])` | 另存/上传到 HTTP 端点 |
| `Close()` | 关闭文档 |
| `ShowDialog(0~5)` | 系统对话框：0=打开 1=另存为 2=保存副本 3=打印 4=页面设置 5=属性 |
| `PrintOut([promptUser])` | 打印 |
| `PrintPreview()` / `PrintPreviewExit()` | 打印预览及退出 |
| `SetTrackRevisions(bool)` / `GetTrackRevisions()` | 修订开关 |
| `SetShowRevisions(bool)` / `GetShowRevisions()` | 修订显示 |
| `InsertFile(path[, progId])` | 插入另一文档内容 |
| `GetDocumentObject()` | 返回 Word.Document 自动化对象（IDispatch） |

## 运行

### Windows

1. 获取 `dsoframer.ocx`（微软 KB311745 原版 1.3 或社区修复版）
2. 注册控件（管理员）：`regsvr32 dsoframer.ocx`
3. 本机安装 Office（Word.Document ProgID 依赖真实 Word）
4. IE 打开 `index.html`（首次会提示「允许阻止的内容」）

### Wine

```bash
# 注册控件（需先将 dsoframer.ocx 放入 Wine prefix 可访问路径）
wine regsvr32 dsoframer.ocx

# 打开页面
wine iexplore index.html
# 或配合 ../call_external/qahost.exe 自建宿主打开
wine ../call_external/qahost.exe <index.html 的 URL>
```

注意：控件宿主的是真实 Office，同一 Wine prefix 内必须已安装 Office 才能嵌入 Word 文档。

## Wine/Wine Wayland 测试要点

| 项目 | 测试目标 |
|------|----------|
| OCX 注册 | regsvr32 自注册（DllRegisterServer）是否成功 |
| ActiveX 实例化 | MSHTML 能否通过 CLSID 创建控件对象 |
| 嵌套 OLE 容器 | 控件内部作为容器再次嵌入 Word（两层就地激活嵌套） |
| 菜单/工具栏合并 | IOleInPlaceFrame 菜单协商（Word 菜单融入宿主窗口） |
| 打印预览 | Word 预览窗口是否正常显示 |
| 自动化调用 | GetDocumentObject 返回的 IDispatch 调用是否畅通 |
| HTTP 打开/保存 | urlmon/WinINet 下载与 POST 上传 |

## 已知限制

- 仅 IE 内核（Trident）可用，现代浏览器均不支持
- 依赖本机安装 Office；无 Office 时控件可实例化但无法打开 Word 文档
- 微软原版 1.3 为 32 位，存在打开 HTTP 文档等已知 bug，社区 fork 有修复
- 无内置服务器端点，「另存到服务器」需自行实现接收端（读 POST 文档流）
