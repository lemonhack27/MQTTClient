# Linux 中断实验
* 不管是单片机裸机实验还是 Linux 下的驱动实验，中断都是频繁使用的功能，在裸机中使用中断我们需要做一大堆的工作，比如配置寄存器，使能 IRQ 等等。但是 Linux 内核提供了完善的中断框架，我们只需要申请中断，然后注册中断处理函数即可，使用非常方便，不需要一系列复杂的寄存器配置。
## Linux 中断简介
### Linux 中断 API 函数
__1、中断号__
* 每个中断都有一个中断号，通过中断号即可区分不同的中断，有的资料也把中断号叫做中断线。在 Linux 内核中使用一个 int 变量表示中断号。

__2、request_irq 函数__
* 在 Linux 内核中要想使用某个中断是需要申请的，request_irq 函数用于申请中断，request_irq函数可能会导致睡眠，因此不能在中断上下文或者其他禁止睡眠的代码段中使用 request_irq 函数。request_irq 函数会激活(使能)中断，所以不需要我们手动去使能中断，request_irq 函数原型如下：
```
int request_irq(unsigned int irq, 
                irq_handler_t handler, 
                unsigned long flags,
                const char *name, 
                void *dev)
```
* 函数参数和返回值含义如下：
__irq__：要申请中断的中断号。
__handler__：中断处理函数，当中断发生以后就会执行此中断处理函数。
__flags__：中断标志，可以在文件 include/linux/interrupt.h 里面查看所有的中断标志，这里我们介绍几个常用的中断标志，如表 31.1.1.1 所示：
__name__：中断名字，设置以后可以在/proc/interrupts 文件中看到对应的中断名字。
__dev__：如果将 flags 设置为 IRQF_SHARED 的话，dev 用来区分不同的中断，一般情况下将dev 设置为设备结构体，dev 会传递给中断处理函数 irq_handler_t 的第二个参数。
__返回值__：0 中断申请成功，其他负值 中断申请失败，如果返回-EBUSY 的话表示中断已经被申请了。

 | 标志 | 描述 |
 | :--- | --- |
 | IRQF_SHARED | 多个设备共享一个中断线，共享的所有中断都必须指定此标志。如果使用共享中断的话，request_irq 函数的 dev 参数就是唯一区分他们的标志。 | 
 | IRQF_ONESHOT  | 单次中断，中断执行一次就结束。 | 
 | IRQF_TRIGGER_NONE  | 无触发。 | 
 | IRQF_TRIGGER_RISING  | 上升沿触发。 | 
 | IRQF_TRIGGER_FALLING  | 下降沿触发。 | 
 | IRQF_TRIGGER_HIGH  | 高电平触发。 | 
 | IRQF_TRIGGER_LOW  | 低电平触发。 | 

__3、free_irq 函数__
* 使用中断的时候需要通过 request_irq 函数申请，使用完成以后就要通过 free_irq 函数释放掉相应的中断。如果中断不是共享的，那么 free_irq 会删除中断处理函数并且禁止中断。free_irq函数原型如下所示：
```
void free_irq(unsigned int irq, 
            void *dev)
```
* 函数参数和返回值含义如下：
__irq__：要释放的中断。
__dev__：如果中断设置为共享(IRQF_SHARED)的话，此参数用来区分具体的中断。共享中断只有在释放最后中断处理函数的时候才会被禁止掉。
__返回值__：无。

__4、中断处理函数__
* 使用 request_irq 函数申请中断的时候需要设置中断处理函数，中断处理函数格式如下所示：
```
irqreturn_t (*irq_handler_t) (int, void *)
```
* 第一个参数是要中断处理函数要相应的中断号。第二个参数是一个指向 void 的指针，也就是个通用指针，需要与 request_irq 函数的 dev 参数保持一致。用于区分共享中断的不同设备，dev 也可以指向设备数据结构。中断处理函数的返回值为 irqreturn_t 类型，irqreturn_t 类型定义如下所示：
```
enum irqreturn {
    IRQ_NONE = (0 << 0),
    IRQ_HANDLED = (1 << 0),
    IRQ_WAKE_THREAD = (1 << 1),
};

typedef enum irqreturn irqreturn_t;
```
* 可以看出 irqreturn_t 是个枚举类型，一共有三种返回值。一般中断服务函数返回值使用如下形式：
```
return IRQ_RETVAL(IRQ_HANDLED)
```

