# 地铁跑酷 2D

纯 C11 + Win32/GDI 写的一个伪 3D 跑酷小游戏。没依赖任何第三方图形库——EasyX
的 MinGW 头文件需要 C++ 编译所以没用，全部 GDI 裸写。

## 编译运行

Windows 上装好 MinGW-w64：

```powershell
mingw32-make           # 编译
mingw32-make clean     # 清理 .o 和 exe
mingw32-make run       # 编译完直接跑
```

或者直接：

```powershell
.\running_game.exe
```

## 操作

- `1` 开始游戏
- `2` 游戏帮助
- `3` 退出
- `4` 排行榜
- `A` 左移一个车道
- `D` 右移一个车道
- `Space` 跳跃，翻过低障碍
- `S` 滑铲，钻过高障碍；在空中按时快速下坠
- `W` 按住加速
- `Esc` 返回菜单
- `R` 结束后重开
- `L` 结束后查看排行榜

## 玩法

三条车道，障碍物从远处逼近（近大远小，伪 3D）。

- 红色低栏：跳过去
- 蓝色高栏：滑铲钻过去
- 火车车厢：换车道躲
- 受伤后 3 秒无敌，角色闪烁
- 有跳跃缓冲：落地前 0.12 秒内按跳跃，落地后自动起跳

存活越久分数越高，每经过一个障碍额外加分。HP 归零游戏结束。

## 排行榜

本地分数记录保存在 `scores.dat`，保留前 10 名。如果本次分数够上榜，游戏结束后会弹姓名输入界面，输入名字后存入排行榜，主菜单和游戏结束界面都能查看。

## 图片和音效

没有外部资源也能玩——每个模型都有 GDI 绘制的占位图。如果你想用自定义图片，把 BMP 文件放进 `assets/` 目录：

- `assets/player_run_0.bmp`
- `assets/player_run_1.bmp`
- `assets/player_jump.bmp`
- `assets/player_crouch.bmp`
- `assets/obstacle_ground.bmp`
- `assets/obstacle_air.bmp`
- `assets/hit.wav`

用 BMP 格式最省事，Win32/GDI 原生支持。
