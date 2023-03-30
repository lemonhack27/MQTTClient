# CMake 入门与进阶
## cmake 简介
* cmake 并不直接编译、构建出最终的可执行文件或库文件，它允许开发者编写一种与平台无关的 CMakeLists.txt 文件来制定整个工程的编译流程，cmake 工具会解析 CMakeLists.txt 文件语法规则，再根据当前的编译平台，生成本地化的 Makefile 和工程文件，最后通过 make 工具来编译整个工程；所以由此可知，cmake 仅仅只是根据不同平台生成对应的 Makefile，最终还是通过 make工具来编译工程源码，但是 cmake 却是跨平台的。

### 使用 out-of-source 方式构建
* 在工程目录下创建一个 build 目录，然后进入到 build 目录下执行 cmake：
```
cd build/
cmake ../
make
```
* 这样 cmake 生成的中间文件以及 make 编译生成的可执行文件就全部在 build 目录下了，如果要清理工程，直接删除 build 目录即可，这样就方便多了。

### 将生成的可执行文件和库文件放置到单独的目录下
* 将库文件存放在 build 目录下的 lib 目录中，而将可执行文件存放在 build 目录下的 bin 目录中，将 src 目录下的 CMakeList.txt 文件进行修改，如下所示：
```
include_directories(${PROJECT_SOURCE_DIR}/libhello)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_executable(hello main.c)
target_link_libraries(hello libhello)
```
* 然后再对 libhello 目录下的 CMakeList.txt 文件进行修改，如下所示：
```
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
add_library(libhello hello.c)
set_target_properties(libhello PROPERTIES OUTPUT_NAME "hello")
```
* 修改完成之后，再次按照步骤对工程进行构建、编译，此时便会按照我们的要求将生成的可执行文件hello 放置在 build/bin 目录下、库文件 libhello.a 放置在 build/lib 目录下。最终的目录结构就如下所示：
```
├── build
│ ├── bin
│ │ └── hello
│ └── lib
│ └── libhello.a
├── CMakeLists.txt
├── libhello
│ ├── CMakeLists.txt
│ ├── hello.c
│ └── hello.h
└── src
  ├── CMakeLists.txt
  └── main.c
```
* 其实实现这个需求非常简单，通过对 LIBRARY_OUTPUT_PATH 和 EXECUTABLE_OUTPUT_PATH变量进行设置即可完成 ； EXECUTABLE_OUTPUT_PATH 变量控制可执行文件的输出路径，而LIBRARY_OUTPUT_PATH 变量控制库文件的输出路径。

## CMakeLists.txt 语法规则
### 简单地语法介绍
 __注释__
* 在 CMakeLists.txt 文件中，使用“#”号进行单行注释

__命令（command）__
* 通常在 CMakeLists.txt 文件中，使用最多的是命令。命令的使用方式有点类似于 C 语言中的函数，因为命令后面需要提供一对括号，并且通常需要我们提供参数，多个参数使用空格分隔而不是逗号“,”，这是与函数不同的地方。命令的语法格式如下所示：
```
command(参数 1 参数 2 参数 3 ...)
```
* 不同的命令所需的参数不同，需要注意的是，参数可以分为必要参数和可选参数（通常称为选项），很多命令都提供了这两类参数，必要参数使用<参数>表示，而可选参数使用[参数]表示，譬如 set 命令：
```
set(<variable> <value>... [PARENT_SCOPE])
```
* set 命令用于设置变量，第一个参数<variable>和第二个参数<value>是必要参数，在参数列表（…表示参数个数没有限制）的最后可以添加一个可选参数 PARENT_SCOPE（PARENT_SCOPE 选项），既然是可选的，那就不是必须的，根据实际使用情况确定是否需要添加。
* 在 CMakeLists.txt 中，命令名不区分大小写，可以使用大写字母或小写字母书写命令名，譬如：
```
project(HELLO) #小写
PROJECT(HELLO) #大写
```
* 这俩的效果是相同的，指定的是同一个命令，并没区别；这个主要看个人喜好，个人喜欢用小写字母，主要是为了和变量区分开来，因为 cmake 的内置变量其名称都是使用大写字母组成的。