__5、中断使能与禁止函数__
* 常用的中断使用和禁止函数如下所示：
```
void enable_irq(unsigned int irq)
void disable_irq(unsigned int irq)
```
* enable_irq 和 disable_irq 用于使能和禁止指定的中断，irq 就是要禁止的中断号。disable_irq函数要等到当前正在执行的中断处理函数执行完才返回，因此使用者需要保证不会产生新的中断，并且确保所有已经开始执行的中断处理程序已经全部退出。在这种情况下，可以使用另外一个中断禁止函数：
```
void disable_irq_nosync(unsigned int irq)
```
* disable_irq_nosync 函数调用以后立即返回，不会等待当前中断处理程序执行完毕。上面三个函数都是使能或者禁止某一个中断，有时候我们需要关闭当前处理器的整个中断系统，也就是在学习 STM32 的时候常说的关闭全局中断，这个时候可以使用如下两个函数：
```
local_irq_enable()
local_irq_disable()
```
* local_irq_enable 用于使能当前处理器中断系统，local_irq_disable 用于禁止当前处理器中断系统。假如 A 任务调用 local_irq_disable 关闭全局中断 10S，当关闭了 2S 的时候 B 任务开始运行，B 任务也调用 local_irq_disable 关闭全局中断 3S，3 秒以后 B 任务调用 local_irq_enable 函数将全局中断打开了。此时才过去 2+3=5 秒的时间，然后全局中断就被打开了，此时 A 任务要关闭 10S 全局中断的愿望就破灭了，然后 A 任务就“生气了”，结果很严重，可能系统都要被A 任务整崩溃。为了解决这个问题，B 任务不能直接简单粗暴的通过 local_irq_enable 函数来打开全局中断，而是将中断状态恢复到以前的状态，要考虑到别的任务的感受，此时就要用到下面两个函数：
```
local_irq_save(flags)
local_irq_restore(flags)
```
* 这两个函数是一对，local_irq_save 函数用于禁止中断，并且将中断状态保存在 flags 中。local_irq_restore 用于恢复中断，将中断到 flags 状态。
### 上半部与下半部
* 我们在使用request_irq 申请中断的时候注册的中断服务函数属于中断处理的上半部，只要中断触发，那么中断处理函数就会执行。我们都知道中断处理函数一定要快点执行完毕，越短越好，但是现实往往是残酷的，有些中断处理过程就是比较费时间，我们必须要对其进行处理，缩小中断处理函数的执行时间。比如电容触摸屏通过中断通知 SOC 有触摸事件发生，SOC 响应中断，然后通过 IIC 接口读取触摸坐标值并将其上报给系统。但是我们都知道 IIC 的速度最高也只有400Kbit/S，所以在中断中通过 IIC 读取数据就会浪费时间。我们可以将通过 IIC 读取触摸数据的操作暂后执行，中断处理函数仅仅相应中断，然后清除中断标志位即可。这个时候中断处理过程就分为了两部分：
__上半部__：上半部就是中断处理函数，那些处理过程比较快，不会占用很长时间的处理就可以放在上半部完成。
__下半部__：如果中断处理过程比较耗时，那么就将这些比较耗时的代码提出来，交给下半部去执行，这样中断处理函数就会快进快出。
* 因此，Linux 内核将中断分为上半部和下半部的主要目的就是实现中断处理函数的快进快出，那些对时间敏感、执行速度快的操作可以放到中断处理函数中，也就是上半部。剩下的所有工作都可以放到下半部去执行，比如在上半部将数据拷贝到内存中，关于数据的具体处理就可以放到下半部去执行。至于哪些代码属于上半部，哪些代码属于下半部并没有明确的规定，一切根据实际使用情况去判断，这个就很考验驱动编写人员的功底了。这里有一些可以借鉴的参考点：
①、如果要处理的内容不希望被其他中断打断，那么可以放到上半部。
②、如果要处理的任务对时间敏感，可以放到上半部。
③、如果要处理的任务与硬件有关，可以放到上半部
④、除了上述三点以外的其他任务，优先考虑放到下半部。

