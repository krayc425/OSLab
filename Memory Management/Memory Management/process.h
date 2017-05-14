//
//  process.h
//  Memory Management
//
//  Created by 宋 奎熹 on 2017/5/11.
//  Copyright © 2017年 宋 奎熹. All rights reserved.
//

#ifndef process_h
#define process_h

#include <stdio.h>
#include "page.h"

struct Process{
    int pid;
    int mem_size;
    Page pages[2];
};
typedef struct Process Process;

#endif /* process_h */
