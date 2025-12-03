# 实验一

开发环境：

  硬件信息
   - CPU: AMD EPYC 7543 32-Core Processor (4核心, 4线程)
   - 架构: x86_64
   - 内存: 未显示，但系统运行正常
   - 虚拟化: VMware虚拟机环境
   - 缓存: L1d/L1i缓存各128 KiB, L2缓存2 MiB, L3缓存1 GiB

  系统版本
   - 操作系统: Ubuntu Linux 22.04 (基于Linux 6.2.0-32-generic)
   - 内核版本: 6.2.0-32-generic #32~22.04.1-Ubuntu SMP 
     PREEMPT_DYNAMIC
   - 架构: x86_64 GNU/Linux

  开发工具版本
   - GCC编译器: 11.4.0 (Ubuntu 11.4.0-1ubuntu1~22.04)
   - Make工具: GNU Make 4.3
   - Git版本控制: 2.52.0
   - Vim编辑器: 8.2 (2019 Dec 12, with patches 1-3995)
   - VSCode编辑器: 1.106.3 (Commit: 
     bf9252a2fb45be6893dd8870c0bf37e2e1766d61)

运行环境：Ubuntu 20.04.3 LTS amd64

### 运行环境配置

本实验使用Docker容器进行搭建Ubuntu 20.04.3 LTS amd64的运行环境。



1. 确保已安装Docker
2. 使用以下命令启动容器并挂载当前目录：
   ```bash
   docker run -it -v $(pwd):~/sdu-se-os-design-nachos ubuntu:20.04.3
   ```
3. 进入容器后，安装必要的开发工具：
   ```bash
   apt update
   apt install -y build-essential g++ make git vim
   ```
4. 进入挂载的目录开始实验：
   ```bash
   cd ~/sdu-se-os-design-nachos
   ```

这种配置方式确保了在Ubuntu 20.04.3环境下进行实验，避免了环境差异带来的问题。

