# CMake 入门与进阶
## cmake 简介
* cmake 并不直接编译、构建出最终的可执行文件或库文件，它允许开发者编写一种与平台无关的 CMakeLists.txt 文件来制定整个工程的编译流程，cmake 工具会解析 CMakeLists.txt 文件语法规则，再根据当前的编译平台，生成本地化的 Makefile 和工程文件，最后通过 make 工具来编译整个工程；所以由此可知，cmake 仅仅只是根据不同平台生成对应的 Makefile，最终还是通过 make工具来编译工程源码，但是 cmake 却是跨平台的。

