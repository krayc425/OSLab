//
//  page.h
//  Memory Management
//
//  Created by 宋 奎熹 on 2017/5/11.
//  Copyright © 2017年 宋 奎熹. All rights reserved.
//

#ifndef page_h
#define page_h

#include "type.h"
#include "bottom.h"

#define PAGE_SIZE 4 * 1024
#define MAX_PAGE_NUM MEMORY_SIZE / PAGE_SIZE

struct Page_private;
struct Page{
    int pageNum;                //页号
    int pid;                    //占用的进程号
    data_unit data[PAGE_SIZE];  //数据
    p_address address;         //开始地址
    
    int (*readPage)(struct Page*);
    
    struct Page_private *priv;
};
typedef struct Page Page;

int writePage();

#endif /* page_h */
