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

//偏移11个字节   Volume Boot Record  长度25字节
struct BPB{
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

//根目录条目 长度32字节
struct RootEntry{
    char DIR_Name[11];
    u8   DIR_Attr;      //文件属性
    char reserved[10];
    u16  DIR_WrtTime;
    u16  DIR_WrtDate;
    u16  DIR_FstClus;   //开始簇号
    u32  DIR_FileSize;
};

//树的节点 type = 0: 目录，1: 文件
struct TreeNode{
    int type;
    char name[100];
    int dir_count;
    int file_count;
    struct TreeNode *fatherNode;
    struct TreeNode *sonNode;       //子目录/文件
    struct TreeNode *siblingNode;   //若有>1的子目录/文件，存在这里
};
typedef struct TreeNode Node;

#pragma pack () /*取消指定对齐，恢复缺省对齐*/

//Nasm 中的打印函数
void my_print(char c);
//打印根目录
void printRoot(FILE *file, int base, struct RootEntry *rootEntry_ptr, struct TreeNode *rootNode_ptr);
//打印一般目录
void printFile(FILE *file, int clus, char *directory, Node *rootNode);
//得到 FAT
int getFATValue(FILE *file, int num);
//打印文件内容
void printFileData(FILE *file, int clus, int size);
//判断是否为非有效的字符（非数字，字母，空格）（用于文件名）
int isInValidChar(char c);
//打印文件名，type = 0: 目录，1: 文件
void printName(char *str, int type);
//判断是否为可以打印的字符
int isPrintableChar(char c);
//递归打印目录
void printRecursively(Node *rootNode_ptr);

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
    fread(bpb_ptr, 1, 25, fd);  //读取25字节（BPB）到 bpb
    
