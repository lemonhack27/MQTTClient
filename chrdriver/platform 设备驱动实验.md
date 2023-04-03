# platform 设备驱动实验
## Linux 驱动的分离与分层
### 驱动的分隔与分离
![分隔后的驱动框架](photos\分隔后的驱动框架.png)
* 这个就是驱动的分隔，也就是将主机驱动和设备驱动分隔开来，比如 I2C、SPI 等等都会采用驱动分隔的方式来简化驱动的开发。在实际的驱动开发中，一般 I2C 主机控制器驱动已经由半导体厂家编写好了，而设备驱动一般也由设备器件的厂家编写好了，我们只需要提供设备信息即可，比如 I2C 设备的话提供设备连接到了哪个 I2C 接口上，I2C 的速度是多少等等。相当于将设备信息从设备驱动中剥离开来，驱动使用标准方法去获取到设备信息(比如从设备树中获取到设备信息)，然后根据获取到的设备信息来初始化设备。 这样就相当于驱动只负责驱动，设备只负责设备，想办法将两者进行匹配即可。这个就是 Linux 中的总线(bus)、驱动(driver)和设备(device)模型，也就是常说的驱动分离。总线就是驱动和设备信息的月老，负责给两者牵线搭桥，如图 34.1.1.4 所示：

![Linux总线、驱动和设备模式](photos\Linux总线、驱动和设备模式.png)
* 当我们向系统注册一个驱动的时候，总线就会在右侧的设备中查找，看看有没有与之匹配的设备，如果有的话就将两者联系起来。同样的，当向系统中注册一个设备的时候，总线就会在左侧的驱动中查找看有没有与之匹配的设备，有的话也联系起来。Linux 内核中大量的驱动程序都采用总线、驱动和设备模式，我们一会要重点讲解的 platform 驱动就是这一思想下的产物。
### 驱动的分层
* Linux 下的驱动往往也是分层的，分层的目的也是为了在不同的层处理不同的内容。以其他书籍或者资料常常使用到的input(输入子系统，后面会有专门的章节详细的讲解)为例，简单介绍一下驱动的分层。input 子系统负责管理所有跟输入有关的驱动，包括键盘、鼠标、触摸等，最底层的就是设备原始驱动，负责获取输入设备的原始值，获取到的输入事件上报给 input 核心层。input 核心层会处理各种 IO 模型，并且提供 file_operations 操作集合。我们在编写输入设备驱动的时候只需要处理好输入事件的上报即可，至于如何处理这些上报的输入事件那是上层去考虑的，我们不用管。可以看出借助分层模型可以极大的简化我们的驱动编写，对于驱动编写来说非常的友好。
## platform 平台驱动模型简介
* 前面我们讲了设备驱动的分离，并且引出了总线(bus)、驱动(driver)和设备(device)模型，比如 I2C、SPI、USB 等总线。在 SOC 中有些外设是没有总线这个概念的，但是又要使用总线、驱动和设备模型该怎么办呢？为了解决此问题，Linux 提出了 platform 这个虚拟总线，相应的就有 platform_driver 和 platform_device。
### platform 总线
* Linux系统内核使用bus_type结构体表示总线，此结构体定义在文件include/linux/device.h，bus_type 结构体内容如下：
```
struct bus_type {
    const char *name;
    const char *dev_name;
    struct device *dev_root;
    const struct attribute_group **bus_groups;
    const struct attribute_group **dev_groups;
    const struct attribute_group **drv_groups;
    int (*match)(struct device *dev, struct device_driver *drv);
    int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
    int (*probe)(struct device *dev);
    int (*remove)(struct device *dev);
    void (*shutdown)(struct device *dev);
    int (*online)(struct device *dev);
    int (*offline)(struct device *dev);
    int (*suspend)(struct device *dev, pm_message_t state);
    int (*resume)(struct device *dev);
    int (*num_vf)(struct device *dev);
    int (*dma_configure)(struct device *dev);
    const struct dev_pm_ops *pm;
    const struct iommu_ops *iommu_ops;
    struct subsys_private *p;
    struct lock_class_key lock_key;
    bool need_parent_lock;
};
```
* 第 8 行，match 函数，此函数很重要，单词 match 的意思就是“匹配、相配”，因此此函数就是完成设备和驱动之间匹配的，总线就是使用 match 函数来根据注册的设备来查找对应的驱动，或者根据注册的驱动来查找相应的设备，因此每一条总线都必须实现此函数。match 函数有两个参数：dev 和 drv，这两个参数分别为 device 和 device_driver 类型，也就是设备和驱动。
* platform 总线是 bus_type 的一个具体实例，定义在文件 drivers/base/platform.c，platform 总线定义如下：
```
struct bus_type platform_bus_type = {
    .name = "platform",
    .dev_groups = platform_dev_groups,
    .match = platform_match,
    .uevent = platform_uevent,
    .dma_configure = platform_dma_configure,
    .pm = &platform_dev_pm_ops,
};
```
* platform_bus_type 就是 platform 平台总线，其中 platform_match 就是匹配函数。我们来看一下驱动和设备是如何匹配的，platform_match 函数定义在文件 drivers/base/platform.c 中，函数内容如下所示：
```
static int platform_match(struct device *dev,
                          struct device_driver *drv)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct platform_driver *pdrv = to_platform_driver(drv);

    /*When driver_override is set,only bind to the matching driver*/
    if (pdev->driver_override)
        return !strcmp(pdev->driver_override, drv->name);

    /* Attempt an OF style match first */
    if (of_driver_match_device(dev, drv))
        return 1;

    /* Then try ACPI style match */
    if (acpi_driver_match_device(dev, drv))
        return 1;

    /* Then try to match against the id table */
    if (pdrv->id_table)
        return platform_match_id(pdrv->id_table, pdev) != NULL;

    /* fall-back to driver name match */
        return (strcmp(pdev->name, drv->name) == 0);
}
```
* 驱动和设备的匹配有四种方法，我们依次来看一下：
* 第 11~12 行，第一种匹配方式， OF 类型的匹配，也就是设备树采用的匹配方式，of_driver_match_device 函数定义在文件 include/linux/of_device.h 中。device_driver 结构体(表示设备驱动)中有个名为of_match_table的成员变量，此成员变量保存着驱动的compatible匹配表，设备树中的每个设备节点的 compatible 属性会和 of_match_table 表中的所有成员比较，查看是否有相同的条目，如果有的话就表示设备和此驱动匹配，设备和驱动匹配成功以后 probe 函数就会执行。
* 第 15~16 行，第二种匹配方式，ACPI 匹配方式。
* 第 19~20 行，第三种匹配方式，id_table 匹配，每个 platform_driver 结构体有一个 id_table成员变量，顾名思义，保存了很多 id 信息。这些 id 信息存放着这个 platformd 驱动所支持的驱动类型。
* 第 23 行，第四种匹配方式，如果第三种匹配方式的 id_table 不存在的话就直接比较驱动和设备的 name 字段，看看是不是相等，如果相等的话就匹配成功。
* 对于支持设备树的 Linux 版本号，一般设备驱动为了兼容性都支持设备树和无设备树两种匹配方式。也就是第一种匹配方式一般都会存在，第三种和第四种只要存在一种就可以，一般用的最多的还是第四种，也就是直接比较驱动和设备的 name 字段，毕竟这种方式最简单了。

