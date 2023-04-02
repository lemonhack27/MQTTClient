# Linux 内核定时器
* Linux 内核中有大量的函数需要时间管理，比如周期性的调度程序、延时程序、对于我们驱动编写者来说最常用的定时器。硬件定时器提供时钟源，时钟源的频率可以设置，设置好以后就周期性的产生定时中断，系统使用定时中断来计时。__中断周期性产生的频率就是系统频率__，也叫做节拍率(tick rate)(有的资料也叫系统频率)，比如 100Hz、1000Hz 等等说的就是系统节拍率。系统节拍率是可以设置的，单位是 Hz。
* 高节拍率和低节拍率的优缺点：
①、高节拍率会提高系统时间精度，如果采用 100Hz 的节拍率，时间精度就是 10ms，采用1000Hz 的话时间精度就是 1ms，精度提高了 10 倍。高精度时钟的好处有很多，对于那些对时间要求严格的函数来说，能够以更高的精度运行，时间测量也更加准确。
②、高节拍率会导致中断的产生更加频繁，频繁的中断会加剧系统的负担，1000Hz 和 100Hz的系统节拍率相比，系统要花费 10 倍的“精力”去处理中断。中断服务函数占用处理器的时间增加，但是现在的处理器性能都很强大，所以采用 1000Hz 的系统节拍率并不会增加太大的负载压力。根据自己的实际情况，选择合适的系统节拍率。
* Linux 内核使用全局变量 jiffies 来记录系统从启动以来的系统节拍数，系统启动的时候会将 jiffies 初始化为 0，jiffies 定义在文件 include/linux/jiffies.h 中，定义如下：
```
extern u64 __cacheline_aligned_in_smp jiffies_64;
extern unsigned long volatile __cacheline_aligned_in_smp 
__jiffy_arch_data jiffies;
```
* jiffies_64 和 jiffies 其实是同一个东西，jiffies_64 用于 64 位系统，而 jiffies 用于 32 位系统。为了兼容不同的硬件，jiffies 其实就是 jiffies_64 的低 32 位。
* 当我们访问 jiffies 的时候其实访问的是 jiffies_64 的低 32 位，使用 get_jiffies_64 这个函数可以获取 jiffies_64 的值。在 32 位的系统上读取的是 jiffies，在 64 位的系统上 jiffes 和 jiffies_64表示同一个变量，因此也可以直接读取 jiffies 的值。所以不管是 32 位的系统还是 64 位系统，都可以使用 jiffies。
* HZ 表示每秒的节拍数，jiffies 表示系统运行的 jiffies 节拍数，所以 jiffies/HZ 就是系统运行时间，单位为秒。不管是 32 位还是 64 位的 jiffies，都有溢出的风险，溢出以后会重新从 0 开始计数，相当于绕回来了，因此有些资料也将这个现象也叫做绕回。假如 HZ 为最大值 1000 的时候，32 位的 jiffies 只需要 49.7 (2^32/1000/60/60/24)天就发生了绕回，对于 64 位的 jiffies 来说大概需要5.8 亿年才能绕回，因此 jiffies_64 的绕回忽略不计。处理 32 位 jiffies 的绕回显得尤为重要，Linux 内核提供了如表 30.1.1.1 所示的几个 API 函数来处理绕回。

| 函数 | 描述 |
| :---: | :---: |
| time_after(unkown, known)  |
| time_before(unkown, known) |unkown 通常为 jiffies，known 通常是需要对比的值。 |
| time_after_eq(unkown, known) |
| time_before_eq(unkown, known) |

* 如果 unkown 超过 known 的话，time_after 函数返回真，否则返回假。如果 unkown 没有超过 known 的话 time_before 函数返回真，否则返回假。time_after_eq 函数和 time_after 函数类似，只是多了判断等于这个条件。同理，time_before_eq 函数和 time_before 函数也类似。比如我们要判断某段代码执行时间有没有超时，此时就可以使用如下所示代码：
```
unsigned long timeout;
timeout = jiffies + (2 * HZ); /* 超时的时间点 */

/*************************************
具体的代码
************************************/

/* 判断有没有超时 */
if(time_before(jiffies, timeout)) {
    /* 超时未发生 */
} else {
    /* 超时发生 */
}
```
* 为了方便开发，Linux 内核提供了几个 jiffies 和 ms、us、ns 之间的转换函数，如表 50.1.1.2所示：

| 函数 | 描述 |
| :---: | :---: |
| int jiffies_to_msecs(const unsigned long j) | 将 jiffies 类型的参数 j 分别转换为对应的毫秒、微秒、纳秒。 |
| int jiffies_to_usecs(const unsigned long j) | 将 jiffies 类型的参数 j 分别转换为对应的毫秒、微秒、纳秒。 |
| u64 jiffies_to_nsecs(const unsigned long j) | 将 jiffies 类型的参数 j 分别转换为对应的毫秒、微秒、纳秒。 |
| long msecs_to_jiffies(const unsigned int m) | 将毫秒、微秒、纳秒转换为 jiffies 类型。|
| long usecs_to_jiffies(const unsigned int u) | 将毫秒、微秒、纳秒转换为 jiffies 类型。|
| unsigned long nsecs_to_jiffies(u64 n) | 将毫秒、微秒、纳秒转换为 jiffies 类型。|

