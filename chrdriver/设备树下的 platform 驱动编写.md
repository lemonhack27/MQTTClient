# 设备树下的 platform 驱动编写
## 设备树下的 platform 驱动简介
* platform 驱动框架分为总线、设备和驱动，其中总线不需要我们这些驱动程序员去管理，这个是 Linux 内核提供的，我们在编写驱动的时候只要关注于设备和驱动的具体实现即可。在没有设备树的 Linux 内核下，我们需要分别编写并注册 platform_device 和 platform_driver，分别代表设备和驱动。在使用设备树的时候，设备的描述被放到了设备树中，因此 platform_device 就不需要我们去编写了，我们只需要实现 platform_driver 即可。
### 修改 pinctrl-stm32.c 文件
* 按道理来讲，在我们使用某个引脚的时候需要先配置其电气属性，比如复用、输入还是输入、默认上下拉等！但是在前面的实验中均没有配置引脚的电气属性，也就是引脚的 pinctrl配置。这是因为 ST 针对 STM32MP1 提供的 Linux 系统中，其 pinctrl 配置的电气属性只能在platform 平台下被引用，前面的实验都没用到 platform，所以 pinctrl 配置是不起作用的！
* 在使用 NXP 的 I.MX6ULL 芯片的时候，Linux 系统启动运行过程中会自动解析设备树下的 pinctrl 配置，然后初始化引脚的电气属性，不需要 platform 驱动框架。所以 pinctrl 什么时候有效，不同的芯片厂商有不同的处理方法，一切以实际所使用的芯片为准！
* 对于 STM32MP1 来说，在使用 pinctrl 的时候需要修改一下 pinctrl-stm32.c 这个文件，否则当某个引脚用作 GPIO 的时候会提示此引脚无法申请到。
* 打开pinctrl-stm32.c 这个文件，找到如下所示代码：
```
static const struct pinmux_ops stm32_pmx_ops = {
.get_functions_count = stm32_pmx_get_funcs_cnt,
.get_function_name = stm32_pmx_get_func_name,
.get_function_groups = stm32_pmx_get_func_groups,
.set_mux = stm32_pmx_set_mux,
.gpio_set_direction = stm32_pmx_gpio_set_direction,
.strict = true,
};
```
* 第 7 行的 strict 成员变量默认为 true，我们需要将其改为 false。
* 修改完成以后使用如下命令重新编译 Linux 内核：
```
make uImage LOADADDR=0XC2000040 -j16 //编译内核
```
* 编译完成以后使用新的 uImage 启动即可。

