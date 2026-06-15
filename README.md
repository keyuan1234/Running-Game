# Subway Runner 2D

一个使用 **C11 + Win32/GDI** 编写的 Windows 伪 3D 跑酷游戏。玩家在三条轨道之间移动、跳跃、滑铲和加速，躲避从远处逼近的障碍物，并尽量刷新最近成绩。

项目保留了 `easyx4mingw_25.9.10` 目录，但游戏本身没有使用 EasyX。原因是随包 EasyX 头文件偏向 C++，而本项目按严格 C11 编译，图形、输入、音效和资源加载都直接基于 Win32 API、GDI、GDI+ 与 WinMM。

## 项目特色

- 伪 3D 透视跑道：障碍物和背景按深度缩放，远处靠近地平线，近处快速放大。
- 四层视差背景：天空渐变、城市剪影、轨道透视线、路边灯杆和标识共同营造前进感。
- 三类障碍：低矮路障需要跳跃，高处路障需要滑铲，列车障碍需要换道。
- 四名角色：普通 Runner、速度型 Sprinter、耐久型 Tank、二段跳 Jumper。
- 连续输入处理：换道和跳跃按单次请求消费，W 加速按键则持续生效。
- 最近成绩记录：游戏结束后输入名字，成绩保存到 `scores.dat`，菜单可查看最近 10 次记录。
- 可选资源系统：BMP 角色和障碍贴图不存在时，会自动使用 GDI 图形占位绘制。

## 构建与运行

推荐使用 MinGW-w64 的 GCC 工具链。Makefile 默认调用 `gcc`，并使用 C11、警告检查和优化参数：

```powershell
mingw32-make
.\running_game.exe
```

常用命令：

```powershell
mingw32-make clean
mingw32-make run
```

如果当前环境没有 `mingw32-make`，也可以按 Makefile 的参数直接用 `gcc` 构建：

```powershell
gcc -std=c11 -Wall -Wextra -O2 -c src\background.c -o src\background.o
gcc -std=c11 -Wall -Wextra -O2 -c src\main.c -o src\main.o
gcc -std=c11 -Wall -Wextra -O2 -c src\obstacle.c -o src\obstacle.o
gcc -std=c11 -Wall -Wextra -O2 -c src\player.c -o src\player.o
gcc -std=c11 -Wall -Wextra -O2 -c src\resources.c -o src\resources.o
gcc -std=c11 -Wall -Wextra -O2 -c src\scores.c -o src\scores.o
gcc -std=c11 -Wall -Wextra -O2 -c src\sound.c -o src\sound.o
gcc -std=c11 -Wall -Wextra -O2 -c src\util.c -o src\util.o
gcc -mwindows -o running_game.exe src\background.o src\main.o src\obstacle.o src\player.o src\resources.o src\scores.o src\sound.o src\util.o -luser32 -lgdi32 -lwinmm -lmsimg32 -lgdiplus
```

## 操作方式

菜单：

- `1`：进入角色选择
- `2`：查看帮助
- `3`：退出游戏
- `4`：查看最近成绩

角色选择：

- `A` / `D` 或方向键：切换角色
- `Enter` / `Space`：确认角色
- `Esc`：返回菜单

游戏中：

- `A`：向左换道
- `D`：向右换道
- `Space`：跳跃
- `S`：地面滑铲，空中快速下落
- `W`：按住加速
- `Esc`：返回菜单

游戏结束和成绩：

- `R`：重新开始
- `L`：查看最近成绩
- `Enter`：确认名字
- `Backspace`：删除名字字符

## 角色说明

- **Runner**：均衡角色，3 点生命值，适合熟悉路线和节奏。
- **Sprinter**：速度更快，速度增长也更快，但只有 2 点生命值。
- **Tank**：5 点生命值，碰到低矮路障不会受伤，受伤后的无敌时间更长，但速度成长较慢。
- **Jumper**：跳跃更高、滞空更久，并且可以二段跳。

选择 Tank 时会先显示一次 `Pay.png` 弹窗。关闭弹窗后再次确认 Tank 才会开始游戏。这个机制只是游戏里的趣味门槛，图片文件放在项目根目录即可。

## 玩法机制

游戏主循环目标帧率为 60 FPS。每帧会处理窗口消息、计算 `dt`、更新游戏状态，再使用内存 DC 双缓冲绘制到窗口，减少闪烁。

障碍物从远处生成并沿深度轴靠近玩家。障碍系统会限制同一组障碍之间的最小距离，也会避免同一种障碍连续出现过多次。玩家通过障碍后会得到基础分，存活时间也会持续增加分数。按住 `W` 可以获得更高速度和更快得分，但也会让反应窗口变短。

受伤后玩家会获得短暂无敌并闪烁显示。生命值归零后，如果进入最近成绩记录，会要求输入名字并保存分数、存活时间和通过障碍数量。

## 资源文件

游戏可以在没有图片资源的情况下运行。存在图片时会优先加载资源，不存在时使用代码绘制的占位角色和障碍。

可选资源：

- `assets/player_run_0.bmp`
- `assets/player_run_1.bmp`
- `assets/player_jump.bmp`
- `assets/player_crouch.bmp`
- `assets/obstacle_ground.bmp`
- `assets/obstacle_air.bmp`
- `assets/obstacle_train.bmp`
- `assets/hit.wav`
- `Pay.png`

`Pay.png` 使用 GDI+ 加载，放在项目根目录。BMP 资源主要使用 Win32 `LoadImageA` 加载，并会先尝试按可执行文件所在目录查找。

## 源码结构

- `src/main.c`：窗口创建、消息处理、状态机、主循环和主要界面绘制。
- `src/player.c`：角色属性、换道、跳跃、滑铲、受伤无敌和玩家绘制。
- `src/obstacle.c`：障碍生成、透视投影、碰撞判定、计分和绘制排序。
- `src/background.c`：天空、城市、跑道、轨道线和路边道具的视差背景。
- `src/resources.c`：BMP、PNG 和音效资源检测加载。
- `src/sound.c`：命中音效播放，缺少音效时使用系统提示音。
- `src/scores.c`：最近成绩的读取、插入和写回。
- `src/config.h`：窗口尺寸、物理参数、速度曲线、生命值和调试开关。
- `src/types.h`：游戏状态、实体结构体和共享类型定义。

## 调试提示

`src/config.h` 中的 `DEBUG_HITBOX` 默认为 `0`。把它改为 `1` 后重新构建，可以显示障碍物碰撞框，方便调试判定范围。

