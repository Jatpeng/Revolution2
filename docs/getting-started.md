## 入门指南（Revolution2 - Unreal Engine 项目）

本指南帮助你在 Windows 环境快速克隆、打开、编译并运行本项目。项目为 Unreal Engine C++ 项目，包含自研插件与多种玩法变体资源。

### 前置条件
- **操作系统**: Windows 10/11（x64）
- **Unreal Engine**: 建议 UE 5.3+（与 `Revolution2.uproject` 版本一致）
- **编译工具链**: Visual Studio 2022（桌面开发 C++ 工作负载）
  - 必选组件：MSVC v143、Windows 10/11 SDK、.NET SDK（用于构建工具）
- （可选）Git 与 Git LFS：若使用版本管理与大资源

### 获取与打开项目
1. 克隆或复制项目到本地，例如 `D:\Revolution2`。
2. 双击打开 `Revolution2.uproject`：
   - 首次打开时 UE 可能提示“编译缺失模块/是否现在编译”，选择“是”。
   - 若未生成工程文件，可右键 `Revolution2.uproject` 选择“Generate Visual Studio project files”。

### 目录结构速览
- `Source/`：C++ 源码
  - `Source/Revolution2/`：游戏模块源文件（约 22 个 .cpp、22 个 .h）
  - `Revolution2.Target.cs`、`Revolution2Editor.Target.cs`：目标配置
- `Content/`：游戏资源（多玩法变体）
  - `Maps/`：`Lobby.umap`、`StartupGameMap.umap`
  - `Variant_Horror/`、`Variant_Shooter/`、`FirstPerson/` 等子目录
- `Config/`：`DefaultEngine.ini`、`DefaultGame.ini` 等配置
- `Plugins/MultiplayerSessions/`：自研联机会话插件（含 C++ 源、资源、Binaries）
- `Binaries/`、`Intermediate/`、`Saved/`：构建产物、临时与日志

### 编译与运行（两种方式）
1) 在 Unreal Editor 中编译与运行：
   - 打开工程后，点击工具栏“编译（Compile）”。
   - 选择目标地图（例如 `Content/Maps/Lobby.umap`），点击“播放（Play）”。

2) 使用 Visual Studio 2022：
   - 双击 `.uproject` 生成 VS 解决方案，或打开根目录 `Revolution2.sln`。
   - 选择启动项目为 `UnrealEditor`（或对应生成的 Editor 目标）。
   - 启动参数可指定地图，例如：`-Project="D:\\Revolution2\\Revolution2.uproject" -game -log -Map=/Game/Maps/Lobby`。
   - 调试/运行即可。

### 关键关卡与玩法
- **Lobby.umap**：大厅/入口关卡。
- **StartupGameMap.umap**：启动/测试用关卡。
- **Variant_Shooter/**：射击玩法资源与蓝图。
- **Variant_Horror/**：恐怖玩法资源与蓝图。
- **FirstPerson/**：第一人称示例与输入设置。

### 插件：MultiplayerSessions
- 位置：`Plugins/MultiplayerSessions/`
- 用途：封装多人会话相关逻辑（创建/发现/加入 Session 等）。
- 结构：
  - `Source/`：C++ 源（.h/.cpp）
  - `MultiplayerSessions.uplugin`：插件描述
  - `Content/`、`Binaries/`、`Intermediate/`：资源与构建产物

> 若首次打开工程提示插件需要重新编译，请在 Editor 内或 VS 中编译后重启 Editor。

### 打包（Windows）
1. 通过 Editor：`File > Package Project > Windows (64-bit)`，选择输出目录。
2. 产物可在 `Saved/StagedBuilds/Windows` 或自定义输出目录找到。

### 常见问题排查
- 缺少编译工具：安装 VS2022 C++ 工作负载、MSVC v143、Windows SDK。
- 引擎版本不匹配：用与 `.uproject` 兼容的 UE 版本打开；必要时执行“Switch Unreal Engine Version”。
- 编译失败/模块缺失：右键 `.uproject` 生成工程文件；在 VS“清理-重建”；删除 `Intermediate/` 后重试。
- 资源缺失：确认 `Content/` 完整；若使用 LFS，执行 `git lfs pull`。

### 日志与诊断
- 运行时日志：`Saved/Logs/Revolution2.log`
- 构建日志：VS 输出窗口或 `Saved/Logs/UnrealBuildTool` 子目录
- 打包日志：`Saved/Logs/UnrealPak.log`

### 代码风格与建议
- 保持清晰的类/文件命名，减少跨模块耦合。
- 优先使用早返回与简化分支；避免无意义 try/catch。
- 仅为非显而易见的逻辑添加简明注释；提交前确保能在 Editor 下编译通过。

### 下一步
- 阅读 `Source/Revolution2/` 了解模块入口与子系统。
- 浏览 `Content/Variant_*` 了解不同玩法的资源组织与蓝图。
- 根据需要扩展 `MultiplayerSessions` 插件实现联机流程。


