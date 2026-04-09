# RASimBackend

基于 AFSIM（Advanced Framework for Simulation, Integration, and Modeling）的军事仿真后端服务。通过 TCP 提供 JSON API，支持前端应用实时控制多域军事仿真。

## 功能特性

- **TCP JSON API** — 监听 `localhost:8888`，以 JSON 格式接收想定和控制指令
- **JSON 想定加载** — 前端发送 JSON 想定定义，后端自动转换为 AFSIM 想定文件并启动仿真
- **多域仿真** — 支持空中、海上、地面、太空、网络空间等多域作战仿真
- **实时控制** — 逐帧推进仿真，支持暂停/恢复/调速
- **多客户端** — 支持多个客户端同时连接，各自管理不同的仿真实例
- **编队管理** — 支持长机-僚机编队、相对位置配置
- **载荷配置** — 传感器、武器、通信设备等载荷灵活挂载
- **区域管理** — 禁飞区、威胁区、卫星排除区等

## 架构概览

```
前端客户端
    │  TCP (localhost:8888)
    ▼
CommunicationLayer ─── ClientSession
    │
    ▼
SimulationController
    │
    ▼
RASimServerManager (单例)
    │
    ▼
RASimServer (工作线程)
    │
    ▼
RASimApp ─── RASimApplication ─── RASimFrameStepSimulation
    │
    ▼
AFSIM SDK
```

## 技术栈

| 组件 | 版本 |
|------|------|
| C++ | C++17 |
| Qt | 5.14.0 (Core, Network) |
| CMake | 3.16+ |
| 编译器 | MSVC 2017 (x64) |
| AFSIM SDK | 随项目发布 |

## 目录结构

```
RASimBackend/
├── CMakeLists.txt              # CMake 构建配置
├── init_scenario_api.md        # 想定初始化 API 详细文档
├── src/                        # 源代码
│   ├── main.cpp                # 程序入口
│   ├── CommunicationLayer.*    # TCP 通信层
│   ├── ClientSession.*         # 客户端会话管理
│   ├── SimulationController.*  # 仿真控制器
│   ├── RASimServerManager.*    # 多仿真实例管理器
│   ├── RASimServer.*           # 仿真服务（含工作线程）
│   ├── RASimApp.*              # AFSIM 应用封装
│   ├── RASimApplication.*      # AFSIM 标准应用接口
│   ├── RASimFrameStepSimulation.* # 逐帧仿真实现
│   └── ScenarioConverter.*     # JSON→AFSIM 想定转换器
├── bin/
│   ├── release/                # 发布版本输出
│   ├── debug/                  # 调试版本输出
│   └── afsimsdk/               # AFSIM SDK（库文件和头文件）
└── build/                      # CMake 构建中间文件
```

## 构建

### 前置条件

- Visual Studio 2017（MSVC x64）
- Qt 5.14.0（安装至 `C:/Qt/Qt5.14.0/5.14.0/msvc2017_64`）
- CMake 3.16+
- AFSIM SDK（需手动准备，见下方说明）

### AFSIM SDK 环境准备

本项目依赖 AFSIM SDK，该 SDK **未纳入 git 仓库**，需手动部署。将 SDK 解压至项目根目录下的 `bin/afsimsdk/`，最终目录结构应如下：

```
bin/afsimsdk/
├── include/           # AFSIM 头文件
│   ├── core/          # 核心模块头文件（wsf, wsf_mil, wsf_space 等）
│   ├── tools/         # 工具库头文件（util, geodata, dis 等）
│   ├── mission/       # 任务模块头文件
│   ├── export/        # 导出接口
│   └── build/         # 构建生成头文件
├── lib/
│   └── Release/       # 链接库文件（.lib），约 200+ 个
└── 3rd_party/         # 第三方依赖（GDAL, GEOS, OSG 等）
```

CMakeLists.txt 中引用的关键库（共 22 个）：

| 库名 | 用途 |
|------|------|
| `wsf` | 仿真框架核心 |
| `wsf_mil` | 军事仿真扩展 |
| `wsf_mil_parser` | 军事想定解析 |
| `wsf_space` | 太空域仿真 |
| `wsf_cyber` | 网络空间仿真 |
| `wsf_nx` | 扩展功能 |
| `wsf_parser` | 想定解析器 |
| `wsf_weapon_server` | 武器服务 |
| `wsf_simdis` | 仿真可视化 |
| `sensor_plot_lib` | 传感器绘图 |
| `geodata` | 地理数据处理 |
| `packetio` | 网络数据包 I/O |
| `dis` | 分布式交互仿真 |
| `genio` | 通用 I/O |
| `util` / `util_script` | 工具库 |
| `tracking_filters` | 跟踪滤波器 |
| `profiling` | 性能分析 |
| 其他 | wsf_grammar_check, wsf_mtt, wsf_ripr, wsf_l16, wsf_util |

### 构建步骤

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

构建产物输出至 `bin/release/RASimBackend.exe`。

### 生成 Visual Studio 解决方案

```bash
mkdir build && cd build
cmake -G "Visual Studio 15 2017 Win64" ..
```

生成的 `.sln` 文件位于 `build/` 目录下，双击即可用 Visual Studio 打开。

## 通信协议

- **传输层**：TCP Socket
- **地址**：`localhost:8888`
- **帧格式**：`[4 字节小端长度][JSON 数据]`
- **编码**：UTF-8

### 支持的命令

| 命令 | 说明 |
|------|------|
| `init_scenario` | 加载 JSON 想定并启动仿真 |
| `step` | 推进一帧 |
| `pause` | 暂停仿真 |
| `resume` | 恢复仿真 |
| `stop` | 停止仿真 |
| `set_speed` | 设置仿真速度 |
| `set_fps` | 设置帧率 |

详细的 API 文档请参阅 [init_scenario_api.md](init_scenario_api.md)。

## 支持的仿真类型

- **空中平台**：战斗机、轰炸机、运输机
- **海上平台**：舰船、潜艇
- **地面平台**：车辆、人员、固定设施
- **太空平台**：卫星
- **传感器**：雷达、声纳、光电、电子战
- **武器**：导弹、炸弹、火炮
