//
//  page.c
//  Memory Management
//
//  Created by 宋 奎熹 on 2017/5/11.
//  Copyright © 2017年 宋 奎熹. All rights reserved.
//

#include "page.h"

int readPage(Page* page){
    for (int i = 0; i < PAGE_SIZE; i++) {
        page->data[i] = mem_read(page->address + i);
    }
}