### 创建设备的 pinctrl 节点
* 在 platform 驱动框架下必须使用 pinctrl 来配置引脚复用功能。我们以本章实验需要用到的 LED0 为例，编写 LED0 引脚的 pinctrl 配置。打开 stm32mp15-pinctrl.dtsi 文件，STM32MP1 的所有引脚 pinctrl 配置都是在这个文件里面完成的，在 pinctrl 节点下添加如下所示内容：
```
led_pins_a: gpioled-0 {
    pins {
        pinmux = <STM32_PINMUX('I', 0, GPIO)>;
        drive-push-pull;
        bias-pull-up;
        output-high;
        slew-rate = <0>;
    };
};
```
* 示例代码 35.1.2.1 中的 led_pins_a 节点就是 LED 的 pinctrl 配置，把 PI0 端口复用为 GPIO功能，同时设置 PI0 的电气特性。我们已经在 25.1.1 小节详细的讲解过如何配置 STM32MP1 的电气属性，大家可以回过头去看一下，这里我们就简单介绍一下 LED0 的配置：
第 3 行，设置 PI0 复用为 GPIO 功能。
第 4 行，设置 PI0 为推挽输出。
第 5 行，设置 PI0 内部上拉。
第 6 行，设置 PI0 默认输出高电平。
第 7 行，设置 PI0 的速度为 0 档，也就是最慢。
### 在设备树中创建设备节点
* 接下来要在设备树中创建设备节点来描述设备信息，重点是要设置好 compatible 属性的值，因为 platform 总线需要通过设备节点的 compatible 属性值来匹配驱动！这点要切记。修改 25.4.1.2小节中我们创建的 gpioled 节点，修改以后的内容如下：
```
gpioled {
    compatible = "alientek,led";
    pinctrl-names = "default";
    status = "okay";
    pinctrl-0 = <&led_pins_a>;
    led-gpio = <&gpioi 0 GPIO_ACTIVE_LOW>;
};
```
* 第 2 行的 compatible 属性值为“alientek,led”，因此一会在编写 platform 驱动的时候of_match_table 属性表中要有“alientek,led”。
第 5 行里，pinctrl-0 属性设置 LED 的 PIN 对应的 pinctrl 节点，也就是我们在示例代码 35.1.1中编写的 led_pins_a。
### 编写 platform 驱动的时候要注意兼容属性
* 上一章已经详细的讲解过了，在使用设备树的时候 platform 驱动会通过 of_match_table 来保存兼容性值，也就是表明此驱动兼容哪些设备。所以，of_match_table 将会尤为重要，比如本例程的 platform 驱动中 platform_driver 就可以按照如下所示设置：
```
static const struct of_device_id led_of_match[] = {
    { .compatible = "alientek,led" }, /* 兼容属性 */
    { /* Sentinel */ }
};

MODULE_DEVICE_TABLE(of, led_of_match);

static struct platform_driver led_platform_driver = {
    .driver = {
        .name = "stm32mp1-led",
        .of_match_table = led_of_match,
    },
    .probe = led_probe,
    .remove = led_remove,
};
```
* 第 1~4 行，of_device_id 表，也就是驱动的兼容表，是一个数组，每个数组元素为 of_device_id类型。每个数组元素都是一个兼容属性，表示兼容的设备，一个驱动可以跟多个设备匹配。这里我们仅仅匹配了一个设备，那就是示例代码 35.1.2 中创建的 gpioled 这个设备。第 2 行的compatible 值为“alientek,led”，驱动中的 compatible 属性和设备中的 compatible 属性相匹配，因此驱动中对应的 probe 函数就会执行。注意第 3 行是一个空元素，在编写 of_device_id 的时候最后一个元素一定要为空！
第 6 行，通过 MODULE_DEVICE_TABLE 声明一下 led_of_match 这个设备匹配表。
第 11 行，设置 platform_driver 中的 of_match_table 匹配表为上面创建的 leds_of_match，至此我们就设置好了 platform 驱动的匹配表了。
* 最后就是编写驱动程序，基于设备树的 platform 驱动和上一章无设备树的 platform 驱动基本一样，都是当驱动和设备匹配成功以后先根据设备树里的 pinctrl 属性设置 PIN 的电气特性再去执行 probe 函数。我们需要在 probe 函数里面执行字符设备驱动那一套，当注销驱动模块的时候 remove 函数就会执行，都是大同小异的。
## 检查引脚复用配置
### 检查引脚 pinctrl 配置
* STM32MP1 的一个引脚可以复用为多种功能，比如 PI0 可以作为 GPIO、TIM5_CH4、SPI2_NSS、DCMI_D13、LCD_G5 等。我们在做 STM32 单片机开发的时候，一个 IO 可以被多个外设使用，比如 PI0 同时作为 TIM5_CH4、LCD_G5，但是同一时刻只能用做一个功能，比如做 LCD_G5 的时候就不能做 TIM5_CH4！在嵌入式 Linux 下，我们要严格按照一个引脚对应一个功能来设计硬件，比如 PI0 现在要用作 GPIO 来驱动 LED 灯，那么就不能将 PI0 作为其他功能，比如你在设计硬件的时候就不能再将 PI0 作为 LCD_G5。
### 检查 GPIO 占用
* 上一小节只是检查了一下，PI0 这个引脚有没有被复用为多个设备，本节我们将 PI0 复用为GPIO。因为我们是在 ST 官方提供的设备树上修改的，因此还要检查一下当 PI0 作为 GPIO 的时候，ST 官方有没有将这个 GPIO 分配给其他设备。其实对于 PI0 这个引脚来说不会的，因为ST 官方将其复用为了 LCD_G5，所以也就不存在说将其在作为 GPIO 分配给其他设备。但是我们在实际开发中要考虑到这一点，说不定其他的引脚就会被分配给某个设备做 GPIO，而我们没有检查，导致两个设备都用这一个 GPIO，那么肯定有一个因为申请不到 GPIO 而导致驱动无法工作。所以当我们将一个引脚用作 GPIO 的时候，一定要检查一下当前设备树里面是否有其他设备也使用到了这个 GPIO，保证设备树中只有一个设备树在使用这个 GPIO。
