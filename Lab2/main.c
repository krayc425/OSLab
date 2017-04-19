#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;   //1字节
typedef unsigned short u16; //2字节
typedef unsigned int u32;   //4字节

int  BytsPerSec;    //每扇区字节数
int  SecPerClus;    //每簇扇区数
int  RsvdSecCnt;    //Boot记录占用的扇区数
int  NumFATs;       //FAT表个数
int  RootEntCnt;    //根目录最大文件数
int  FATSz;         //FAT扇区数

#pragma pack (1) /*指定按1字节对齐*/

//偏移11个字节   Volume Boot Record
struct BPB {
    u16  BPB_BytsPerSec;    //每扇区字节数
    u8   BPB_SecPerClus;    //每簇扇区数
    u16  BPB_RsvdSecCnt;    //Boot记录占用的扇区数
    u8   BPB_NumFATs;       //FAT表个数
    u16  BPB_RootEntCnt;    //根目录最大文件数
    u16  BPB_TotSec16;
    u8   BPB_Media;
    u16  BPB_FATSz16;       //FAT扇区数
    u16  BPB_SecPerTrk;
    u16  BPB_NumHeads;
    u32  BPB_HiddSec;
    u32  BPB_TotSec32;      //如果BPB_FATSz16为0，该值为FAT扇区数
};
//BPB至此结束，长度25字节

//根目录条目s
struct RootEntry {
    char DIR_Name[11];
    u8   DIR_Attr;      //文件属性
    char reserved[10];
    u16  DIR_WrtTime;
    u16  DIR_WrtDate;
    u16  DIR_FstClus;   //开始簇号
    u32  DIR_FileSize;
};
//根目录条目结束，32字节

#pragma pack () /*取消指定对齐，恢复缺省对齐*/

//Nasm 中的打印函数
void my_print(char c);
//打印根目录
void printRoot(FILE *file, int base, struct RootEntry *rootEntry_ptr);
//打印一般目录
void printFile(FILE *file, int clus, char *directory);
//得到 FAT
int getFATValue(FILE *fat12 , int num);
//判断是否为非有效的字符（非数字，字母，空格）
int isInValidChar(char c);

int main(){
    printf("----------Start to load image----------\n");
    
    //In CentOS
//    FILE *fd = fopen("/home/krayc425/Desktop/Link to Desktop/a.img", "rb+");
    //In macOS
	 FILE *fd = fopen("/Users/Kray/Desktop/a.img", "rb");
    if (fd == NULL) {
        printf("failed to open img!\n");
        return 0;
    }
    
    struct BPB bpb;
    struct BPB *bpb_ptr = &bpb;
    fseek(fd, 11, SEEK_SET); //SEEK_SET表示文件头
    fread(bpb_ptr, 1, 25, fd);  //读取512个字节
    
    BytsPerSec = bpb_ptr->BPB_BytsPerSec;
    SecPerClus = bpb_ptr->BPB_SecPerClus;
    RsvdSecCnt = bpb_ptr->BPB_RsvdSecCnt;
    NumFATs = bpb_ptr->BPB_NumFATs;
    RootEntCnt = bpb_ptr->BPB_RootEntCnt;
    if (bpb_ptr->BPB_FATSz16 != 0) {
        FATSz = bpb_ptr->BPB_FATSz16;
    } else {
        FATSz = bpb_ptr->BPB_TotSec32;
    }
    
    printf("BytsPerSec\t%d\n", BytsPerSec);
    printf("SecPerClus\t%d\n", SecPerClus);
    printf("RsvdSecCnt\t%d\n", RsvdSecCnt);
    printf("NumFATs\t\t%d\n",  NumFATs);
    printf("RootEntCnt\t%d\n", RootEntCnt);
    printf("FATSz\t\t%d\n",    FATSz);
    
    printf("----------Start to find files----------\n");
    
    struct RootEntry rootEntry;
    struct RootEntry *rootEntry_ptr = &rootEntry;
    
    printRoot(fd, 19 * 512, rootEntry_ptr);
    
    fclose(fd);

    return 0;
}

/**
 从 Root 的 base 处开始读取一个 RootEntry
 */
