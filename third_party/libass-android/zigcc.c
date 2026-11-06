/* zig cc 的单文件包装：meson 的 native 编译器入口必须是无参前缀的单一可执行。
 * 命令串不加引号——本机路径（C:/re-tools/...）全部无空格，cmd /c 直接可用；
 * 若未来路径带空格，这里要换成 CreateProcess 而不是 system()。 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    (void)argc;
    static char cmd[60000];
    snprintf(cmd, sizeof(cmd), "C:/re-tools/zig-windows-x86_64-0.13.0/zig.exe cc");
    for (int i = 1; i < argc; ++i) {
        size_t len = strlen(cmd);
        snprintf(cmd + len, sizeof(cmd) - len, " %s", argv[i]);
    }
    return system(cmd) != 0 ? 1 : 0;
}