__变量（variable）__
* 在 CMakeLists.txt 文件中可以使用变量，使用 set 命令可以对变量进行设置，譬如：
```
# 设置变量 MY_VAL
set(MY_VAL "Hello World!")
```
* 上例中，通过 set 命令对变量 MY_VAL 进行设置，将其内容设置为"Hello World!"；通过${MY_VAL}方式来引用变量
### 部分常用命令
```
add_executable 可执行程序目标
add_library 库文件目标
add_subdirectory 去指定目录中寻找新的 CMakeLists.txt 文件
aux_source_directory 收集目录中的文件名并赋值给变量
cmake_minimum_required 设置 cmake 的最低版本号要求
get_target_property 获取目标的属性
include_directories 设置所有目标头文件的搜索路径，相当于 gcc 的-I 选项
link_directories 设置所有目标库文件的搜索路径，相当于 gcc 的-L 选项
link_libraries 设置所有目标需要链接的库
list 列表相关的操作
message 用于打印、输出信息
project 设置工程名字
set 设置变量
set_target_properties 设置目标属性
target_include_directories 设置指定目标头文件的搜索路径
target_link_libraries 设置指定目标库文件的搜索路径
target_sources 设置指定目标所需的源文件
```
__add_executable__
* add_executable 命令用于添加一个可执行程序目标，并设置目标所需的源文件，该命令定义如下所示：
```
add_executable(<name> [WIN32] [MACOSX_BUNDLE] [EXCLUDE_FROM_ALL] source1 [source2 ...])
```
* 该命令提供了一些可选参数，只需传入目标名和对应的源文件即可，譬如：
```
#生成可执行文件 hello
add_executable(hello 1.c 2.c 3.c)
```
* 定义了一个可执行程序目标 hello，生成该目标文件所需的源文件为 1.c、2.c 和 3.c。需要注意的是，源文件路径既可以使用相对路径、也可以使用绝对路径，相对路径被解释为相对于当前源码路径（注意，这里源码指的是 CMakeLists.txt 文件，因为 CMakeLists.txt 被称为 cmake 的源码，若无特别说明，后续将沿用这个概念！）。

__add_library__
* add_library 命令用于添加一个库文件目标，并设置目标所需的源文件，该命令定义如下所示：
```
add_library(<name> [STATIC | SHARED | MODULE]
                    [EXCLUDE_FROM_ALL]
                    source1 [source2 ...])
```
* 第一个参数 name 指定目标的名字，参数 source1…source2 对应源文件列表；add_library 命令默认生成的库文件是静态库文件，通过 SHARED 选项可使其生成动态库文件，具体的使用方法如下：
```
#生成静态库文件 libmylib.a
add_library(mylib STATIC 1.c 2.c 3.c)
#生成动态库文件 libmylib.so
add_library(mylib SHARED 1.c 2.c 3.c)
```
* 与 add_executable 命令相同，add_library 命令中源文件既可以使用相对路径指定、也可以使用绝对路径指定，相对路径被解释为相对于当前源码路径。不管是 add_executable、还是 add_library，它们所定义的目标名在整个工程中必须是唯一的，不可出现两个目标名相同的目标。

__add_subdirectory__
* add_subdirectory 命令告诉 cmake 去指定的目录中寻找源码并执行它，有点像 Makefile 的 include，其定义如下所示：
```
add_subdirectory(source_dir [binary_dir] [EXCLUDE_FROM_ALL])
```
* 参数 source_dir 指定一个目录，告诉cmake 去该目录下寻找 CMakeLists.txt文件并执行它；参数binary_dir指定了一个路径，该路径作为子源码（调用 add_subdirectory 命令的源码称为当前源码或父源码，被执行的源码称为子源码）的输出文件（cmake 命令所产生的中间文件）目录，binary_dir 参数是一个可选参数，如果没有显式指定，则会使用一个默认的输出文件目录；为了后续便于表述，我们将输出文件目录称为BINARY_DIR。
* 指定子源码的 BINARY_DIR 为 output，这里使用的是相对路径方式，add_subdirectory 命令对于相对路径的解释为：相对于当前源码的 BINARY_DIR；修改完成之后，再次进入到 build 目录下执行 cmake、make命令进行构建、编译，此时会在 build 目录下生成一个 output 目录，这就是子源码的 BINARY_DIR。设置 BINARY_DIR 可以使用相对路径、也可以是绝对路径，相对路径则是相对于当前源码的BINARY_DIR，并不是当前源码路径，这个要理解。通过 add_subdirectory 命令加载、执行一个外部文件夹中的源码，既可以是当前源码路径的子目录、也可以是与当前源码路径平级的目录亦或者是当前源码路径上级目录等等；对于当前源码路径的子目录，不强制调用者显式指定子源码的 BINARY_DIR；如果不是当前源码路径的子目录，则需要调用者显式指定BINARY_DIR，否则执行源码时会报错。