void printRoot(FILE *file, int base, struct RootEntry *rootEntry_ptr){
    for(int i = 0; i < RootEntCnt; i++){
        fseek(file, base, SEEK_SET);
        fread(rootEntry_ptr, 1, 32, file);

        if(rootEntry_ptr->DIR_Name[0] == '\0'){
            continue;
        }else{
            int flag = 0;
            for (int j = 0; j < 11; j++) {
                if(isInValidChar(rootEntry_ptr->DIR_Name[j])){
                    flag = 1;
                    break;
                }
            }
            if(flag == 0){
//                printf("-----succeeds-----\n");
                //提取文件名
                char filename[12];
                int realLength = 0;
                for (int k = 0; k < 11; k++) {
                    if(rootEntry_ptr->DIR_Name[k] == ' '){
                        filename[realLength] = '.';
                        while (rootEntry_ptr->DIR_Name[k] == ' ') {
                            k++;
                        }
                        k--;
                    }else{
                        filename[realLength] = rootEntry_ptr->DIR_Name[k];
                    }
                    realLength++;
                }
                filename[realLength] = '\0';
                
//                printf("DIR_Name\t%s\n", filename);
//                printf("DIR_FstClus\t%d\n", rootEntry_ptr->DIR_FstClus);
                
                switch (rootEntry_ptr->DIR_Attr) {
                    case 0x10:
                        filename[realLength - 1] = '\0';
                        printFile(file, rootEntry_ptr->DIR_FstClus, filename);
                        break;
                    case 0x20:
                        printf("%s\n", filename);
                        break;
                    default:
                        break;
                }
            }
        }
        //一个根目录项是32位（一个字）
        base += 32;
    }
}

/**
 递归打印目录及文件
 */
void printFile(FILE *file, int clus, char *directory){
    char dir[100];
    strcpy(dir, directory);
    dir[strlen(directory)] = '/';
    dir[strlen(directory)+1] = '\0';
    char *filename = &dir[strlen(directory) + 1];
    
    //？？？
    int dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
//    printf("database : %d\n", dataBase);
    
    int currentClus = clus;
    int clusNum = 0x000;  //簇号
    
    while (clusNum < 0xFF8) {
        //？？？
        clusNum = getFATValue(file, currentClus);
        if (clusNum == 0xFF7) {
            printf("坏簇，读取失败!\n");
            break;
        }
        
        char str[512];  //暂存从簇中读出的数据
        char *content = str;
        
        int startByte = dataBase + (currentClus - 2) * 512;
        fseek(file, startByte, SEEK_SET);
        fread(content, 1, 512, file);
        
        //解析content中的数据,依次处理各个条目,目录下每个条目结构与根目录下的目录结构相同
        int count = 512;  //每簇的字节数
        int loop = 0;
        while (loop < count) {
            
            if (content[loop] == '\0') {
                loop += 32;
                continue;
            }   //空条目不输出
            
            //过滤非目标文件
            int flag = 0;
            for (int j = loop; j < loop + 11; j++) {
                if(isInValidChar(content[j])){
                    flag = 1;
                    break;
                }
            }
            
            if(flag == 0){
                //提取文件名
                char tempFileName[12];
                int tempRealLength = 0;
                for (int k = 0; k < 11; k++) {
                    if(content[loop + k] == ' '){
                        tempFileName[tempRealLength] = '.';
                        while (content[loop + k] == ' ') {
                            k++;
                        }
                        k--;
                    }else{
                        tempFileName[tempRealLength] = content[loop + k];
                    }
                    tempRealLength++;
                }
                tempFileName[tempRealLength] = '\0';
                       
                strcpy(filename, tempFileName);
                
                switch (content[loop + 11]) {
                    case 0x10:
                        dir[strlen(directory) + tempRealLength] = '\0';
                        printf("%s\n", dir);
                        printFile(file, content[loop + 26], dir);
                        break;
                    case 0x20:
                        printf("%s\n", dir);
                        break;
                    default:
                        break;
                }
            }
            loop += 32;
        }
        
        currentClus = clusNum;
    }
}

/**
 判断是否为数字、字母、空格
 */
int isInValidChar(char c){
    return !((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == ' ');
}

//？？
int getFATValue(FILE *fat12 , int num) {
    //FAT1的偏移字节
    int fatBase = RsvdSecCnt * BytsPerSec;
    //FAT项的偏移字节
    int fatPos = fatBase + num * 3 / 2;
    //奇偶FAT项处理方式不同，分类进行处理，从0号FAT项开始
    int type = 0;
    if (num % 2 == 0) {
        type = 0;
    } else {
        type = 1;
    }
    
    //先读出FAT项所在的两个字节
    u16 bytes;
    u16* bytes_ptr = &bytes;
    int check;
    check = fseek(fat12,fatPos,SEEK_SET);
    if (check == -1)
        printf("fseek in getFATValue failed!");
    
    check = fread(bytes_ptr,1,2,fat12);
    if (check != 2)
        printf("fread in getFATValue failed!");
    
    //u16为short，结合存储的小尾顺序和FAT项结构可以得到
    //type为0的话，取byte2的低4位和byte1构成的值，type为1的话，取byte2和byte1的高4位构成的值
    if (type == 0) {
        return bytes<<4;
    } else {
        return bytes>>4;
    }
}

