# pinctrl 和 gpio 子系统实验
* 借助 pinctrl 和 gpio 子系统来简化 GPIO 驱动开发
## pinctrl 子系统
### pinctrl 子系统简介
* Linux 驱动讲究驱动分离与分层，pinctrl 和 gpio 子系统就是驱动分离与分层思想下的产物，驱动分离与分层其实就是按照面向对象编程的设计思想而设计的设备驱动框架。
* 来回顾一下上一章是怎么初始化 LED 灯所使用的 GPIO，步骤如下：
①、修改设备树，添加相应的节点，节点里面重点是设置 reg 属性，reg 属性包括了 GPIO相关寄存器。
②、获取 reg 属性中 GPIOI_MODER、GPIOI_OTYPER、GPIOI_OSPEEDR、GPIOI_PUPDR和 GPIOI_BSRR 这些寄存器的地址，并且初始化它们，这些寄存器用于设置 PI0 这个 PIN 的复用功能、上下拉、速度等。
③、 在②里面将 PI0 这个 PIN 设置为通用输出功能，因此需要设置 PI0 这个 GPIO 相关的寄存器，也就是设置 GPIOI_MODER 寄存器。
④、在②里面将 PI0 这个 PIN 设置为高速、上拉和推挽模式，就要需要设置 PI0 的GPIOI_OTYPER、GPIOI_OSPEEDR 和 GPIOI_PUPDR 这些寄存器。
* 总结一下，②中完成对 PI0 这个 PIN 的获取相关的寄存器地址，③和④设置这个 PIN 的复用功能、上下拉等，比如将 GPIOI_0 这个 PIN 设置为通用输出功能。如果使用过 STM32 单片机的话应该都记得，STM32 单片机也是要先设置某个 PIN 的复用功能、速度、上下拉等，然后再设置 PIN 所对应的 GPIO，STM32MP1 是从 STM32 单片机发展而来的，设置 GPIO 是一样。其实对于大多数的 32 位 SOC 而言，引脚的设置基本都是这两方面，因此 Linux 内核针对 __PIN的复用配置推出了 pinctrl 子系统，对于 GPIO 的电气属性配置推出了 gpio 子系统__。
* pinctrl 子系统主要工作内容如下：
①、获取设备树中 pin 信息。
②、根据获取到的 pin 信息来设置 pin 的复用功能
③、根据获取到的 pin 信息来设置 pin 的电气特性，比如上/下拉、速度、驱动能力等。
* 对于我们使用者来讲，只需要在设备树里面设置好某个 pin 的相关属性即可，其他的初始化工作均由 pinctrl 子系统来完成，pinctrl 子系统源码目录为 drivers/pinctrl。
### STM32MP1 的 pinctrl 子系统驱动
__PIN 配置信息详解__
*  ranges 属性，值为<0 0x10000000 0x100000>，此属性值指定了一个 1024KB(0x100000)的地址范围，子地址空间的物理起始地址为 0，父地址空间的物理起始地址为 0x10000000。
* 先打开 stm32mp15-pinctrl.dtsi 文件，你们能找到如下内容：
```
&pinctrl {
......
    m_can1_pins_a: m-can1-0 {
        pins1 {
            pinmux = <STM32_PINMUX('H', 13, AF9)>; /* CAN1_TX */
            slew-rate = <1>;
            drive-push-pull;
            bias-disable;
        };
        pins2 {
            pinmux = <STM32_PINMUX('I', 9, AF9)>; /* CAN1_RX */
            bias-disable;
        };
    };
    ......
    pwm1_pins_a: pwm1-0 {
        pins {
            pinmux = <STM32_PINMUX('E', 9, AF1)>, /* TIM1_CH1 */
            <STM32_PINMUX('E', 11, AF1)>, /* TIM1_CH2 */
            <STM32_PINMUX('E', 14, AF1)>; /* TIM1_CH4 */
            bias-pull-down;
            drive-push-pull;
            slew-rate = <0>;
        };
    };
......
};
```
* 向 pinctrl 节点追加数据，不同的外设使用的 PIN 不同、其配置也不同，因此一个萝卜一个坑，将某个外设所使用的所有 PIN 都组织在一个子节点里面。示例代码25.1.2.2 中 m_can1_pins_a 子节点就是 CAN1 的所有相关 IO 的 PIN 集合，pwm1_pins_a 子节点就是 PWM1 相关 IO 的 PIN 集合。绑定文档 Documentation/devicetree/bindings/pinctrl/st,stm32-pinctrl.yaml 描述了如何在设备树中设置 STM32 的 PIN 信息：每个 pincrtl 节点必须至少包含一个子节点来存放 pincrtl 相关信息，也就是 pinctrl 集，这个集合里面存放当前外设用到哪些引脚(PIN)、这些引脚应该怎么配置、复用相关的配置、上下拉、默认输出高电平还是低电平。
* 一般这个存放 pincrtl 集的子节点名字是“pins”，如果某个外设用到多种配置不同的引脚那么就需要多个 pins 子节点，比如示例代码 25.1.2.2 中第 535 行和 541行的 pins1 和 pins2 这两个子节点分别描述 PH13 和 PI9 的配置方法，由于 PH13 和 PI9 这两个IO 的配置不同，因此需要两个 pins 子节点来分别描述。第 555~562 行的 pins 子节点是描述PWM1 的相关引脚，包括 PE9、PE11、PE14，由于这三个引脚的配置是一模一样的，因此只需要一个 pins 子节点就可以了。
* 上面讲了，在 pins 子节点里面存放外设的引脚描述信息，这些信息包括：
__1、pinmux 属性__
* 此属性用来存放外设所要使用的所有 IO，示例代码 25.1.2.2 中第 536 行的 pinmux 属性内容如下：
```
pinmux = <STM32_PINMUX('H', 13, AF9)>;
```
* 可以看出，这里使用 STM32_PINMUX 这宏来配置引脚和引脚的复用功能，此宏定义在include/dt-bindings/pinctrl/stm32-pinfunc.h 文件里面，内容如下：
```
#define PIN_NO(port, line) (((port) - 'A') * 0x10 + (line))

#define STM32_PINMUX(port, line, mode) (((PIN_NO(port, line)) << 8)
                                        | (mode))
```
* 可以看出，STM32_PINMUX 宏有三个参数，这三个参数含义如下所示：
__port__：表示用那一组 GPIO(例：H 表示为 GPIO 第 H 组，也就是 GPIOH)。
__line__：表示这组 GPIO 的第几个引脚(例：13 表示为 GPIOH_13，也就是 PH13)。
__mode__：表示当前引脚要做那种复用功能(例：AF9 表示为用第 9 个复用功能)，这个需要查阅 STM32MP1 数据手册来确定使用哪个复用功能。
__2、电气属性配置__
* 电气特性在 pinctrl 子系统里不是必须的，可以不配置，但是 pinmux 属性是必须要设置的。stm32-pinctrl.yaml 文件里面也描述了如何设置STM32 的电气属性。

