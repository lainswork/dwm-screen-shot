# [dwm-screen-shot](https://github.com/lainswork/dwm-screen-shot)
## 将shellcode注入dwm.exe 进行DXGI屏幕截取
> imgui 显示部分的代码有些丑....无伤大雅
## 特点

- 兼容绝大部分Windows10系统
- FPS游戏反作弊(anti-cheat)杀手锏

## 使用 Use

```shell
// 确保你已经安装了VS2019或以上 Make sure u have installed Visual Studio 2019 or later version
// 打开PowerShell并进入一个为项目准备的文件夹,依次输入以下命令, Enter the following commands in PowerShell

> git clone https://github.com/lainswork/dwm-screen-shot.git

> cd dwm-screen-shot

> git submodule update --init --recursive

> cd ./build

> devenv dwm-screen-shot.sln /build "Debug|x64" /Project dwm-screen-shot

> cd ../bin/x64/Debug

> .\dwm-screen-shot

```

## 注意
##### 你可能会发现源代码中存在一个 payload.hpp, 这是截屏的主要代码生成的 shellcode 
- 请查看  [shellcode-factory](https://github.com/lainswork/shellcode-factory)
- 在 shellcode-factory/shellcode-payload/dwm-screen-shot-demo.cpp 中你会见到它是如何编写的
## 依赖
- [shellcode-factory](https://github.com/lainswork/shellcode-factory)

- [imgui](https://github.com/ocornut/imgui) 
  - 请使用我的分支:[imgui](https://github.com/lainswork/imgui) 
- [raw_pdb](https://github.com/MolecularMatters/raw_pdb)
  - 请使用我的分支:[raw_pdb](https://github.com/lainswork/raw_pdb) 


## 知识 knowledge
> Direct3D(...Dx9 Dx10 Dx11 Dx12...)与 DXGI
- Direct3D是一种底层绘图API（application programming interface，应用程序接口），它可以让我们可以通过3D硬件加速绘制3D世界。从本质上讲，Direct3D提供的是一组软件接口，我们可以通过这组接口来控制绘图硬件。
[在以前图形子系统都归D3D，结果D3D8/D3D9分别有一套代码用来管理swap chain。在Vista+里，图形API越来越多，D3D9/D3D10/D3D11/D3D12，都来一套swap chain太没意义了。于是重构成所有API可以共享一份swap chain的代码，这就放在DXGI。除此之外，窗口全屏化之类的事情也都归DXGI了，你可以认为屏幕输出的部分都归DXGI。](https://www.zhihu.com/question/36501678/answer/67786884)

> DWM
- Desktop Window Manager (dwm.exe) 是窗口管理器的组成部分.[后来DXGI又加了一些底层的功能，用来跟DWM打交道，比如拷贝混合后的屏幕，设备旋转，跨屏幕窗口](https://www.zhihu.com/question/36501678/answer/67786884)

> VEH hook
- ...未完待续

> 多线程下的代码注入
- ...未完待续
