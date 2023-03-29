# 设备树 LED 驱动
##  实验程序编写
### 修改设备树文件
* 在根节点“/”下创建一个名为“stm32mp1_led”的子节点，打开 stm32mp157d-atk.dts 文件，在根节点“/”最后面输入如下所示内容：
```
stm32mp1_led {
    compatible = "atkstm32mp1-led";
    status = "okay";
    reg = <0X50000A28 0X04 /* RCC_MP_AHB4ENSETR */
            0X5000A000 0X04 /* GPIOI_MODER */
            0X5000A004 0X04 /* GPIOI_OTYPER */
            0X5000A008 0X04 /* GPIOI_OSPEEDR */
            0X5000A00C 0X04 /* GPIOI_PUPDR */
            0X5000A018 0X04 >; /* GPIOI_BSRR */
};
```
* 设备树修改完成以后输入如下命令重新编译一下 stm32mp157d-atk.dts：
```
make dtbs
```
* 编译的时候遇到交叉编译器的gcc找不到时候，使用以下命令配置环境
```
source /opt/st/stm32mp1/3.1-openstlinux-5.4-dunfell-mp1-20-06-24/environment-setup-cortexa7t2hf-neon-vfpv4-ostl-linux-gnueabi
```