| 电气特性属性 | 类型 | 作用 |
| :--- | :---: | :--- |
|bias-disable|bootlean |禁止使用內部偏置电压| 
|bias-pull-up | bootlean | 内部上拉| 
|drive-push-pull | bootlean | 推挽输出| 
|drive-open-drain | bootlean | 开漏输出| 
|output-low | bootlean | 输出低电平| 
|output-high | bootlean | 输出高电平| 
|slew-rate | enum | 引脚的速度，可设置：0~3， 0 最慢，3 最高。| 
* 表 25.1.2.1 里的 bootlean 类型表示了在 pinctrl 子系统只要定义这个电气属性就行了，例如：我要禁用内部电压，只要在 PIN 的配置集里添加“bias-disable”即可，这个时候 bias-pull-down和 bias-pull-up 都不能使用了，因为已经禁用了内部电压，所以不能配置上下拉。enum 类型使用方法更简单跟 C 语言的一样，比如要设置 PIN 速度为最低就可以使“slew-rate=<0>”。在示例代码 25.1.2.3 里添加引脚的电气特性组合成，如示例代码 25.1.2.6 所示：
```
&pinctrl {

    myuart4_pins_a: myuart4-0 {
        pins1{
            pinmux = <STM32_PINMUX('H', 13, AF8)>;
            bias-pull-up;
            drive-push-pull;
        };
    };
};
```
### 设备树中添加 pinctrl 节点模板
__1、创建对应的节点__
```
&pinctrl {
    uart4_pins: uart4-0 {
    /* 具体的 PIN 信息 */
    };
};
```
__2、添加“pins”属性__
* 添加一个“pins”子节点，这个子节点是真正用来描述 PIN 配置信，要注意，同一个 pins子节点下的所有 PIN 电气属性要一样。如果某个外设所用的 PIN 可能有不同的配置，那么就需要多个 pins 子节点，例如 UART4 的 TX 和 RX 引脚配置不同，因此就有 pins1 和 pins2 两个子节点。这里我们只添加 UART4 的 TX 引脚，所以添加完 pins1 子节点以后如下所示：
```
&pinctrl {
    uart4_pins: uart4-0 {
        pins1{
        /* UART4 TX 引脚的 PIN 配置信息 */
        };
    };
};
```
__3、在“pins”节点中添加 PIN 配置信息__
* 最后在“pins”节点中添加具体的 PIN 配置信息，完成以后如下所示：
```
&pinctrl {
    uart4_pins: uart4-0 {
        pins1{
            pinmux = <STM32_PINMUX('G', 11, AF6)>; /* UART4_TX */
            bias-disable;
            drive-push-pull;
        };
    };
};
```
* 
按道理来讲，当我们将一个 IO 用作 GPIO 功能的时候也需要创建对应的 pinctrl 节点，并且将所用的 IO 复用为 GPIO 功能，比如将 PI0 复用为 GPIO 的时候就需要设置 pinmux 属性为：<STM32_PINMUX('I', 0, GPIO)>，但是！对于 STM32MP1 而言，如果一个 IO 用作 GPIO 功能的时候不需要创建对应的 pinctrl 节点！
## gpio 子系统
### gpio 子系统简介
* pinctrl 子系统重点是设置 PIN(有的 SOC 叫做 PAD)的复用和电气属性，如果 pinctrl 子系统将一个 PIN 复用为 GPIO 的话，那么接下来就要用到 gpio 子系统了。gpio 子系统顾名思义，就是用于初始化 GPIO 并且提供相应的 API 函数，比如设置 GPIO为输入输出，读取 GPIO 的值等。gpio 子系统的主要目的就是方便驱动开发者使用 gpio，驱动开发者在设备树中添加 gpio 相关信息，然后就可以在驱动程序中使用 gpio 子系统提供的 API函数来操作 GPIO，Linux 内核向驱动开发者屏蔽掉了 GPIO 的设置过程，极大的方便了驱动开发者使用 GPIO。
__STM32MP1 的 gpio 子系统驱动__

