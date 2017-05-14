//
//  call.c
//  Memory Management
//
//  Created by 宋 奎熹 on 2017/5/11.
//  Copyright © 2017年 宋 奎熹. All rights reserved.
//

#include "call.h"
#include "bottom.h"
#include <stdio.h>

void init(){
    //Initialize Memory
    for (int i = 0; i < MEMORY_SIZE; i++) {
        mem_write(0, i);
    }
//    //Initialize Disk
//    for (int i = 0; i < 4; i++) {
//        disk_save(0, i * MEMORY_SIZE, MEMORY_SIZE);
//    }
}

int read(data_unit *data, v_address address, m_pid_t pid){
    printf("read\n");
    return 1;
}

int write(data_unit data, v_address address, m_pid_t pid){
    printf("write\n");
    return 1;
}

int allocate(v_address *address, m_size_t size, m_pid_t pid){
    printf("allocate\n");
    
    return 1;
}

int free(v_address address, m_pid_t pid){
    printf("free\n");
    return 1;
}
