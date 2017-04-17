#include <stdio.h>

void print(char c) {
    //将字符c转化成两位的16进制表示
    int m = c;
    int high = 0x000000f0, low = 0x0000000f;
    high &= m;
    high >>= 4;
    low &= m;
    printf("%1X%1X", high, low);
}

int main(){
	FILE *fd = fopen("/Users/Kray/Documents/Software Engineering/操作系统/实验/OSLab/Lab2/a.img", "rb+");
    if (fd == NULL) {
        printf("failed to open img!\n");
        return 0;
    }
    fseek(fd, 0, SEEK_SET); //SEEK_SET表示文件头
    char buffer[512] = { 0 };
    fread(buffer, 512, 1, fd);  //读取512个字节
    int i = 1;
    //暂时调用 c 的格式化输出
    for (; i <= 512; i++) {
        print(buffer[i - 1]);   //调用函数格式化输出
        //增加间距提高可读性
        if (i % 16 == 0) {
            printf("\n");
        }
        else if (i % 8 == 0) {
            printf("    ");
        }
        else if (i % 4 == 0) {
            printf("  ");
        }
        else if (i % 2 == 0) {
            printf(" ");
        }
    }
    fclose(fd);
    return 0;
}