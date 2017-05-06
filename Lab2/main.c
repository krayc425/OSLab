#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROMPT_START "----------Load image-----------"
#define PROMPT_PRINT "----------Print files----------"
#define PROMPT_ENTER "--------Enter commands---------"
#define PROMPT_END   "-----------Good bye------------"
#define PROMPT_NOT_FOUND "No such directory or file"
#define PROMPT_NOT_DIR "Not a directory"
#define PROMPT_UNKNOWN_CMD "Unknown command"
#define PROMPT_COMMAND ">>>>>>"

typedef unsigned char u8;   //1字节
typedef unsigned short u16; //2字节
typedef unsigned int u32;   //4字节

int BytsPerSec;    //每扇区字节数
int SecPerClus;    //每簇扇区数
int RsvdSecCnt;    //Boot记录占用的扇区数
int NumFATs;       //FAT表个数
int RootEntCnt;    //根目录最大文件数
int FATSz;         //FAT扇区数

FILE *fat12;        //读取的硬盘文件

//#pragma pack (1)    //指定按1字节对齐

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
    char DIR_Name[11];  //文件名
    u8   DIR_Attr;      //文件属性
    char reserved[10];
    u16  DIR_WrtTime;
    u16  DIR_WrtDate;
    u16  DIR_FstClus;   //文件簇号
    u32  DIR_FileSize;  //文件大小
};

//树的节点 type = 0: 目录，1: 文件
/*
 Example:
        root
        /
      son -- sibling -- sibling -- sibling
               /                     /
             son -- sibling        son
                                   /
                                 son -- sibling
*/
struct TreeNode{
    int type;                       //类型，是文件还是目录
    char name[100];                 //文件名
    int dir_count;                  //目录数量
    int file_count;                 //文件数量
    struct TreeNode *sonNode;       //子目录/文件
    struct TreeNode *siblingNode;   //若有>1的子目录/文件，存在这里
    int clus;                       //文件簇号
    int fileSize;                   //文件大小
};
typedef struct TreeNode Node;

//#pragma pack ()     //取消指定对齐，恢复缺省对齐

//Nasm 中的打印函数
void my_print_str(char *str, int type);
//打印根目录
void printRoot(int base, struct RootEntry *rootEntry_ptr, Node *rootNode_ptr);
//打印一般目录
void printFile(int clus, char *directory, Node *rootNode);
//得到 FAT 表并找到下一个簇号
int findFatValue(int num);
//打印文件内容
void printFileData(int clus, int size);
//判断是否为非有效的字符（非数字，字母，空格）（用于文件名）
int isInValidChar(char c);
//判断是否为可以打印的字符
int isPrintableChar(char c);
//打印字符串（调用 my_print_str）
void printString(char *str, int type);
//判断路径是否代表一个文件
int isFile(char *path);
//数多少个文件和目录
int countDirectoryAndFile(char *path, Node *rootNode_ptr, int fileCount);
//打印目录和文件
int printDirectoryAndFile(char *path, Node *rootNode_ptr, int fileCount);

int main(){
    //读取映像文件
   	fat12 = fopen("a.img", "rb+");
    if (fat12 == NULL) {
        return 0;
    }
    
    struct BPB bpb;
    struct BPB *bpb_ptr = &bpb;
    //移到11字节偏移处
    fseek(fat12, 11, SEEK_SET);     //SEEK_SET表示文件头
    fread(bpb_ptr, 1, 25, fat12);   //读取25字节（BPB）到 bpb
    
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
    
//    printf("BytsPerSec\t%d\n", BytsPerSec);
//    printf("SecPerClus\t%d\n", SecPerClus);
//    printf("RsvdSecCnt\t%d\n", RsvdSecCnt);
//    printf("NumFATs\t\t%d\n",  NumFATs);
//    printf("RootEntCnt\t%d\n", RootEntCnt);
//    printf("FATSz\t\t%d\n",    FATSz);
    
    //初始化根目录指针
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
    
    //开始打印文件目录
    printString(PROMPT_PRINT, 2);
    printRoot(19 * BytsPerSec, rootEntry_ptr, rootNode_ptr);
    
    //开始接受命令
    printString(PROMPT_ENTER, 2);
    while (1) {
        printString(PROMPT_COMMAND, 2);
        char command[100];
        char path[100];
        char *command_str = command;
        fgets(command, 100, stdin);
        command[strlen(command) - 1] = '\0';
        if(command[0] == 'e'){
            //exit：退出
            break;
        }else if(command[0] == 'c'){
            //count：计算目录/文件数量，如果 fileCount=0 则表示没找到
            if(countDirectoryAndFile(command + 6, rootNode_ptr, 0) == 0){
                printString(PROMPT_NOT_FOUND, 2);
            }
        }else if(command[0] == '/'){
            //直接输入路径，打印目录或者文件内容，如果 fileCount=0 则表示没找到
            if(printDirectoryAndFile(command, rootNode_ptr, 0) == 0){
                printString(PROMPT_NOT_FOUND, 2);
            }
        }else{
            printString(PROMPT_UNKNOWN_CMD, 2);
        }
    }
    
    //结束
    printString(PROMPT_END, 2);
    fclose(fat12);
    return 0;
}