### platform 驱动
* platform_driver结构体表示platform驱动，此结构体定义在文件include/linux/platform_device.h 中，内容如下：
```
struct platform_driver {
    int (*probe)(struct platform_device *);
    int (*remove)(struct platform_device *);
    void (*shutdown)(struct platform_device *);
    int (*suspend)(struct platform_device *, pm_message_t state);
    int (*resume)(struct platform_device *);
    struct device_driver driver;
    const struct platform_device_id *id_table;
    bool prevent_deferred_probe;
};
```
* 第 2 行，probe 函数，当驱动与设备匹配成功以后 probe 函数就会执行，非常重要的函数！！一般驱动的提供者会编写，如果自己要编写一个全新的驱动，那么 probe 就需要自行实现。
* 第 7 行，driver 成员，为 device_driver 结构体变量，Linux 内核里面大量使用到了面向对象的思维，device_driver 相当于基类，提供了最基础的驱动框架。plaform_driver 继承了这个基类，然后在此基础上又添加了一些特有的成员变量。
* 第 8 行，id_table 表，也就是我们上一小节讲解 platform 总线匹配驱动和设备的时候采用的第三种方法，id_table 是个表(也就是数组)，每个元素的类型为 platform_device_id，platform_device_id 结构体内容如下：
```
struct platform_device_id {
    char name[PLATFORM_NAME_SIZE];
    kernel_ulong_t driver_data;
};
```
* device_driver 结构体定义在 include/linux/device.h，device_driver 结构体内容如下：
```
struct device_driver {
    const char *name;
    struct bus_type *bus;
    struct module *owner;
    const char *mod_name; /* used for built-in modules */
    bool suppress_bind_attrs; /* disables bind/unbind via sysfs */
    enum probe_type probe_type;
    const struct of_device_id *of_match_table;
    const struct acpi_device_id *acpi_match_table;
    int (*probe) (struct device *dev);
    int (*remove) (struct device *dev);
    void (*shutdown) (struct device *dev);
    int (*suspend) (struct device *dev, pm_message_t state);
    int (*resume) (struct device *dev);
    const struct attribute_group **groups;
    const struct attribute_group **dev_groups;
    const struct dev_pm_ops *pm;
    void (*coredump) (struct device *dev);
    struct driver_private *p;
};
```
* 第 8 行，of_match_table 就是采用设备树的时候驱动使用的匹配表，同样是数组，每个匹配项都为 of_device_id 结构体类型，此结构体定义在文件 include/linux/mod_devicetable.h 中，内容如下：
```
struct of_device_id {
    char name[32];
    char type[32];
    char compatible[128];
    const void *data;
};
```
* 第 4 行的 compatible 非常重要，因为对于设备树而言，就是通过设备节点的 compatible 属性值和 of_match_table 中每个项目的 compatible 成员变量进行比较，如果有相等的就表示设备和此驱动匹配成功。
* 在编写 platform 驱动的时候，首先定义一个 platform_driver 结构体变量，然后实现结构体中的各个成员变量，重点是实现匹配方法以及 probe 函数。当驱动和设备匹配成功以后 probe函数就会执行，具体的驱动程序在 probe 函数里面编写，比如字符设备驱动等等。
* 当我们定义并初始化好 platform_driver 结构体变量以后，需要在驱动入口函数里面调用platform_driver_register 函数向 Linux 内核注册一个 platform 驱动，platform_driver_register 函数原型如下所示：
```
int platform_driver_register (struct platform_driver *driver)
```
* 函数参数和返回值含义如下：
__driver__：要注册的 platform 驱动。
__返回值__：负数，失败；0，成功。
* 还需要在驱动卸载函数中通过 platform_driver_unregister 函数卸载 platform 驱动，platform_driver_unregister 函数原型如下：
```
void platform_driver_unregister(struct platform_driver *drv)
```
* 函数参数和返回值含义如下：
__drv__：要卸载的 platform 驱动。
__返回值__：无。