__1、软中断__
* Linux 内核使用结构体 softirq_action 表示软中断， softirq_action结构体定义在文件 include/linux/interrupt.h 中，内容如下：
```
struct softirq_action
{
    void (*action)(struct softirq_action *);
};
```
* 在 kernel/softirq.c 文件中一共定义了 10 个软中断，如下所示：
```
static struct softirq_action softirq_vec[NR_SOFTIRQS];
```
* NR_SOFTIRQS 是枚举类型，定义在文件 include/linux/interrupt.h 中，定义如下：
```
enum
{
    HI_SOFTIRQ=0, /* 高优先级软中断 */
    TIMER_SOFTIRQ, /* 定时器软中断 */
    NET_TX_SOFTIRQ, /* 网络数据发送软中断 */
    NET_RX_SOFTIRQ, /* 网络数据接收软中断 */
    BLOCK_SOFTIRQ, 
    IRQ_POLL_SOFTIRQ, 
    TASKLET_SOFTIRQ, /* tasklet 软中断 */
    SCHED_SOFTIRQ, /* 调度软中断 */
    HRTIMER_SOFTIRQ, /* 高精度定时器软中断 */
    RCU_SOFTIRQ, /* RCU 软中断 */
    NR_SOFTIRQS
};
```
* 可以看出，一共有 10 个软中断，因此 NR_SOFTIRQS 为 10，因此数组 softirq_vec 有 10 个元素。softirq_action 结构体中的 action 成员变量就是软中断的服务函数，数组 softirq_vec 是个全局数组，因此所有的 CPU(对于 SMP 系统而言)都可以访问到，每个 CPU 都有自己的触发和控制机制，并且只执行自己所触发的软中断。但是各个 CPU 所执行的软中断服务函数确是相同的，都是数组 softirq_vec 中定义的 action 函数。要使用软中断，必须先使用 open_softirq 函数注册对应的软中断处理函数，open_softirq 函数原型如下：
```
void open_softirq(int nr, void (*action)(struct softirq_action *))
```
* 函数参数和返回值含义如下：
__nr__：要开启的软中断。
__action__：软中断对应的处理函数。
__返回值__：没有返回值。
* 注册好软中断以后需要通过 raise_softirq 函数触发，raise_softirq 函数原型如下：
```
void raise_softirq(unsigned int nr)
```
* 函数参数和返回值含义如下：
__nr__：要触发的软中断，在示例代码 31.1.2.3 中选择要注册的软中断。
__返回值__：没有返回值。
* 软中断必须在编译的时候静态注册！

