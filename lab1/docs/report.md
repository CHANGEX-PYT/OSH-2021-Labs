# 实验报告

## 1、GIT

​	使用git将代码等文件上传到lab1的目录下面。

​	基本步骤：

​	1、使用git config配置用户信息，再使用ssh-keygen生成SSH密钥，并将生成的密钥配置到GitHub中，完成传输文件前基本的配置。

​	2、对文件夹使用git init初始化，创建项目仓库，通过git remote add ...... .git关联远程库。

​	3、使用git add添加需要传输的文件，使用git commit添加更新的提示信息，最后使用git push成功上传代码等文件。

​	![](./src/github_msg.png)

## 2、LINUX内核

使用图形化的config配置页面对内核进行裁剪，在不影响1、2、3程序执行的前提下删去了尽可能多的对执行此三个程序不必要的功能：如全部的网络系统，全部的文件系统，全部的安全加密模块等，同时使用了节约空间而非优化性能的编译方式，使得最终的内核大小能满足小于6MB的实验要求。

最终裁剪大小在Linux系统下显示为5.6MB，在Windows系统下显示为5.3MB。经过测试，如果删除调试功能，使内核在执行程序时只输出程序显示的信息，不显示其他的调试信息，还可再减少0.4MB左右的大小。

内核大小如下：

![](./src/image_size.png)

运行状态见下文“初始内存盘” 部分配图。

## 3、初始内存盘

通过创立initrd文件的方式建立了初始内存盘，并使得内核在一开始运行的时候就执行我们的用户程序：init.c

init.c主要做了以下两件事：

1、创建设备文件，为后面调用1、2、3的测试程序做铺垫。

2、使用fork+execv函数的方式调用外部的1，2，3三个程序，完成测试。

init.c代码已置于linux/src目录下。

测试结果如下：

![](./src/kernel_run.png)

## 4、BOOT

阅读bootloader文件夹目录下的makefile文件发现，当我们调用make命令的时候，makefile会自动帮助我们编译bootloader并运行qemu对其进行测试。

测试结果如下所示：

![](./src/boot_run.png)

###### 第一组问题：

1、`xor ax, ax` 使用了异或操作 `xor`，这是在干什么？这么做有什么好处呢？

​		xor ax ax代表ax这个寄存器里面的值与自身异或，可以起到将该寄存器清零的作用，由于是只涉及到寄存器的操作，只需要在CPU内部进行运算，而不需要从内存等其他地方转移过来0，这样子效率更高。同时由于bootloader使用的是x86指令集，是变长指令集，同样的清零操作，如果使用movl完成的话，则需要更多的代码（3个字节），而xor实现同样的效果却只需要2个字节。

​		因此好处就是提高了效率，节约了空间。

2、`jmp $` 又是在干什么？

​		$ 代表当前的地址，因此jmp $ 便代表跳转到当前的地址，也即是一个死循环，代表不执行后续的程序了。

###### 第二组问题：

1、尝试修改代码，在目前已有的输出中增加一行输出“I am OK!”，样式不限，位置不限，但不能覆盖其他的输出。

​		已实现。阅读源码可知，输出信息前的汇编指令为：“log_info”，因此我们仿照其他输出信息的方式，在loader.asm中添加上我们的字符串“I am OK！”，并跟在最后一条输出语句的后面输出我们的字符串，其中log_info指令后面所跟三个参数应分别为字符串的label，字符串的长度，以及输出的行号。

​		对boot目录下的bootloader.img进行测试即可看到结果，如上图bootloader.img的运行结果所示。

## 5、思考题

**1、请简要解释 `Linux` 与 `Ubuntu`、`Debian`、`ArchLinux`、`Fedora` 等之间的关系和区别。**

​		Linux严格来说应该称为Linux内核，由于它过于精简，因此并不算是一个完整的操作系统。而Ubuntu、Debian、ArchLinux、Fedora等操作系统，都是基于某一个Linux内核构造出来的操作系统，通常被称为Linux发行版。一个典型的Linux发行版除了Linux内核以外，通常还会包括一系列的GNU工具和库，一些附带的软件，说明文档，一个桌面系统，一个窗口管理器和一个桌面环境。不同的发行版之间除了Linux内核以外的其他部分可能都不一样，但是相同的是他们都有着相同的核心，即Linux内核。

**3、简述树莓派启动的各个阶段。（hint：`bootcode.bin`、`start.elf`、`cmdline.txt` 等的作用）**

​		树莓派的启动大致可以分为3步：

​		1、树莓派芯片ROM中的代码加载位于SD卡第一个分区上的bootloader.bin文件。

​		2、由于此时CPU和ram都还没有初始化，因此bootloader.bin实际上是在GPU中执行，它的主要工作是初始化ram，并将也位于SD卡第一分区的start.etl加载到内存中。

​		3、start.etl先把ram空间划分为两部分：CPU访问空间和GPU访问空间，然后再从第一个分区加载树莓派配置文件config.txt。然后再从第一个分区加载cmdline.txt。cmdline.txt保存的是启动kernel的参数。start.efl最后再将kernel.img，ramdisk，dtb加载到内存的预定地址，然后向CPU发出重启信号，CPU便可以从内存中执行kernel的代码，进入软件定义的系统启动流程。

**5、为什么在 `x86` 环境下装了 `qemu-user` 之后可以直接运行 `arm64` 的 `busybox`，其中的具体执行过程是怎样的？`qemu-user` 和 `qemu-system` 的主要区别是什么？**

​		Qemu是一个开源的，通过纯软件来实现虚拟化的模拟器。qemu-user也即user mode模式，它的工作原理是利用代码动态翻译机制去执行不同主机架构的代码。也就是说，它可以将arm架构下的程序执行时的每一条指令动态翻译成x86架构下的指令，从而使得即使在x86的环境下也能使用arm64的busybox。

​		qemu-user和qemu-system的区别即user mode和system mode的区别，user mode的执行方式如上所述，是对程序的每一条指令进行动态翻译。而system mode则是去模拟整个电脑系统，利用其他VMM使用硬件提供的虚拟化支持，创建接近于主机性能的全功能虚拟机。简单来说，user mode是翻译功能，使某个程序能在其他架构上运行，而system mode则是直接创建了一个小型的arm模拟环境，使程序能在上面很好的运行。

**6、查阅 `PXE` 的资料，用自己的话描述它启动的过程。**

​		PXE即Pre-boot Execution Environment，是一种能够让计算机通过网络启动而不需使用本地操作系统的协议。协议分为client和server两端，PXE client在client网卡的ROM中，当client计算机引导时，将经历以下步骤：

​		1、PXE client 向DHCP Server请求IP地址，DHCP Server判断合法后返回IP地址和bootstrap的位置（一般在一台TFTP服务器上）。

​		2、PXE client 向TFTP Server 请求传输bootstrap，接收完成后执行bootstrap。

​		3、PXE client 向TFTP Server 请求传输配置文件，接收后读取配置文件并使用户根据需要进行选择合适的Linux内核和根文件系统。

​		4、PXE client 向TFTP Server 分别请求用户选择的Linux内核和Linux根文件系统，然后带参数启动Linux内核，完成PXE启动过程。