# synology-index-gui

.c编写：裙下孤魂

## 界面

![screenshots](https://github.com/tbc0309/synolgy-index-gui/raw/main/demo.png)

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

index2024.c.zip：增加注释颜色，支持打开WEB管理界面

![index2024.c.zip](https://github.com/user-attachments/assets/92a880c5-3aad-4f30-97b9-37907ec7e20b)