__2、tasklet__
* tasklet 是利用软中断来实现的另外一种下半部机制，在软中断和 tasklet 之间，建议大家使用 tasklet。tasklet_struct 结构体如下所示：
```
struct tasklet_struct
{
    struct tasklet_struct *next; /* 下一个 tasklet */
    unsigned long state; /* tasklet 状态 */
    atomic_t count; /* 计数器，记录对 tasklet 的引用数 */
    void (*func)(unsigned long); /* tasklet 执行的函数 */
    unsigned long data; /* 函数 func 的参数 */
};
```
* func 函数就是 tasklet 要执行的处理函数，用户实现具体的函数内容，相当于中断处理函数。如果要使用 tasklet，必须先定义一个 tasklet_struct 变量，然后使用 tasklet_init 函数对其进行初始化，taskled_init 函数原型如下：
```
void tasklet_init(struct tasklet_struct *t,
                void (*func)(unsigned long), 
                unsigned long data);
```
* 函数参数和返回值含义如下：
__t__：要初始化的 tasklet
__func__：tasklet 的处理函数。
__data__：要传递给 func 函数的参数
__返回值__：没有返回值。
* 也可以使用宏 DECLARE_TASKLET 来一次性完成 tasklet 的定义和初始化，DECLARE_TASKLET 定义在 include/linux/interrupt.h 文件中，定义如下:
```
DECLARE_TASKLET(name, func, data)
```
* 其中 name 为要定义的 tasklet 名字，其实就是 tasklet_struct 类型的变量名，func 就是 tasklet的处理函数，data 是传递给 func 函数的参数。
* 在上半部，也就是中断处理函数中调用 tasklet_schedule 函数就能使 tasklet 在合适的时间运行，tasklet_schedule 函数原型如下：
```
void tasklet_schedule(struct tasklet_struct *t)
```
* 函数参数和返回值含义如下：
__t__：要调度的 tasklet，也就是 DECLARE_TASKLET 宏里面的 name。
__返回值__：没有返回值。
* 关于 tasklet 的参考使用示例如下所示：
```
/* 定义 taselet */
struct tasklet_struct testtasklet;
/* tasklet 处理函数 */
void testtasklet_func(unsigned long data)
{
    /* tasklet 具体处理内容 */
}
/* 中断处理函数 */
irqreturn_t test_handler(int irq, void *dev_id)
{
    ......
    /* 调度 tasklet */
    tasklet_schedule(&testtasklet);
    ......
}
/* 驱动入口函数 */
static int __init xxxx_init(void)
{
    ......
    /* 初始化 tasklet */
    tasklet_init(&testtasklet, testtasklet_func, data);
    /* 注册中断处理函数 */
    request_irq(xxx_irq, test_handler, 0, "xxx", &xxx_dev);
    ......
}
```
__3、工作队列__
* 工作队列是另外一种下半部执行方式，工作队列在进程上下文执行，工作队列将要推后的工作交给一个内核线程去执行，因为工作队列工作在进程上下文，因此工作队列允许睡眠或重新调度。因此如果你要推后的工作可以睡眠那么就可以选择工作队列，否则的话就只能选择软中断或 tasklet。

### 设备树中断信息节点
__1、GIC 中断控制器__
* STM32MP1 有三个与中断有关的控制器：GIC、EXTI 和 NVIC，其中 NVIC 是 Cortex-M4内核的中断控制器，本教程只讲解 Cortex-A7 内核，因此就只剩下了 GIC 和 EXTI。首先是 GIC，全称为：Generic Interrupt Controller。
* GIC 是 ARM 公司给 Cortex-A/R 内核提供的一个中断控制器，类似 Cortex-M 内核中的NVIC。目前 GIC 有 4 个版本:V1~V4，V1 是最老的版本，已经被废弃了。V2~V4 目前正在大量的使用。GIC V2 是给 ARMv7-A 架构使用的，比如 Cortex-A7、Cortex-A9、Cortex-A15 等，V3 和 V4 是给 ARMv8-A/R 架构使用的，也就是 64 位芯片使用的。STM32MP1 是 Cortex-A7 内核，因此我们主要讲解 GIC V2。GIC V2 最多支持 8 个核。ARM 会根据 GIC 版本的不同研发出不同的 IP 核，那些半导体厂商直接购买对应的 IP 核即可，比如 ARM 针对 GIC V2 就开发出了 GIC400 这个中断控制器 IP 核。当 GIC 接收到外部中断信号以后就会报给 ARM 内核，但是ARM 内核只提供了四个信号给 GIC 来汇报中断情况：VFIQ、VIRQ、FIQ 和 IRQ，他们之间的关系如图 31.1.3.1 所示：
![中断示意图](photos\中断示意图.png)
* 在图 31.1.3.1 中，GIC 接收众多的外部中断，然后对其进行处理，最终就只通过四个信号报给 ARM 内核，这四个信号的含义如下：
__VFIQ__:虚拟快速 FIQ。
__VIRQ__:虚拟外部 IRQ。
__FIQ__:快速中断 IRQ。
__IRQ__:外部中断 IRQ。
![GIC V2 总体框图](photos\GIC_V2总体框图.png)
* 图 31.1.3.2 中左侧部分就是中断源，中间部分就是 GIC 控制器，最右侧就是中断控制器向处理器内核发送中断信息。我们重点要看的肯定是中间的 GIC 部分，GIC 将众多的中断源分为分为三类：
①、SPI(Shared Peripheral Interrupt),共享中断，顾名思义，所有 Core 共享的中断，这个是最常见的，那些外部中断都属于 SPI 中断(注意！不是 SPI 总线那个中断) 。比如 GPIO 中断、串口中断等等，这些中断所有的 Core 都可以处理，不限定特定 Core。
②、PPI(Private Peripheral Interrupt)，私有中断，我们说了 GIC 是支持多核的，每个核肯定有自己独有的中断。这些独有的中断肯定是要指定的核心处理，因此这些中断就叫做私有中断。
③、SGI(Software-generated Interrupt)，软件中断，由软件触发引起的中断，通过向寄存器GICD_SGIR 写入数据来触发，系统会使用 SGI 中断来完成多核之间的通信。

