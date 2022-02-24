# synolgy-index-gui

## 编译工具
apt install gcc-7-aarch64-linux-gnu

apt install gcc-aarch64-linux-gnu

apt install gcc

## 编译x86_64和armv8
gcc -o index888.cgi indexv2022.c

aarch64-linux-gnu-gcc -o index666.cgi indexv2022.c

## 使用
package\ui目录内：

images #图标文件夹

config #桌面图标配置

gettoken.html #ui引导加载

index.cgi #ui运行文件

## 须知
indexv2022.c：修改配置后需要重启套件，适合批量通用

indexv2021.c：修改配置和可以重新启动套件，单独修改
