# CPY —— Python 风味的脚本解释器

> 这一世我重生了，我居然成了 Guido van Rossum！
> 这一世我一定要成为 Python 之父，编写出 Python。

我用c写了一个Python风味（夹带私货版）的解释器，可以像Python自带的解释器一样在命令行用，也可以执行.cpy文件
>用vscode打开.cpy，会自动识别成Python文件哦
---

## 快速开始
>
>简单版
<li>点击“Release”选项，然后下载那个压缩包，在你想要的目录下解压（一定要记得解压！！！）
<li>然后对着文件管理器的空白处右键选择“在终端中打开”即可使用

---

>调黑的低客版

你都当调黑的低客了还要我教吗？？？

## 命令与语法
>
>命令

打开REPL：

```bash
cpy
```

脚本模式

```bash
cpy example.cpy
```

>语法

语法和Python一样，不过我夹带了一点私货：
代码块的分割不用再管那诡异的缩进了！你可以像c/c++一样用花括号（{}）分割代码块

代码belike

```py
# CPY Demo —— 当前已实现的所有功能展示

# ---- 函数定义 ----
def add(a, b) {
    return a + b
}

def fact(n) {
    if n <= 1 {
        return 1
    }
    return n * fact(n - 1)
}

# ---- 变量与表达式 ----
x = 10
y = 3
print("x =", x)
print("x + y =", add(x, y))
print("x * y =", x * y)
print("x > y ?", x > y)

# ---- 字符串 ----
name = "cpy"
print("hello " + name)

# ---- if / elif / else ----
score = 85
if score >= 90 {
    print("A")
}
elif score >= 80 {
    print("B")
}
elif score >= 60 {
    print("C")
}
else {
    print("D")
}

# ---- while循环 ----
i = 0
while i < 3 {
    print("loop", i)
    i = i + 1
}

# ---- 逻辑运算 ----
a = True
b = False
if a and not b {
    print("logic works!")
}

# ---- 递归+函数调用 ----
print("fact(5) =", fact(5))

# ---- input+类型转换 ----
name = input("your name: ")
print("hi " + name)

```

---

## 内置函数

内置了6个函数

| 函数 | 说明 |
|------|------|
| `print(x_1 x_2 ...)` | 打印，参数间用空格分隔，末尾换行 |
| `input(prompt)` | 先打印，再读取一行用户输入，prompt是提示，**注意input返回值是str！！！** |
| `len(s)` | 输出字符串长度 |
| `int(x)` | 转换x为整数 |
| `str(x)` | 转换x为字符串 |
| `type(x)` | 打印x的类型 |

注意现在input还有一点小bug（prompt不能是中文，不做类型检查）

---

## 架构

```
3步：
.cpy
->Lexer。tokenizer分词，把代码语句分成一个个token。同时去掉注释和多余的空行（不过我并没有用到tokenizer，而是用的while循环去拆。话说c有好用的tokenizer吗？）
->Parser。把拆出来的token拼成有向树（ast）（搞半天还要自己拼）。这部分用了递归下降算法。ast主要描述了选择结构和循环结构
->Interpreter。遍历执行ast，主要执行四种节点：AST_IF选择分支，AST_WHILE循环分支，AST_BINARY算数分支，AST_CALL调用函数
```

---

## 目录结构

```
be_Guido_van_Rossum/
├── src/
│   ├── main.c          入口（REPL + 文件执行）
│   ├── lexer.c/h       词法分析
│   ├── token.h          Token 类型定义
│   ├── parser.c/h      递归下降解析器
│   ├── ast.c/h          AST 节点
│   ├── value.c/h        运行时值类型
│   ├── env.c/h          符号表（哈希表 + 链式作用域）
│   └── interpreter.c/h  解释器 + 内置函数
├── examples/
│   └── hello.cpy
├── build.bat            编译脚本
└── readme.md
```

---

## todolist

目前只做了一个v0.1的demo，虽然已经是图灵完备的了但是还有很多Python的内容没有添加，比如oop和高级数据结构。（import估计要下辈子才可能吧）
>已知的bug:目前print和input的prompt都不支持中文的显示（ansi编码老师，我还记得你😭），以及input没有做类型转换

## 鸣谢

感谢deepseek，在我想要做cpy之后告诉我解释器都需要什么🙏不然光是ast就够我赤的了。同时感谢离散数学，原来离散数学的用处在这里吗

## 许可

MIT — 这一世你是 Guido van Rossum。
