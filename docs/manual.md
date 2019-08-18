#+TITLE: Koala Language Manual
#+AUTHOR: James Zhu
#+EMAIL: zhuguangxiang@163.com

* Introduction
** 项目模式
*** 编译
1. koala -c a/b/foo
2. 设置项目路径环境变量KOALA_PATH
3. 如果是编译目录，则
4. 如果是编译单个文件，则
5. 不能出现foo.kl和foo目录存在同一个目录下。
6. 不支持使用相对导入方式导入包
*** 执行
1. koala a/b/foo
2. 查找foo.klc文件，如果找到了，则执行此文件。
3. 如果foo.klc没有找到，则查找其源文件foo.kl，
3.1. 如果foo.kl源文件存在，则检查当前目录是否有__package__.kl文件，
如果__package__.kl存在，则报错，不允许单独运行包内部单个文件。
如果__package__.kl不存在，则编译foo.kl，生成foo.klc文件，并执行此文件。
3.2. 如果foo.kl源文件存在，并且存在同名的foo目录，则报错，存在相同的包。
3.3. 如果foo.kl源文件不存在，则检查当前目录是否有相同的目录，
如果foo目录存在，则编译此目录(此目录下必须存在__package__.kl文件)，在当前目录下生成foo.klc。
如果foo目录下不存在__package__.kl文件，则报错，找不到需要执行的foo可执行文件或者源码文件。
4. 包描述文件__package__.kl默认是空的，如果有包依赖so文件，则可以在这里调用sys.load来优先
加载依赖的底层包。如果包中各个模块之间有顺序依赖关系，也可以在这里定义和调整顺序。
5. 如果一个目录下没有__package__.kl文件，则此目录下的各个源文件都是一个独立的包。
*** 包(源码)管理
1. koala package manager(kpm)
2. 使用git等源码管理软件下载包，并且安装此包到本地文件系统中。
3. 下载后可以在setup.kl文件中进行编译此包，或者延时到需要时由系统自动编译。
** 脚本模式
1. 源码以#!/usr/bin/koala开始，告诉shell去调用koala。
2. 检查当前目录下是否有__package__.kl文件和是否有相同目录名(不包含后缀.kl), 如果存在，
则无法执行，原因和执行过程相同。
3. 如果不存在，则编译此源文件。foo --> foo.klc; foo.kl --> foo.kl.klc
4. 自运行'项目模式'
** 交互模式
1. 内存模式，编译的字节码存放在内存中，并执行代码块。
2. 编译的输入流来自标准输入，而不是文件。
** 字节和字符
1. 单个字节可以赋值给字节. 'a', '\xfa', 不能超过255, 不能是CJK等多字节的字符.
2. 是一个无符号整数0-255. var b byte = 138(0x6b, 'A', '\x6b')
3. 'a', '\xfa'默认是字符而不是字节.
4. 字符支持unicode的字符, '汉'也是一个字符, 其unicode为6c49.
5. unicode: '\uxxxx', '\Uxxxxxxxx', '\u6c49'表示一个'汉'字
byte:
uint8_t value
char:
uint16_t value
1. 字符串以双引号表示, "hello, world", "汉字", 字符串是一个字符数组, 可以转化为字节数组.
2. "\u4e2d\u56fd\u4eba" "\U00004e2d\U000056fd\U00004eba" "中国人" 前者机器认识，后者人类认识.
3. 如果需要打印字符串，则需要转化为字节数组才能打印.
string:
char chars[]
** 编码：
koala内部采用utf-16编码，源文件以utf-8格式进行解析.
对于字面字符串或者字符需要注意源文件编码格式.对于使用unicode的表示方式则不受源文件编码格式影响。
unicode -> char array (enocde)
char array -> unicode (decode)

b = '\u6c49'表示一个字符
b = "\u6c49" 表示一个字符串
b = "\u6c49\u8bed" 表示一个字符串, 字符数组, 或者字节数组.
** 转义字符：
\\
\'
\"
\a
\b
\f
\n
\r
\t
\v
* programming style
** naming
- variable and function
  The names of variables (including function parameters) and struct members are all lowercase, with underscores between words.
- struct and typedef
  Type names start with a capital letter and have a capital letter for each new word, with no underscores.
  struct tag name is identical with typedef alias name.
- function pointer definition
  typedef void (*FooFunc)(void);
- function callback definition
  void _foo_cb_()
- foo_init and foo_free are pair.
- foo_intialize and foo_destroy are pair.
- comment
  use multi line comment
  comment in up line or right of line.
- left brace:
  struct or if, switch, while case are right of line.
  functions are in new line.
- file:
  xxxyy.c[h] xxx-yy.c[h] xxx_yy.c[h]