/**
 * 数多少个文件和目录
 */
int countDirectoryAndFile(char *path, Node *rootNode_ptr, int fileCount){
    char tmpStr[strlen(path)];
    strcpy(tmpStr, rootNode_ptr->name);
    tmpStr[strlen(path)] = '\0';
    
    if(strcmp(path, tmpStr) == 0){
        if(isFile(path)){  
            //给的路径不是目录
            printString(PROMPT_NOT_DIR, 2);
        }else{
            if(!isFile(rootNode_ptr->name)){    
                //要输出的文件路径不是目录
                char *result = (char *)malloc(200);
                int len = 0;
                //存储目录数
                int temp = 1;
                    //转成数字
                while (rootNode_ptr->dir_count / (temp * 10) >= 1) {
                    temp *= 10;
                }
                    //存成字符串
                while (temp >= 1) {
                    result[len] = ((rootNode_ptr->dir_count / temp) % 10) + '0';
                    len++;
                    temp /= 10;
                }
                char *t = (char *)" directories and ";
                for (int i = 0; i < strlen(t); i++) {
                    result[len+i] = *(t + i);
                }
                len += strlen(t);
                //存储文件数
                temp = 1;
                    //转成数字
                while (rootNode_ptr->file_count / (temp * 10) >= 1) {
                    temp *= 10;
                }
                    //存成字符串
                while (temp >= 1) {
                    result[len] = ((rootNode_ptr->file_count / temp) % 10) + '0';
                    len++;
                    temp /= 10;
                }
                t = (char *)" files in ";
                for (int i = 0; i < strlen(t); i++) {
                    result[len+i] = *(t + i);
                }
                len += strlen(t);
                //当前目录或文件名
                t = (char *)rootNode_ptr->name;
                for (int i = 0; i < strlen(t); i++) {
                    result[len+i] = *(t + i);
                }
                result[len+strlen(t)] = '\0';
                printString(result, 2);
            }
        }
        fileCount++;
    }
    //打印孩子
    if(rootNode_ptr->sonNode != NULL){
        fileCount += countDirectoryAndFile(path, rootNode_ptr->sonNode, fileCount);
        //有孩子才可能会有邻居，打印邻居
        Node *indexNode = rootNode_ptr->sonNode->siblingNode;
        while (1) {
            if(indexNode == NULL){
                break;
            }
            fileCount += countDirectoryAndFile(path, indexNode, fileCount);
            indexNode = indexNode->siblingNode;
        }
    }
    return fileCount;
}

/**
 * 打印目录和文件
 */
int printDirectoryAndFile(char *path, Node *rootNode_ptr, int fileCount){
    char tmpStr[strlen(path)];
    strcpy(tmpStr, rootNode_ptr->name);
    tmpStr[strlen(path)] = '\0';
    
    if(strcmp(path, tmpStr) == 0){
        if(isFile(path)){   
           //文件：输出内容
            printFileData(rootNode_ptr->clus, rootNode_ptr->fileSize);
        }else{  
            //目录：打印目录名字
            printString(rootNode_ptr->name, isFile(rootNode_ptr->name));
        }
        fileCount++;
    }
    
    //打印孩子
    if(rootNode_ptr->sonNode != NULL){
        fileCount += printDirectoryAndFile(path, rootNode_ptr->sonNode, fileCount);
        //有孩子才可能会有邻居，打印邻居
        Node *indexNode = rootNode_ptr->sonNode->siblingNode;
        while (1) {
            if(indexNode == NULL){
                break;
            }
            fileCount += printDirectoryAndFile(path, indexNode, fileCount);
            indexNode = indexNode->siblingNode;
        }
    }
    return fileCount;
}