__aux_source_directory__
* aux_source_directory 命令会查找目录中的所有源文件，其命令定义如下：
```
aux_source_directory(<dir> <variable>)
```
* 从指定的目录中查找所有源文件，并将扫描到的源文件路径信息存放到<variable>变量中，譬如目录结构如下：
```
├── build
├── CMakeLists.txt
└── src
 ├── 1.c
 ├── 2.c
 ├── 2.cpp
 └── main.c
```
* CMakeCache.txt 内容如下所示：
```
# 顶层 CMakeLists.txt
cmake_minimum_required("VERSION" "3.5")
project("HELLO")
# 查找 src 目录下的所有源文件
aux_source_directory(src SRC_LIST)
message("${SRC_LIST}") # 打印 SRC_LIST 变量
```
* 由此可见，aux_source_directory 会将扫描到的每一个源文件添加到 SRC_LIST 变量中，组成一个字符串列表，使用分号“;”分隔。同理，aux_source_directory 既可以使用相对路径，也可以使用绝对路径，相对路径是相对于当前源码路径。

__get_target_property 和 set_target_properties__
* 分别用于获取/设置目标的属性。
__include_directories__
* include_directories 命令用于设置头文件的搜索路径，相当于 gcc 的-I 选项，其定义如下所示：
```
include_directories([AFTER|BEFORE] [SYSTEM] dir1 [dir2 ...])
```
* 默认情况下会将指定目录添加到头文件搜索列表（可以认为每一个 CMakeLists.txt 源码都有自己的头文件搜索列表）的最后面，可以通过设置 CMAKE_INCLUDE_DIRECTORIES_BEFORE 变量为 ON 来改变它默认行为，将目录添加到列表前面。也可以在每次调用 include_directories 命令时使用 AFTER 或 BEFORE选项来指定是添加到列表的前面或者后面。如果使用 SYSTEM 选项，会把指定目录当成系统的搜索目录。既可以使用绝对路径来指定头文件搜索目录、也可以使用相对路径来指定，相对路径被解释为当前源码路径的相对路径。
* 默认情况下，include 目录被添加到头文件搜索列表的最后面，通过 AFTER 或 BEFORE 选项可显式指定添加到列表后面或前面：
```
# 添加到列表后面
include_directories(AFTER include)
# 添加到列表前面
include_directories(BEFORE include)
```
* 当调用 add_subdirectory 命令加载子源码时，会将 include_directories 命令包含的目录列表向下传递给子源码（子源码从父源码中继承过来）。

__link_directories 和 link_libraries__
* link_directories 命令用于设置库文件的搜索路径，相当于 gcc 编译器的-L 选项；link_libraries 命令用于设置需要链接的库文件，相当于 gcc 编译器的-l 选项；命令定义如下所示：
```
link_directories(directory1 directory2 ...)
link_libraries([item1 [item2 [...]]]
               [[debug|optimized|general] <item>] ...)
```
* link_directories 会将指定目录添加到库文件搜索列表（可以认为每一个 CMakeLists.txt 源码都有自己的库文件搜索列表）中；同理，link_libraries 命令会将指定库文件添加到链接库列表。link_directories 命令可以使用绝对路径或相对路径指定目录，相对路径被解释为当前源码路径的相对路径。
* 譬如工程目录结构如下所示：
```
├── build
├── CMakeLists.txt
├── include
│ └── hello.h
├── lib
│ └── libhello.so
└── main.c
```
* 在 lib 目录下有一个动态库文件 libhello.so，编译链接 main.c 源文件时需要链接 libhello.so；CMakeLists.txt文件内容如下所示：
```
# 顶层 CMakeLists.txt
cmake_minimum_required("VERSION" "3.5")
project("HELLO")
include_directories(include)
link_directories(lib)
link_libraries(hello)
add_executable(main main.c)
```
* 库文件名既可以使用简写，也可以库文件名的全称，譬如：
```
# 简写
link_libraries(hello)
# 全称
link_libraries(libhello.so)
```
* link_libraries 命令也可以指定库文件的全路径（绝对路径 /开头），如果不是/开头，link_libraries 会认为调用者传入的是库文件名，而非库文件全路径，譬如上述 CMakeLists.txt 可以修改为下面这种方式：
```
# 顶层 CMakeLists.txt
cmake_minimum_required("VERSION" "3.5")
project("HELLO")
include_directories(include)
link_libraries(${PROJECT_SOURCE_DIR}/lib/libhello.so)
add_executable(main main.c)
```
* 与 include_directories 命令相同，当调用 add_subdirectory 命令加载子源码时，会将 link_directories 命令包含的目录列表以及 link_libraries 命令包含的链接库列表向下传递给子源码（子源码从父源码中继承过来）。

