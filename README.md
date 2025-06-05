# 软死锁测试内核模块 (Soft Lockup Test Kernel Module)

⚠️ **重要警告 / CRITICAL WARNING** ⚠️
```
本模块仅用于测试和学习目的，绝对不要在生产环境中使用！
This module is for testing and educational purposes only. 
NEVER use it in production environments!
```

## 简介

`softlockup_test` 是一个Linux内核模块，用于故意触发软死锁(soft lockup)现象，帮助理解和复现内核软死锁检测机制。该模块通过创建一个持续占用CPU并持有自旋锁的内核线程来模拟软死锁场景。

## 功能特性

- 创建一个内核线程，绑定到CPU 0
- 获取自旋锁并进入无限循环，模拟CPU密集型任务
- 触发内核的软死锁检测机制
- 可选择性地触发内核panic（需要取消注释相关代码）

## 技术原理

### 软死锁 (Soft Lockup)
软死锁是指一个CPU长时间(通常超过20-60秒)被某个任务占用，无法响应其他任务或中断的情况。Linux内核通过watchdog机制检测这种情况。

### 模块工作原理
1. **模块加载时**：创建名为`softlockup_thread`的内核线程
2. **线程执行**：获取自旋锁后进入无限循环
3. **CPU绑定**：线程被绑定到CPU 0，确保在特定CPU上触发软死锁
4. **触发检测**：内核watchdog检测到CPU长时间无响应，报告软死锁

## 安全警告

### 🚨 危险性说明
- **系统不稳定**：会导致系统响应缓慢或无响应
- **CPU占用**：会100%占用指定的CPU核心
- **内核panic**：如果启用panic选项，会导致系统崩溃
- **数据丢失风险**：系统崩溃可能导致未保存数据丢失
- **服务中断**：会影响系统上运行的所有服务

### 🛑 禁止使用场景
- ❌ 生产服务器
- ❌ 关键业务系统
- ❌ 包含重要数据的系统
- ❌ 多用户共享系统
- ❌ 任何不能承受系统崩溃的环境

### ✅ 适用场景
- ✅ 专门的测试虚拟机
- ✅ 内核开发调试环境
- ✅ 教学演示环境
- ✅ 已做好完整备份的隔离测试系统

## 编译和使用

### 前提条件
```bash
# 安装内核开发包
# Ubuntu/Debian:
sudo apt-get install linux-headers-$(uname -r) build-essential

# CentOS/RHEL:
sudo yum install kernel-devel kernel-headers gcc make

# 或者使用dnf (较新版本):
sudo dnf install kernel-devel kernel-headers gcc make
```

### 编译模块
```bash
# 创建Makefile (如果不存在)
cat << 'EOF' > Makefile
obj-m += softlockup_test.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
EOF

# 编译
make
```

### 加载和使用
```bash
# 加载模块 (需要root权限)
sudo insmod softlockup_test.ko

# 查看模块状态
lsmod | grep softlockup

# 查看内核日志
dmesg | tail -f

# 卸载模块 (可能需要强制重启系统)
sudo rmmod softlockup_test
```

## 预期现象

### 正常情况下的日志输出
```
[  xxx.xxx] task:xx (内核线程开始执行的日志)
[  xxx.xxx] task:xx (循环中的日志输出)
```

### 软死锁检测触发后
```
[  xxx.xxx] watchdog: BUG: soft lockup - CPU#0 stuck for XXs!
[  xxx.xxx] RIP: task+0x.../0x... [softlockup_test]
```

### 如果启用panic
```
[  xxx.xxx] Kernel panic - not syncing: softlockup: hung tasks
```

## 代码结构说明

### 主要函数
- `task()`: 内核线程执行函数，包含无限循环逻辑
- `softlockup_init()`: 模块初始化函数
- `softlockup_exit()`: 模块清理函数

### 关键变量
- `task0`: 内核线程指针
- `spinlock`: 自旋锁变量
- `val`: 线程参数

## 故障排除

### 系统无响应
如果系统变得无响应：
1. 等待内核watchdog触发（通常20-60秒）
2. 如果可能，通过SSH连接尝试卸载模块
3. 如果完全无响应，可能需要硬重启系统

### 模块无法卸载
```bash
# 强制卸载 (谨慎使用)
sudo rmmod -f softlockup_test

# 如果仍然失败，可能需要重启系统
sudo reboot
```

## 学习目标

使用此模块可以学习和理解：
- Linux内核软死锁检测机制
- 内核线程的创建和管理
- 自旋锁的使用和影响
- CPU绑定技术
- 内核watchdog机制

## 相关内核参数

```bash
# 查看软死锁检测相关参数
cat /proc/sys/kernel/watchdog
cat /proc/sys/kernel/watchdog_thresh
cat /proc/sys/kernel/soft_watchdog

# 临时禁用软死锁检测 (不推荐)
echo 0 > /proc/sys/kernel/watchdog
```

## 备选触发panic的方法

如需测试内核panic，取消注释代码中的以下行：
```c
/* panic("softlockup: hung tasks"); */
```
改为：
```c
panic("softlockup: hung tasks");
```

**警告**：启用此选项会立即导致系统崩溃！

## 免责声明

- 使用此模块造成的任何系统损坏、数据丢失或服务中断，作者概不负责
- 用户需完全了解其风险并自行承担责任
- 强烈建议仅在隔离的测试环境中使用
- 使用前请确保已做好完整的系统备份

## 许可证

GPL v2

## 作者

Original Author: Saiyam Doshi  
Documentation: Enhanced for safety and educational purposes

---

**再次提醒：此模块具有高度危险性，仅供学习和测试使用，绝对不要在生产环境中运行！**