/**
 从 Root 的 base 处开始一个个读取 RootEntry
 */
void printRoot(int base, struct RootEntry *rootEntry_ptr, Node *rootNode_ptr){
    for(int i = 0; i < RootEntCnt; i++){

        //移到根目录文件起始点开始读取
        fseek(fat12, base, SEEK_SET);
        fread(rootEntry_ptr, 1, 32, fat12);

        if(rootEntry_ptr->DIR_Name[0] == '\0'){ 
            //无法读取的一个字
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
                        node_ptr->sonNode = NULL;
                        node_ptr->siblingNode = NULL;

                        node_ptr->clus = rootEntry_ptr->DIR_FstClus;
                        node_ptr->fileSize = rootEntry_ptr->DIR_FileSize;
                        
                        if(rootNode_ptr->sonNode == NULL){
                            rootNode_ptr->sonNode = node_ptr;
                        }else{
                            if(rootNode_ptr->sonNode->siblingNode == NULL){
                                rootNode_ptr->sonNode->siblingNode = node_ptr;
                            }else{
                                Node *indexNode = rootNode_ptr->sonNode->siblingNode;
                                while (1) {
                                    if(indexNode->siblingNode == NULL){
                                        indexNode->siblingNode = node_ptr;
                                        break;
                                    }
                                    indexNode = indexNode->siblingNode;
                                }
                            }
                        }
                        
                        //打印目录名
                        printString(node_ptr->name, 0);
                        //打印子目录
                        printFile(rootEntry_ptr->DIR_FstClus, filename, node_ptr);
                    }
                        break;
                    case 0x20:
                    {
                        //文件
                        Node *node_ptr = (Node *)malloc(sizeof(Node));
                        node_ptr->type = 1;
                        strcpy(node_ptr->name, rootNode_ptr->name);
                        strcpy(node_ptr->name+strlen(rootNode_ptr->name), filename);
                        rootNode_ptr->file_count++;
                        node_ptr->dir_count = 0;
                        node_ptr->file_count = 0;
                        node_ptr->sonNode = NULL;
                        node_ptr->siblingNode = NULL;
                        
                        node_ptr->clus = rootEntry_ptr->DIR_FstClus;
                        node_ptr->fileSize = rootEntry_ptr->DIR_FileSize;
                        
                        if(rootNode_ptr->sonNode == NULL){
                            rootNode_ptr->sonNode = node_ptr;
                        }else{
                            if(rootNode_ptr->sonNode->siblingNode == NULL){
                                rootNode_ptr->sonNode->siblingNode = node_ptr;
                            }else{
                                Node *indexNode = rootNode_ptr->sonNode->siblingNode;
                                while (1) {
                                    if(indexNode->siblingNode == NULL){
                                        indexNode->siblingNode = node_ptr;
                                        break;
                                    }
                                    indexNode = indexNode->siblingNode;
                                }
                            }
                        }
                        
                        //打印文件名
                        printString(node_ptr->name, 1);
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
 递归打印目录及目录中文件
 */
void printFile(int clus, char *directory, Node *rootNode_ptr){
    char dir[100];
    dir[0] = '/';
    char *filename = dir + 1;
    
    int dataBase = BytsPerSec * 33;
    
    int currentClus = clus;
    int clusNum = 0x000;  //簇号
    
    //结束的簇号
    while (clusNum < 0xFF8){
        //找到下一个簇号
        clusNum = findFatValue(currentClus);
        if (clusNum == 0xFF7) {
            //坏簇
            break;
        }
        
        //存放从簇中读出的数据
        char str[BytsPerSec];
        char *content = str;
        
        int startByte = dataBase + (currentClus - 2) * BytsPerSec;
        fseek(fat12, startByte, SEEK_SET);
        fread(content, 1, BytsPerSec, fat12);
        
        //解析content中的数据,依次处理各个条目,目录下每个条目结构与根目录下的目录结构相同
        int count = BytsPerSec;  //每簇的字节数
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
                        //目录
                        dir[tempRealLength] = '\0';
                        
                        Node *node_ptr = (Node *)malloc(sizeof(Node));
                        node_ptr->type = 0;
                        strcpy(node_ptr->name, rootNode_ptr->name);
                        strcpy(node_ptr->name+strlen(rootNode_ptr->name), dir);
                        rootNode_ptr->dir_count++;
                        node_ptr->dir_count = 0;
                        node_ptr->file_count = 0;
                        node_ptr->sonNode = NULL;
                        node_ptr->siblingNode = NULL;
                        
                        node_ptr->clus = content[i + 26];
                        
                        if(rootNode_ptr->sonNode == NULL){
                            rootNode_ptr->sonNode = node_ptr;
                        }else{
                            if(rootNode_ptr->sonNode->siblingNode == NULL){
                                rootNode_ptr->sonNode->siblingNode = node_ptr;
                            }else{
                                Node *indexNode = rootNode_ptr->sonNode->siblingNode;
                                while (1) {
                                    if(indexNode->siblingNode == NULL){
                                        indexNode->siblingNode = node_ptr;
                                        break;
                                    }
                                    indexNode = indexNode->siblingNode;
                                }
                            }
                        }
                        
                        char temp[100];
                        char *printDir = temp;
                        strcpy(printDir, directory);
                        strcpy(printDir+strlen(directory), dir);
                        
                        //打印目录名
                        printString(node_ptr->name, 0);
                        //打印子目录
                        printFile(content[i + 26], dir, node_ptr);
                    }
                        break;
                    case 0x20:
                    {
                        //文件
                        Node *node_ptr = (Node *)malloc(sizeof(Node));
                        node_ptr->type = 1;
                        strcpy(node_ptr->name, rootNode_ptr->name);
                        strcpy(node_ptr->name+strlen(rootNode_ptr->name), dir);
                        rootNode_ptr->file_count++;
                        node_ptr->dir_count = 0;
                        node_ptr->file_count = 0;
                        node_ptr->sonNode = NULL;
                        node_ptr->siblingNode = NULL;
                        
                        node_ptr->clus = content[i + 26];
                        node_ptr->fileSize = content[i + 28];
                        
                        if(rootNode_ptr->sonNode == NULL){
                            rootNode_ptr->sonNode = node_ptr;
                        }else{
                            if(rootNode_ptr->sonNode->siblingNode == NULL){
                                rootNode_ptr->sonNode->siblingNode = node_ptr;
                            }else{
                                Node *indexNode = rootNode_ptr->sonNode->siblingNode;
                                while (1) {
                                    if(indexNode->siblingNode == NULL){
                                        indexNode->siblingNode = node_ptr;
                                        break;
                                    }
                                    indexNode = indexNode->siblingNode;
                                }
                            }
                        }
                        
                        //打印文件名
                        printString(node_ptr->name, 1);
                    }
                        break;
                    default:
                        break;
                }
            }
        }
        clusNum++;
        currentClus = clusNum;
    }
}

