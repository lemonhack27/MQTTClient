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