### gpio 子系统 API 函数
* 对于驱动开发人员，设置好设备树以后就可以使用 gpio 子系统提供的 API 函数来操作指定的 GPIO，gpio 子系统向驱动开发人员屏蔽了具体的读写寄存器过程。这就是驱动分层与分离的好处，大家各司其职，做好自己的本职工作即可。gpio 子系统提供的常用的 API 函数有下面几个：

__1、gpio_request 函数__
* gpio_request 函数用于申请一个 GPIO 管脚，在使用一个 GPIO 之前一定要使用 gpio_request进行申请，函数原型如下：
```
int gpio_request(unsigned gpio, const char *label)
```
* 函数参数和返回值含义如下：
__gpio__：要申请的 gpio 标号，使用 of_get_named_gpio 函数从设备树获取指定 GPIO 属性信息，此函数会返回这个 GPIO 的标号。
__label__：给 gpio 设置个名字。
__返回值__：0，申请成功；其他值，申请失败。

__2、gpio_free 函数__
* 如果不使用某个 GPIO 了，那么就可以调用 gpio_free 函数进行释放。函数原型如下：
```
void gpio_free(unsigned gpio)
```
* 函数参数和返回值含义如下：
__gpio__：要释放的 gpio 标号。
__返回值__：无。

__3、gpio_direction_input 函数__
* 此函数用于设置某个 GPIO 为输入，函数原型如下所示：
```
int gpio_direction_input(unsigned gpio)
```
* 函数参数和返回值含义如下：
__gpio__：要设置为输入的 GPIO 标号。
__返回值__：0，设置成功；负值，设置失败。