/**
 打印文件内容
 */
void printFileData(int clus, int size){
    int dataBase = BytsPerSec * 33;
    int startByte = dataBase + (clus - 2) * BytsPerSec * SecPerClus;
    char content[size + 1];
    char data[size + 1];
    int realLength = 0;
    char *str = content;
    //移到文件开始处
    fseek(fat12, startByte, SEEK_SET);
    fread(content, 1, size, fat12);
    //一个个读取 char
    for(int i = 0; i < size; i++){
        if(isPrintableChar(content[i])){
            data[realLength] = content[i];
            realLength++;
        }
    }
    data[realLength] = '\0';
    printString(data, 2);
}

/**
 打印文件名，包括目录和文件
 */
void printString(char *str, int type){
    //调用 Nasm
    my_print_str(str, type);
}

/**
 通过 FAT 表找到文件的下一个簇号
 */
int findFatValue(int clusNum){
    //FAT1的偏移字节
    int fatBase = BytsPerSec;
    //FAT项的偏移字节
    int fatPos = fatBase + clusNum * 3 / 2; //每个簇占1.5个字节
    //奇偶FAT项处理方式不同，分类进行处理
    int type = 0;
    if (clusNum % 2 == 0){
        type = 0;
    } else {
        type = 1;
    }
    
    //先读出FAT项所在的两个字节
    u16 bytes;
    u16 *bytes_ptr = &bytes;
    fseek(fat12, fatPos, SEEK_SET);
    fread(bytes_ptr, 1, 2, fat12);
    
    //u16为short，存储的小尾顺序
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

/**
 判断路径是否代表一个文件
 */
int isFile(char *path){
    int isFile = 0;
    int i = 0;
    while (i < strlen(path)) {
        if(path[i] == '.'){
            isFile = 1;
            break;
        }
        i++;
    }
    return isFile;
}
