# HCOS
该项目是本人参考川合秀实所著的《30天自制操作系统》所制作的一个简单的操作系统。该项目使用了C语言和汇编进行开发。目前已经实现了GUI，内存分段，内存管理，中断，多任务等功能，并且提供了一些应用程序可供使用。

在项目过程中，本人在作者基础上通过自己思考以及整合前人成果对项目进行了较大的创新和改进，其过程如下列所示。

1. [运用小根堆思想重写定时器功能](documents/timer.md)
2. [双向链表与RBT，优化内存管理功能（上）](documents/memory1.md)
3. [双向链表与RBT，优化内存管理功能（中）](documents/memory2.md)。
4. 双向链表与RBT，优化内存管理功能（下）（待发布）。
6. 实现中文支持（待发布）。

使用方法：

本人使用的是VMware Workstation Pro来运行这一操作系统，使用方法参考这位前辈的文章。

[【操作系统】30天自制操作系统--(1)虚拟机加载最小操作系统_公子无缘的博客-CSDN博客_自制操作系统](https://blog.csdn.net/sinat_33408502/article/details/124013812)

在HCOS文件夹下的hcos文件夹中，运行make.bat脚本即可编译出hcos.sys文件，再回到HCOS文件夹，运行make.bat文件，即可打包出hcos.img镜像，这就是我们所能运行的操作系统了。

效果图（已打开大部分应用程序）：

![](documents\pic\QQ图片20220807022343.png)

