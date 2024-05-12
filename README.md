# Antlr4 环境配置

根据以下链接配置 cmake 

[cmake](https://github.com/antlr/antlr4/tree/dev/runtime/Cpp/cmake)

下载 jar 包并将其复制到 `./third_party/antlr/`

[antlr-4.13.1](https://www.antlr.org/download/antlr-4.13.1-complete.jar)


# LLVM 环境配置

```shell
brew install llvm
# 可执行文件的路径
export PATH="/opt/homebrew/Cellar/llvm/18.1.5/bin:$PATH"
# 让编译器能够找到LLVM
export LDFLAGS="-L/opt/homebrew/Cellar/llvm/18.1.5/lib"
export CPPFLAGS="-I/opt/homebrew/Cellar/llvm/18.1.5/include"
```

如果你遇到下述问题,

![img.png](docs/img/img.png)

参考[how-to-use-llvm-target-initialize-function](https://stackoverflow.com/questions/71867265/how-to-use-llvm-target-initialize-function)修改CMakeLists.txt

```cmake
set(LLVM_LINK_COMPONENTS
        ${LLVM_TARGETS_TO_BUILD}
)

llvm_map_components_to_libnames(llvm_libs Core ExecutionEngine IRReader Passes Support TransformUtils native AsmParser ${LLVM_LINK_COMPONENTS})
```

并在使用 cmake build 的时候设置参数:
```shell
cmake .. -DLLVM_TARGETS_TO_BUILD=all  
```


# 参考资料

## Antlr4 

[getting-started-antlr-cpp](https://tomassetti.me/getting-started-antlr-cpp/)

## LLVM

[MyFirstLanguageFrontend](https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/)

[writing-your-own-toy-compiler](https://gnuu.org/2009/09/18/writing-your-own-toy-compiler/)