__list__
* list 命令是一个关于列表操作的命令，譬如获取列表的长度、从列表中返回由索引值指定的元素、将元素追加到列表中等等。命令定义如下：
```
list(LENGTH <list> <output variable>)
list(GET <list> <element index> [<element index> ...]
 <output variable>)
list(APPEND <list> [<element> ...])
list(FIND <list> <value> <output variable>)
list(INSERT <list> <element_index> <element> [<element> ...])
list(REMOVE_ITEM <list> <value> [<value> ...])
list(REMOVE_AT <list> <index> [<index> ...])
list(REMOVE_DUPLICATES <list>)
list(REVERSE <list>)
list(SORT <list>)
```
* 列表其实就是字符串数组（或者叫字符串列表、字符串数组）
LENGTH 选项用于返回列表长度；
GET 选项从列表中返回由索引值指定的元素；
APPEND 选项将元素追加到列表后面；
FIND 选项将返回列表中指定元素的索引值，如果未找到，则返回-1。
INSERT 选项将向列表中的指定位置插入元素。
REMOVE_AT 和 REMOVE_ITEM 选项将从列表中删除元素，不同之处在于 REMOVE_ITEM 将删除给
定的元素，而 REMOVE_AT 将删除给定索引值的元素。
REMOVE_DUPLICATES 选项将删除列表中的重复元素。
REVERSE 选项就地反转列表的内容。
SORT 选项按字母顺序对列表进行排序。

__message__
* message 命令用于打印、输出信息，类似于 Linux 的 echo 命令，命令定义如下所示：
```
message([<mode>] "message to display" ...)
```
* 可选的 mode 关键字用于确定消息的类型，如下：
```
none（无） 重要信息、普通信息
STATUS 附带信息
WARNING CMake 警告，继续处理
AUTHOR_WARNING CMake 警告（开发），继续处理
SEND_ERROR CMake 错误，继续处理，但跳过生成
FATAL_ERROR CMake 错误，停止处理和生成
DEPRECATION 如果变量 CMAKE_ERROR_DEPRECATED 或CMAKE_WARN_DEPRECATED 分别启用，则 CMake 弃用错误或警告，否则没有消息。
```
* 所以可以使用这个命令作为 CMakeLists.txt 源码中的输出打印语句，譬如：
```
# 打印"Hello World"
message("Hello World!")
```
__project__
* project 命令用于设置工程名称：
```
# 设置工程名称为 HELLO
project(HELLO)
```
* 执行这个之后会引入两个变量：HELLO_SOURCE_DIR 和 HELLO_BINARY_DIR，注意这两个变量名的前缀就是工程名称，HELLO_SOURCE_DIR 变量指的是 HELLO 工程源码目录、HELLO_BINARY_DIR 变量指的是 HELLO 工程源码的输出文件目录。
* 如果不加入 project(HELLO)命令，这两个变量是不存在的；工程源码目录指的是顶层源码所在目录，cmake 定义了两个等价的变量 PROJECT_SOURCE_DIR 和 PROJECT_BINARY_DIR，通常在 CMakeLists.txt源码中都会使用这两个等价的变量。通常只需要在顶层 CMakeLists.txt 源码中调用 project 即可！



