
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"

extern void clearScreen();
extern void sleep(int);
extern void wakeup(PROCESS*);

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule(){
    /**
     *  Modified Here
     */
	PROCESS* p;
    //调度进程
	for (p = proc_table; p < proc_table + NR_TASKS; p++) {
		if (p->ticks > 0) {
            //给每个正在睡眠的进程的 ticks - 1
			p->ticks--;
		}
	}
    
    //寻找下一个进程
    while(1){
        //遍历任务表
        if(++p_proc_ready >= proc_table + NR_TASKS){
            p_proc_ready = proc_table;
        }
        //当前进程没被 block，且可以运行
        if(p_proc_ready->ticks <= 0 && p_proc_ready->block != 1){
            break;
        }
    }
}

//以下是系统调用
/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks(){
	return ticks;
}

/*======================================================================*
                           sys_disp_str
 *======================================================================*/
PUBLIC void sys_disp_str(char* str){
	disp_str(str);
}

/*======================================================================*
                           sys_disp_color_str
 *======================================================================*/
PUBLIC void sys_disp_color_str(char* str, int color){
	disp_color_str(str, color);
    //判断是否满屏要清屏
	int i = 0;
	while(1){
		if(str[i] == '\0'){
			break;
		}
		if(str[i] == '\n'){
			currentLineNum++;
		}
		i++;
	}
    if(currentLineNum == 25){
		clearScreen();
	}
}

/*======================================================================*
                           sys_process_sleep
 *======================================================================*/
PUBLIC void sys_process_sleep(int mill_seconds){
    //进程沉睡，设置 ticks 数
	p_proc_ready->ticks = mill_seconds / 1000 * HZ;
    //沉睡不分配时间片，调度下一个进程
	schedule();
}

/*======================================================================*
                           sys_process_wakeup
 *======================================================================*/
PUBLIC void sys_process_wakeup(PROCESS* p) {
    p->block = 0;
}

/*======================================================================*
                           sys_sem_p
 *======================================================================*/
PUBLIC void sys_sem_p(SEMAPHORE* s){
	s->value--;     //信号量值 - 1
	if (s->value < 0) {     //结果小于 0，执行 P 操作的进程被阻塞
		s->list[s->head] = p_proc_ready;    //排入 list 队列中
        //阻止当前进程，调度下一个进程
		p_proc_ready->block = 1;
		schedule();
        s->head = (s->head + 1) % QUEUE_LENGTH;
	}
    //否则继续执行
}

/*======================================================================*
                           sys_sem_v
 *======================================================================*/
PUBLIC void sys_sem_v(SEMAPHORE* s){
	s->value++;     //信号量值 + 1
	if (s->value <= 0) {    //结果不大于 0，执行 V 操作的进程从 list 队列中释放一个进程
		PROCESS* p = s->list[s->tail];
        wakeup(p);          //并将其转换为就绪态
		s->tail = (s->tail + 1) % QUEUE_LENGTH;
	}
}

/*======================================================================*
                              wakeup
 *======================================================================*/
PUBLIC void wakeup(PROCESS* p){     //唤醒一个进程
	process_wakeup(p);
}