### platform 设备
* platform 驱动已经准备好了，我们还需要 platform 设备，否则的话单单一个驱动也做不了什么。platform_device 这个结构体表示 platform 设备，这里我们要注意，如果内核支持设备树的话就不要再使用 platform_device 来描述设备了，因为改用设备树去描述了。当然了，你如果一定要用 platform_device 来描述设备信息的话也是可以的。platform_device 结构体定义在文件include/linux/platform_device.h 中，结构体内容如下：
```
struct platform_device {
    const char *name;
    int id;
    bool id_auto;
    struct device dev;
    u64 platform_dma_mask;
    u32 num_resources;
    struct resource *resource;

    const struct platform_device_id *id_entry;
    char *driver_override; /* Driver name to force a match */

    /* MFD cell pointer */
    struct mfd_cell *mfd_cell;

    /* arch specific additions */
    struct pdev_archdata archdata;
};
```
* 第 2 行，name 表示设备名字，要和所使用的 platform 驱动的 name 字段相同，否则的话设备就无法匹配到对应的驱动。比如对应的 platform 驱动的 name 字段为“xxx-gpio”，那么此 name字段也要设置为“xxx-gpio”。
* 第 7 行，num_resources 表示资源数量，一般为第 8 行 resource 资源的大小。
* 第 8 行，resource 表示资源，也就是设备信息，比如外设寄存器等。Linux 内核使用 resource结构体表示资源，resource 结构体定义在 include/linux/ioport.h 文件里面，内容为：
```
struct resource {
    resource_size_t start;
    resource_size_t end;
    const char *name;
    unsigned long flags;
    unsigned long desc;
    struct resource *parent, *sibling, *child;
};
```
* start 和 end 分别表示资源的起始和终止信息，对于内存类的资源，就表示内存起始和终止地址，name 表示资源名字，flags 表示资源类型，可选的资源类型都定义在了文件include/linux/ioport.h 里面，如下所示：
```
#define IORESOURCE_BITS 0x000000ff /* Bus-specific bits */

#define IORESOURCE_TYPE_BITS 0x00001f00 /* Resource type */
#define IORESOURCE_IO 0x00000100 /* 表示 IO 口的资源 */
#define IORESOURCE_MEM 0x00000200 /* 表示内存地址 */
#define IORESOURCE_REG 0x00000300 /* Register offsets */
#define IORESOURCE_IRQ 0x00000400 /* 中断号 */
#define IORESOURCE_DMA 0x00000800 /* DMA 通道号 */
#define IORESOURCE_BUS 0x00001000 /* 总线号 */
......
#define IORESOURCE_PCI_FIXED (1<<4) /* Do not move resource */
```
* 在以前不支持设备树的Linux版本中，用户需要编写platform_device变量来描述设备信息，然后使用 platform_device_register 函数将设备信息注册到 Linux 内核中，此函数原型如下所示：
```
int platform_device_register(struct platform_device *pdev)
```
* 函数参数和返回值含义如下：
__pdev__：要注册的 platform 设备。
__返回值__：负数，失败；0，成功。
* 如果不再使用 platform 的话可以通过 platform_device_unregister 函数注销掉相应的 platform设备，platform_device_unregister 函数原型如下：
```
void platform_device_unregister(struct platform_device *pdev)
```
* 函数参数和返回值含义如下：
__pdev__：要注销的 platform 设备。
__返回值__：无。