__2、中断 ID__
中断源有很多，为了区分这些不同的中断源肯定要给他们分配一个唯一 ID，这些 ID 就是中断 ID。每一个 CPU 最多支持 1020 个中断 ID，中断 ID 号为 ID0~ID1019。这 1020 个 ID 包含了 PPI、SPI 和 SGI，那么这三类中断是如何分配这 1020 个中断 ID 的呢？这 1020 个 ID 分配如下：
ID0~ID15：这 16 个 ID 分配给 SGI。
ID16~ID31：这 16 个 ID 分配给 PPI。
ID32~ID1019：这 988 个 ID 分配给 SPI，像 GPIO 中断、串口中断等这些外部中断 ，至于具体到某个 ID 对应哪个中断那就由半导体厂商根据实际情况去定义了。比如 STM32MP1 系列总共分配了 265 个中断 ID(有很多并未使用，只是保留着)，加上前面属于 PPI 和 SGI 的 32 个ID，STM32MP1 的中断源共有 256+32=288 个。

__3、EXTI- 外部中断和事件控制器__
* EXTI 是什么东西呢？全称是：Extended interrupt and event controller，中文一般叫做外部中断和事件控制器。EXTI 是 ST 自己设计的，用来辅助 GIC 管理 STM32MP1 相应中断的。EXTI通过可配置的事件输入和直接事件输入来管理唤醒。它可以针对电源控制提供唤醒请求、针对CPU 事件输入生成事件。EXTI 唤醒请求可让系统从停止模式唤醒，以及让 CPU 从 CSTOP 和CSTANDBY 模式唤醒。此外，EXTI 还可以在运行模式下生成中断请求和事件请求，这个非常重要，因为在实际使用中 EXTI 主要是为 STM32 的 GPIO 中断服务的。
EXTI 主要特性如下：
·支持 76 个输入事件
·两个 CPU 内核都支持。
·所有事件输入均可让 CPU 唤醒。
EXTI 的异步输入事件可以分为 2 组：
①、可配置事件（来自能够生成脉冲的 I/O 或外设的信号），这类事件具有以下特性：
– 可选择的有效触发边沿。
– 中断挂起状态寄存器位。
– 单独的中断和事件生成屏蔽。
– 支持软件触发。
②、直接事件（来自其他外设的中断和唤醒源，需要在外设中清除），这类事件具有以下特性：
– 固定上升沿有效触发。
– EXTI 中无中断挂起状态寄存器位（中断挂起状态由生成事件的外设提供）。
– 单独的中断和事件生成屏蔽。
– 不支持软件触发。
对于 GPIO 中断来说，就是可配置事件，EXTI 和 GIC 的关系如图 31.1.3.4 所示：
![EXTI 框架](photos\EXTI框架.png)
* 从图 31.1.3.4 可以看出 STM32MP1 的中断处理方式有 5 种：
①、外设直接产生中断到 GIC，然后由 GIC 通知 CPU 内核。
②、GPIO 或外设产生中断到 EXTI，EXTI 将信号提交给 GIC，最终再将中断信号提交给CPU。
③、GPIO 或外设产生中断到 EXTI，EXTI 直接将中断信号提交给 CPU。Linux 系统会用到这三种中断方式，一个外设最多可以有两种中断方式，为啥是两种而不是三种后面会说。
* STM32MP1 的所有 GPIO 都有中断功能，而 GPIO 中断是我们最常用的功能。STM32 每一组 GPIO 最多有 16 个 IO，比如 PA0~PA15，因此每组 GPIO 就有 16 个中断，这 16 个 GPIO 事件输入对应 EXTI0~15，其中 PA0、PB0 等都对应 EXTI0，如图 31.1.3.5 所示
![EXTI0~15对应的GPIO](photos\EXTI0~15对应的GPIO.png)
* 如果要在 Linux 系统中使用中断，那么就需要在设备树中设置好中断属性信息，Linux 内核通过读取设备树中的中断属性信息来配置中断，GIC 控制器的设备树绑定信息参考文档Documentation/devicetree/bindings/ interrupt-controller/arm,gic.yaml， EXTI 控制器的设备树绑定信息参考文档 Documentation/devicetree/bindings/interrupt-controller/st,stm32-exti.txt。