### 内核定时器简介
* Linux 内核定时器使用很简单，只需要提供超时时间(相当于定时值)和定时处理函数即可，当超时时间到了以后设置的定时处理函数就会执行，和我们使用硬件定时器的套路一样，只是使用内核定时器不需要做一大堆的寄存器初始化工作。在使用内核定时器的时候要注意一点，内核定时器并不是周期性运行的，超时以后就会自动关闭，因此如果想要实现周期性定时，那么就需要在定时处理函数中重新开启定时器。Linux 内核使用 timer_list 结构体表示内核定时器，timer_list 定义在文件include/linux/timer.h 中，定义如下：
```
struct timer_list {
/*
* All fields that change during normal runtime grouped to the
* same cacheline
*/
struct hlist_node entry;
unsigned long expires; /* 定时器超时时间，单位是节拍数 */
void (*function)(struct timer_list *);/* 定时处理函数*/
u32 flags; /* 标志位 */

#ifdef CONFIG_LOCKDEP
    struct lockdep_map lockdep_map;
#endif
};
```
* 要使用内核定时器首先要先定义一个 timer_list 变量，表示定时器，tiemr_list 结构体的expires 成员变量表示超时时间，单位为节拍数。比如我们现在需要定义一个周期为 2 秒的定时器，那么这个定时器的超时时间就是 jiffies+(2\*HZ)，因此 expires=jiffies+(2\*HZ)。function 就是定时器超时以后的定时处理函数，我们要做的工作就放到这个函数里面，需要我们编写这个定时处理函数，function 函数的形参就是我们定义的 timer_list 变量。
* 定义好定时器以后还需要通过一系列的 API 函数来初始化此定时器，这些函数如下：
__1、timer_setup 函数__
* timer_setup 函数负责初始化 timer_list 类型变量，当我们定义了一个 timer_list 变量以后一定要先用 timer_setup 初始化一下。timer_setup 函数原型如下：
```
void timer_setup(struct timer_list *timer, void (*func)(struct timer_list *), unsigned int flags)
```
* 函数参数和返回值含义如下：
__timer__：要初始化定时器。
__func__：定时器的回调函数，此函数的形参是当前定时器的变量。
__flags__: 标志位，直接给 0 就行。
__返回值__：没有返回值。

__2、add_timer 函数__
* add_timer 函数用于向 Linux 内核注册定时器，使用 add_timer 函数向内核注册定时器以后，定时器就会开始运行，函数原型如下：
```
void add_timer(struct timer_list *timer)
```
* 函数参数和返回值含义如下：
__timer__：要注册的定时器。
__返回值__：没有返回值。

__3、del_timer 函数__
* del_timer 函数用于删除一个定时器，不管定时器有没有被激活，都可以使用此函数删除。在多处理器系统上，定时器可能会在其他的处理器上运行，因此**在调用 del_timer 函数删除定时器之前要先等待其他处理器的定时处理器函数退出**。del_timer 函数原型如下：
```
int del_timer(struct timer_list * timer)
```
* 函数参数和返回值含义如下：
__timer__：要删除的定时器。
__返回值__：0，定时器还没被激活；1，定时器已经激活。

__4、del_timer_sync 函数__
* del_timer_sync 函数是 del_timer 函数的同步版，会等待其他处理器使用完定时器再删除，del_timer_sync 不能使用在中断上下文中。del_timer_sync 函数原型如下所示：
```
int del_timer_sync(struct timer_list *timer)
```
* 函数参数和返回值含义如下：
__timer__：要删除的定时器。
__返回值__：0，定时器还没被激活；1，定时器已经激活。

__5、mod_timer 函数__
* mod_timer 函数用于修改定时值，如果定时器还没有激活的话，mod_timer 函数会激活定时器！函数原型如下：
```
int mod_timer(struct timer_list *timer, unsigned long expires)
```
* 函数参数和返回值含义如下：
__timer__：要修改超时时间(定时值)的定时器。
__expires__：修改后的超时时间。
__返回值__：0，调用 mod_timer 函数前定时器未被激活；1，调用 mod_timer 函数前定时器已被激活。
* 内核定时器一般的使用流程如下所示：
```
struct timer_list timer; /* 定义定时器 */

/* 定时器回调函数 */
void function(struct timer_list *arg)
{ 
    /* 
    * 定时器处理代码
    */

    /* 如果需要定时器周期性运行的话就使用 mod_timer
    * 函数重新设置超时值并且启动定时器。
    */
    mod_timer(&dev->timertest, jiffies + msecs_to_jiffies(2000));
 }
 
 /* 初始化函数 */
 void init(void) 
 {
    timer_setup(&timerdev.timer, timer_function, 0); /* 初始化定时器 */
    timer.expires=jffies + msecs_to_jiffies(2000);/* 超时时间 2 秒 */
    add_timer(&timer); /* 启动定时器 */
 }

 /* 退出函数 */
 void exit(void)
 {
    del_timer(&timer); /* 删除定时器 */
    /* 或者使用 */
    del_timer_sync(&timer);
 }
```
### Linux 内核短延时函数
* 有时候我们需要在内核中实现短延时，尤其是在 Linux 驱动中。Linux 内核提供了毫秒、微秒和纳秒延时函数，这三个函数如表 30.1.3.1 所示：

 | 函数 | 描述 |
 | :---: | :---: |
 | void ndelay(unsigned long nsecs) | 
 | void udelay(unsigned long usecs)  | 纳秒、微秒和毫秒延时函数。 | 
 | void mdelay(unsigned long mseces) | 