    //初始化变量
    BytsPerSec = bpb_ptr->BPB_BytsPerSec;
    SecPerClus = bpb_ptr->BPB_SecPerClus;
    RsvdSecCnt = bpb_ptr->BPB_RsvdSecCnt;
    NumFATs = bpb_ptr->BPB_NumFATs;
    RootEntCnt = bpb_ptr->BPB_RootEntCnt;
    if (bpb_ptr->BPB_FATSz16 != 0){
        FATSz = bpb_ptr->BPB_FATSz16;
    }else{
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
    
    Node rootNode;
    Node *rootNode_ptr = &rootNode;
    rootNode_ptr->type = 0;
    rootNode_ptr->dir_count = 0;
    rootNode_ptr->file_count = 0;
    rootNode_ptr->sonNode = NULL;
    rootNode_ptr->siblingNode = NULL;
    strcpy(rootNode_ptr->name, "/");
    printRoot(fd, 19 * 512, rootEntry_ptr, rootNode_ptr);
    
    printf("----------Start to print files----------\n");
    
    printRecursively(rootNode_ptr);
    
    fclose(fd);

    return 0;
}

void printRecursively(Node *rootNode_ptr){
    printf("%d directories, %d files, %s \n", rootNode_ptr->dir_count, rootNode_ptr->file_count, rootNode_ptr->name);
    if(rootNode_ptr->sonNode != NULL){
        printf("root %s son %s\n", rootNode_ptr->name, rootNode_ptr->sonNode->name);
        
        printRecursively(rootNode_ptr->sonNode);
    }else{
        return; //没孩子肯定没邻居
    }
    Node *indexNode = rootNode_ptr->sonNode->siblingNode;
    while (1) {
        if(indexNode == NULL){
            break;
        }
        printf("root %s sib %s\n", rootNode_ptr->name, indexNode->name);
        
        printRecursively(indexNode);
        
//        if(indexNode->siblingNode != NULL){
            indexNode = indexNode->siblingNode;
//        }else{
//            break;
//        }
    }
}

/**
 从 Root 的 base 处开始一个个读取 RootEntry
 */
void printRoot(FILE *file, int base, struct RootEntry *rootEntry_ptr, Node *rootNode_ptr){
    for(int i = 0; i < RootEntCnt; i++){

        fseek(file, base, SEEK_SET);
        fread(rootEntry_ptr, 1, 32, file);

        if(rootEntry_ptr->DIR_Name[0] == '\0'){ //无法读取的一个字
            continue;
        }else{
            int flag = 0;
            for (int j = 0; j < 11; j++){
                if(isInValidChar(rootEntry_ptr->DIR_Name[j])){
                    flag = 1;
                    break;
                }
            }
            //该行可以读取
            if(flag == 0){
                //提取文件名
                char filename[12];
                int realLength = 0;
                for (int k = 0; k < 11; k++) {
                    if(rootEntry_ptr->DIR_Name[k] == ' '){
                        //将第一个空格替换为.
                        filename[realLength] = '.';
                        while (rootEntry_ptr->DIR_Name[k] == ' '){
                            k++;
                        }
                        k--;
                    }else{
                        filename[realLength] = rootEntry_ptr->DIR_Name[k];
                    }
                    realLength++;
                }
                filename[realLength] = '\0';
                
                switch (rootEntry_ptr->DIR_Attr){
                    case 0x10:
                    {
                        //目录，最后一个.改成\0
                        filename[realLength - 1] = '\0';
                        
                        Node *node_ptr = (Node *)malloc(sizeof(Node));
                        node_ptr->type = 0;
                        strcpy(node_ptr->name, rootNode_ptr->name);
                        strcpy(node_ptr->name+strlen(rootNode_ptr->name), filename);
                        rootNode_ptr->dir_count++;
                        node_ptr->dir_count = 0;
                        node_ptr->file_count = 0;
                        node_ptr->fatherNode = rootNode_ptr;
                        node_ptr->sonNode = NULL;
                        node_ptr->siblingNode = NULL;
                        if(rootNode_ptr->sonNode == NULL){
                            rootNode_ptr->sonNode = node_ptr;
//                            printf("d son %s\t %s\n", rootNode_ptr->name, rootNode_ptr->sonNode->name);
                        }else{
                            if(rootNode_ptr->sonNode->siblingNode == NULL){
                                rootNode_ptr->sonNode->siblingNode = node_ptr;
//                                printf("d sib %s\t %s\n", rootNode_ptr->name, rootNode_ptr->sonNode->siblingNode->name);
                            }else{
                                Node *indexNode = rootNode_ptr->sonNode->siblingNode;
                                while (1) {
                                    if(indexNode->siblingNode == NULL){
                                        indexNode->siblingNode = node_ptr;
                                        break;
                                    }
                                    indexNode = indexNode->siblingNode;
                                }
//                                printf("d sib %s\t %s\n", rootNode_ptr->name, indexNode->siblingNode->name);
                            }
                        }
                        
                        printName(filename, 0);
                        printFile(file, rootEntry_ptr->DIR_FstClus, filename, node_ptr);
                    }
                        break;
                    case 0x20:
                    {
                        Node *node_ptr = (Node *)malloc(sizeof(Node));
                        node_ptr->type = 1;
                        strcpy(node_ptr->name, rootNode_ptr->name);
                        strcpy(node_ptr->name+strlen(rootNode_ptr->name), filename);
                        rootNode_ptr->file_count++;
                        node_ptr->dir_count = 0;
                        node_ptr->file_count = 0;
                        node_ptr->fatherNode = rootNode_ptr;
                        node_ptr->sonNode = NULL;
                        node_ptr->siblingNode = NULL;
                        if(rootNode_ptr->sonNode == NULL){
                            rootNode_ptr->sonNode = node_ptr;
//                            printf("f son %s\t %s\n", rootNode_ptr->name, rootNode_ptr->sonNode->name);
                        }else{
                            if(rootNode_ptr->sonNode->siblingNode == NULL){
                                rootNode_ptr->sonNode->siblingNode = node_ptr;
//                                printf("f sib %s\t %s\n", rootNode_ptr->name, rootNode_ptr->sonNode->siblingNode->name);
                            }else{
                                Node *indexNode = rootNode_ptr->sonNode->siblingNode;
                                while (1) {
                                    if(indexNode->siblingNode == NULL){
                                        indexNode->siblingNode = node_ptr;
                                        break;
                                    }
                                    indexNode = indexNode->siblingNode;
                                }
//                                printf("f sib %s\t %s\n", rootNode_ptr->name, indexNode->siblingNode->name);
                            }
                        }
                        
                        printName(filename, 1);
                        printFile(file, rootEntry_ptr->DIR_FstClus, filename, node_ptr);

                    }
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
void printFile(FILE *file, int clus, char *directory, Node *rootNode_ptr){
    char dir[100];
    dir[0] = '/';
    char *filename = dir + 1;
    
    int dataBase = 512 * 33;
    
    int currentClus = clus;
    int clusNum = 0x000;  //簇号
    
    while (clusNum < 0xFF8){
        //找到下一个簇号
        clusNum = getFATValue(file, currentClus);
        if (clusNum == 0xFF7) { //坏簇
            break;
        }
        
        char str[512];  //暂存从簇中读出的数据
        char *content = str;
        
        int startByte = dataBase + (currentClus - 2) * 512;
        fseek(file, startByte, SEEK_SET);
        fread(content, 1, 512, file);
        
        //解析content中的数据,依次处理各个条目,目录下每个条目结构与根目录下的目录结构相同
        int count = 512;  //每簇的字节数
        for(int i = 0; i < count; i += 32){
            
            //过滤非目标文件
            if (content[i] == '\0') {
                continue;
            }
            int flag = 0;
            for (int j = i; j < i + 11; j++){
                if(isInValidChar(content[j])){
                    flag = 1;
                    break;
                }
            }
            
            if(flag == 0){
                //提取文件名
                char tempFileName[12];
                int tempRealLength = 0;
                for (int k = 0; k < 11; k++){
                    if(content[i + k] == ' '){
                        tempFileName[tempRealLength] = '.';
                        while (content[i + k] == ' '){
                            k++;
                        }
                        k--;
                    }else{
                        tempFileName[tempRealLength] = content[i + k];
                    }
                    tempRealLength++;
                }
                tempFileName[tempRealLength] = '\0';
                       
                strcpy(filename, tempFileName);
                
                switch (content[i + 11]){
                    case 0x10:
                    {
                        dir[tempRealLength] = '\0';
                        
                        Node *node_ptr = (Node *)malloc(sizeof(Node));
                        node_ptr->type = 0;
                        strcpy(node_ptr->name, rootNode_ptr->name);
                        strcpy(node_ptr->name+strlen(rootNode_ptr->name), dir);
                        rootNode_ptr->dir_count++;
                        node_ptr->dir_count = 0;
                        node_ptr->file_count = 0;
                        node_ptr->fatherNode = rootNode_ptr;
                        node_ptr->sonNode = NULL;
                        node_ptr->siblingNode = NULL;
                        if(rootNode_ptr->sonNode == NULL){
                            rootNode_ptr->sonNode = node_ptr;
//                            printf("d son %s\t %s\n", rootNode_ptr->name, rootNode_ptr->sonNode->name);
                        }else{
                            if(rootNode_ptr->sonNode->siblingNode == NULL){
                                rootNode_ptr->sonNode->siblingNode = node_ptr;
//                                printf("d sib %s\t %s\n", rootNode_ptr->name, rootNode_ptr->sonNode->siblingNode->name);
                            }else{
                                Node *indexNode = rootNode_ptr->sonNode->siblingNode;
                                while (1) {
                                    if(indexNode->siblingNode == NULL){
                                        indexNode->siblingNode = node_ptr;
                                        break;
                                    }
                                    indexNode = indexNode->siblingNode;
                                }
//                                printf("d sib %s\t %s\n", rootNode_ptr->name, indexNode->siblingNode->name);
                            }
                        }
                        
                        char temp[100];
                        char *printDir = temp;
                        strcpy(printDir, directory);
                        strcpy(printDir+strlen(directory), dir);
                        printName(printDir, 0);
                        printFile(file, content[i + 26], dir, node_ptr);
                    }
                        break;
                    case 0x20:
                    {
                        Node *node_ptr = (Node *)malloc(sizeof(Node));
                        node_ptr->type = 1;
                        strcpy(node_ptr->name, rootNode_ptr->name);
                        strcpy(node_ptr->name+strlen(rootNode_ptr->name), dir);
                        rootNode_ptr->file_count++;
                        node_ptr->dir_count = 0;
                        node_ptr->file_count = 0;
                        node_ptr->fatherNode = rootNode_ptr;
                        node_ptr->sonNode = NULL;
                        node_ptr->siblingNode = NULL;
                        if(rootNode_ptr->sonNode == NULL){
                            rootNode_ptr->sonNode = node_ptr;
//                            printf("f son %s\t %s\n", rootNode_ptr->name, rootNode_ptr->sonNode->name);
                        }else{
                            if(rootNode_ptr->sonNode->siblingNode == NULL){
                                rootNode_ptr->sonNode->siblingNode = node_ptr;
//                                printf("f sib %s\t %s\n", rootNode_ptr->name, rootNode_ptr->sonNode->siblingNode->name);
                            }else{
                                Node *indexNode = rootNode_ptr->sonNode->siblingNode;
                                while (1) {
                                    if(indexNode->siblingNode == NULL){
                                        indexNode->siblingNode = node_ptr;
                                        break;
                                    }
                                    indexNode = indexNode->siblingNode;
                                }
//                                printf("f sib %s\t %s\n", rootNode_ptr->name, indexNode->siblingNode->name);
                            }
                        }
                        
                        char temp[100];
                        char *printDir = temp;
                        strcpy(printDir, directory);
                        strcpy(printDir+strlen(directory), dir);
                        printName(printDir, 1);
//                        printFileData(file, content[i + 26], content[i + 28]);
                    }
                        break;
                    default:
                        break;
                }
            }
        }
        clusNum += 1;
        currentClus = clusNum;
    }
}

/**
 打印文件内容
 */
void printFileData(FILE *file, int clus, int size){
    int dataBase = 512 * 33;
    int startByte = dataBase + (clus - 2) * 512;
    char content[size + 1];
    char data[size + 1];
    int realLength = 0;
    char *str = content;
    fseek(file, startByte, SEEK_SET);
    fread(content, 1, size, file);
    for(int i = 0; i < size; i++){
        if(isPrintableChar(content[i])){
            data[realLength] = content[i];
            realLength++;
        }
    }
    data[realLength] = '\0';
    printf("file content: \n%s\n", data);
}

/**
 打印文件名，包括目录和文件
 */
void printName(char *str, int type){
    printf("%s", str);
    if(type == 0){
        printf("/");
    }
    printf("\n");
}

/**
 通过 FAT 表找到文件的下一个簇号
 */
int getFATValue(FILE *file, int num){
    //FAT1的偏移字节
    int fatBase = 512;
    //FAT项的偏移字节
    int fatPos = fatBase + num * 3 / 2;
    //奇偶FAT项处理方式不同，分类进行处理，从0号FAT项开始
    int type = 0;
    if (num % 2 == 0){
        type = 0;
    } else {
        type = 1;
    }
    
    //先读出FAT项所在的两个字节
    u16 bytes;
    u16 *bytes_ptr = &bytes;
    fseek(file, fatPos, SEEK_SET);
    fread(bytes_ptr, 1, 2, file);
    
    //u16为short，结合存储的小尾顺序和FAT项结构可以得到
    //type为0的话，取byte2的低4位和byte1构成的值，type为1的话，取byte2和byte1的高4位构成的值
    if (type == 0) {
        return bytes<<4;
    } else {
        return bytes>>4;
    }
}

/**
 判断是否为数字、字母、空格
 */
int isInValidChar(char c){
    return !((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == ' ');
}

/**
 判断是否是可打印的字符
 */
int isPrintableChar(char c){
    return (c >= 32 && c <= 126) || c == '\n';
}