__4、GIC 控制器节点__
* 打开 stm32mp151.dtsi 文件，其中的 intc 节点就是 GIC 的中断控制器节点，节点内容如下所示：
```
intc: interrupt-controller@a0021000 {
    compatible = "arm,cortex-a7-gic";
    #interrupt-cells = <3>;
    interrupt-controller;
    reg = <0xa0021000 0x1000>,
    <0xa0022000 0x2000>;
};
```
* 第 2 行，compatible 属性值为“arm,cortex-a7-gic”在 Linux 内核源码中搜索“arm,cortex-a7-gic”即可找到 GIC 中断控制器驱动文件。
第 3 行，#interrupt-cells 和#address-cells、#size-cells 一样。表示此中断控制器下设备的 cells大小，对于设备而言，会使用 interrupts 属性描述中断信息，#interrupt-cells 描述了 interrupts 属性的 cells 大小，也就是一条信息有几个 cells。每个 cells 都是 32 位整形值，对于 ARM 处理的GIC 来说，一共有 3 个 cells，这三个 cells 的含义如下：
第一个 cells：中断类型，0 表示 SPI 中断，1 表示 PPI 中断。
第二个 cells：中断号，对于 SPI 中断来说中断号的范围为 32~287(256 个)，对于 PPI 中断来说中断号的范围为 16~31，但是该 cell 描述的中断号是从 0 开始。
第三个 cells：标志，bit[3:0]表示中断触发类型，为 1 的时候表示上升沿触发，为 2 的时候表示下降沿触发，为 4 的时候表示高电平触发，为 8 的时候表示低电平触发。bit[15:8]为 PPI 中断的 CPU 掩码。
第 4 行，interrupt-controller 节点为空，表示当前节点是中断控制器。
* 打开 stm32mp151.dtsi，找到 SPI6 节点内容，如下所示：
```
spi6: spi@5c001000 {
    #address-cells = <1>;
    #size-cells = <0>;
    compatible = "st,stm32h7-spi";
    reg = <0x5c001000 0x400>;
    interrupts = <GIC_SPI 86 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&scmi0_clk CK_SCMI0_SPI6>;
    resets = <&scmi0_reset RST_SCMI0_SPI6>;
    dmas = <&mdma1 34 0x0 0x40008 0x0 0x0 0x0>,
            <&mdma1 35 0x0 0x40002 0x0 0x0 0x0>;
    dma-names = "rx", "tx";
    power-domains = <&pd_core>;
    status = "disabled";
 };
```
* 第 6 行，interrupts 描述中断中断源的信息，第一个表示中断类型，为 GIC_SPI，也就是共享中断。第二个表示中断号为 86，来源就是图 31.1.3.6。第三个表示中断触发类型是高电平触发。