__4、gpio_direction_output 函数__
* 此函数用于设置某个 GPIO 为输出，并且设置默认输出值，函数原型如下：
```
int gpio_direction_output(unsigned gpio, int value)
```
* 函数参数和返回值含义如下：
__gpio__：要设置为输出的 GPIO 标号。
__value__：GPIO 默认输出值。
__返回值__：0，设置成功；负值，设置失败。

__5、gpio_get_value 函数__
此函数用于获取某个 GPIO 的值(0 或 1)，此函数是个宏，定义所示：
```
#define gpio_get_value __gpio_get_value
int __gpio_get_value(unsigned gpio)
```
* 函数参数和返回值含义如下：
__gpio__：要获取的 GPIO 标号。
__返回值__：非负值，得到的 GPIO 值；负值，获取失败。

__6、gpio_set_value 函数__
* 此函数用于设置某个 GPIO 的值，此函数是个宏，定义如下
```
#define gpio_set_value __gpio_set_value
void __gpio_set_value(unsigned gpio, int value)
```
* 函数参数和返回值含义如下：
__gpio__：要设置的 GPIO 标号。
__value__：要设置的值。
__返回值__：无
### 设备树中添加 gpio 节点模板
__1、创建 led 设备节点__
* 在根节点“/”下创建 test 设备子节点，如下所示：
```
led {
    /* 节点内容 */
};
```
__2、添加 GPIO 属性信息__
* 在 led 节点中添加 GPIO 属性信息，表明 test 所使用的 GPIO 是哪个引脚，添加完成以后如下所示：
```
led {
    compatible = "atk,led";
    gpio = <&gpioi 0 GPIO_ACTIVE_LOW>;
    status = "okay";
};
```
### 与 gpio 相关的 OF 函
__1、of_gpio_named_count 函数__
* of_gpio_named_count 函数用于获取设备树某个属性里面定义了几个 GPIO 信息，要注意的是空的 GPIO 信息也会被统计到，比如：
```
gpios = <0 &gpio1 1 2 0 &gpio2 3 4>;
```
上述代码的“gpios”节点一共定义了 4 个 GPIO，但是有 2 个是空的，没有实际的含义。通过 of_gpio_named_count 函数统计出来的 GPIO 数量就是 4 个，此函数原型如下：
```
int of_gpio_named_count(struct device_node *np, const char *propname)
```
* 函数参数和返回值含义如下：
__np__：设备节点。
__propname__：要统计的 GPIO 属性。
__返回值__：正值，统计到的 GPIO 数量；负值，失败。

__2、of_gpio_count 函数__
* 和 of_gpio_named_count 函数一样，但是不同的地方在于，此函数统计的是“gpios”这个属性的 GPIO 数量，而 of_gpio_named_count 函数可以统计任意属性的 GPIO 信息，函数原型如下所示：
```
int of_gpio_count(struct device_node *np)
```
* 函数参数和返回值含义如下：
__np__：设备节点。
__返回值__：正值，统计到的 GPIO 数量；负值，失败。

__3、of_get_named_gpio 函数__
* 此函数获取 GPIO 编号，因为 Linux 内核中关于 GPIO 的 API 函数都要使用 GPIO 编号，此函数会将设备树中类似<&gpioi 0(GPIO_ACTIVE_LOW | GPIO_PULL_UP)>的属性信息转换为对应的 GPIO 编号，此函数在驱动中使用很频繁！函数原型如下：
```
int of_get_named_gpio(struct device_node *np,
                    const char *propname, 
                    int index)
```
* 函数参数和返回值含义如下：
__np__：设备节点。
__propname__：包含要获取 GPIO 信息的属性名。
__index__：GPIO 索引，因为一个属性里面可能包含多个 GPIO，此参数指定要获取哪个 GPIO的编号，如果只有一个 GPIO 信息的话此参数为 0。
__返回值__：正值，获取到的 GPIO 编号；负值，失败。