__2、EXTI 控制器节点__
* 对于 GPIO 中断而言，要用到 EXTI，所以我们接下来看一下 EXTI 设备节点。打开stm32mp151.dtsi 文件，其中的 exti 节点就是 EXTI 的中断控制器节点，节点内容如下所示：
```
exti: interrupt-controller@5000d000 {
    compatible = "st,stm32mp1-exti", "syscon";
    interrupt-controller;
    #interrupt-cells = <2>;
    reg = <0x5000d000 0x400>;
    hwlocks = <&hsem 1 1>;
};
```
第 3 行，表明 exti 节点是个中断控制器。
第 4 行，interrupt-cells=2，表明 exti 的子节点里面第一个 cell 表示为中断号，也可以叫 EXTI事件编号，第二个 cell 表示中断标志位。其它的设备树属性和 GIC 控制器是一样的。
前面说了，GPIO 用到了 EXTI，因此 GPIO 节点信息里面的 EXTI 相关内容，在
stm32mp151.dtsi 文件中找到如下所示内容：
```
pinctrl: pin-controller@50002000 {
    #address-cells = <1>;
    #size-cells = <1>;
    compatible = "st,stm32mp157-pinctrl";
    ranges = <0 0x50002000 0xa400>;
    interrupt-parent = <&exti>;
    st,syscfg = <&exti 0x60 0xff>;
    hwlocks = <&hsem 0 1>;
    pins-are-numbered;
 
    gpioa: gpio@50002000 {
        gpio-controller;
        #gpio-cells = <2>;
        interrupt-controller;
        #interrupt-cells = <2>;
        reg = <0x0 0x400>;
        clocks = <&rcc GPIOA>;
        st,bank-name = "GPIOA";
        status = "disabled";
    };
......

    gpiok: gpio@5000c000 {
        gpio-controller;
        #gpio-cells = <2>;
        interrupt-controller;
        #interrupt-cells = <2>;
        reg = <0xa000 0x400>;
        clocks = <&rcc GPIOK>;
        st,bank-name = "GPIOK";
        status = "disabled";
    };
};

pinctrl_z: pin-controller-z@54004000 {
    #address-cells = <1>;
    #size-cells = <1>;
    compatible = "st,stm32mp157-z-pinctrl";
    ranges = <0 0x54004000 0x400>;
    pins-are-numbered;
    interrupt-parent = <&exti>;
    st,syscfg = <&exti 0x60 0xff>;
    hwlocks = <&hsem 0 1>;

    gpioz: gpio@54004000 {
        gpio-controller;
        #gpio-cells = <2>;
        interrupt-controller;
        #interrupt-cells = <2>;
        reg = <0 0x400>;
        clocks = <&scmi0_clk CK_SCMI0_GPIOZ>;
        st,bank-name = "GPIOZ";
        st,bank-ioport = <11>;
        status = "disabled";
    };
};
```
第 1~131 行，pinctrl 节点，此节点有 11 个子节点，gpioa~gpiok，分别对应 GPIOA~GPIOK这 11 组 IO。第 8 行，通过 interrupt-parent 属性指定 pinctrl 所有子节点的中断父节点为 exti，这样 GPIO 的中断就和 EXTI 联系起来了。第 11~20 行为 gpioa 节点，第 14 行表明 gpioa 节点也是个中断控制器，第 15 行设置 interrupt-cells 为 2，那么对于具体的 GPIO 而言，interrupts 属性第一个 cell 为某个 IO 在所处组的编号，第二个 cell 表示中断触发方式。比如现在要设置 PA1 这个引脚为下降沿触发，那么就要 interrupts = <1 IRQ_TYPE_EDGE_FALLING>。
第 133~154 行，pinctrl_z 节点，由于 GPIOZ 这一组对应的寄存器地址和 GPIOA~GPIOK 不是连续的，所以单独使用 pinctrl_z 来描述 GPIOZ，含义和 pinctrl 一样。
* 我们来看一个具体的应用，打开 stm32mp15xx-dkx.dtsi 文件，找到如下所示内容：
```
hdmi-transmitter@39 {
    compatible = "sil,sii9022";
    reg = <0x39>;
    iovcc-supply = <&v3v3_hdmi>;
    cvcc12-supply = <&v1v2_hdmi>;
    reset-gpios = <&gpioa 10 GPIO_ACTIVE_LOW>;
    interrupts = <1 IRQ_TYPE_EDGE_FALLING>;
    interrupt-parent = <&gpiog>;
    #sound-dai-cells = <0>;
    status = "okay";
};
```
* sii9022 是 ST 官方在开发板上的一个 HDMI 芯片，上述代码就是 sii9022 的节点信息，sii9022a 芯片有一个中断，此引脚链接到了 STM32MP1 的 PG1 上，此中断是下降沿触发。
第 7 行，interrupts 设置中断信息，1 表示本组内第一个 IO，在这里就是 PG1。IRQ_TYPE_EDGE_FALLING 表示下降沿触发。
第 8 行，interrupt-parent 属性设置中断控制器，这里是有 gpiog 作为中断控制器。结合上面的 interrupts 属性，这两行的目的就是设置 PG1 为下降沿出触发。可以看出使用起来是非常的简单，在我们实际编写代码的时候，只需要通过 interrupt-parent 和 interrupts 这两个属性即可设置某个 GPIO 的中断功能。
* 简单总结一下与中断有关的设备树属性信息：
①、#interrupt-cells，指定中断源的信息 cells 个数。
②、interrupt-controller，表示当前节点为中断控制器。
③、interrupts，指定中断号，触发方式等。
④、interrupt-parent，指定父中断，也就是中断控制器。
⑤、interrupts-extended，指定中断控制器、中断号、中断类型和触发方式，这个属性比较特殊，是新加入的。前面说了，要通过 interrupts 和 interrupt-parent 一起设置某个 IO 的中断方式。这里我们也可以只使用 interrupts-extended 属性一次性指定中断父节点，IO 编号，中断方式等。打开 stm32mp157f-ev1-a7-examples.dts 文件，里面有如下所示代码：
```
test_keys {
    compatible = "gpio-keys";
    #address-cells = <1>;
    #size-cells = <0>;
    autorepeat;
    status = "okay";
    /* gpio needs vdd core in retention for wakeup */
    power-domains = <&pd_core_ret>;

    button@1 {
        label = "PA13";
        linux,code = <BTN_1>;
        interrupts-extended = <&gpioa 13 IRQ_TYPE_EDGE_FALLING>;
        status = "okay";
        wakeup-source;
    };
};
```
* 很明显，上述代码用于描述一个按键，此按键采用中断方式，这个按键使用到了 PA13 这个引脚。第 13 行直接通过 interrupts-extended 属性描述了所有的中断信息，第一个参数为 gpioa，第二个参数为 13，第三个参数表示下降沿触发。如果使用 interrupts 和 interrupt-parent 来描述的话就是：
```
interrupt-parent = <&gpioa>;
interrupts = <13 IRQ_TYPE_EDGE_FALLING>;
```
* 大家根据自己的喜好来选择使用哪种形式描述即可。

### 获取中断号
* 编写驱动的时候需要用到中断号，我们用到的中断号，中断信息已经写到了设备树里面，因此可以通过 irq_of_parse_and_map 函数从 interupts 属性中提取到对应的设备号，函数原型如下：
```
unsigned int irq_of_parse_and_map(struct device_node *dev, int index)
```
* 函数参数和返回值含义如下：
__dev__：设备节点。
__index__：索引号，interrupts 属性可能包含多条中断信息，通过 index 指定要获取的信息。
__返回值__：中断号。
* 如果使用 GPIO 的话，可以使用 gpio_to_irq 函数来获取 gpio 对应的中断号，函数原型如下：
```
int gpio_to_irq(unsigned int gpio)
```
* 函数参数和返回值含义如下：
__gpio__：要获取的 GPIO 编号。
__返回值__：GPIO 对应的中断号。