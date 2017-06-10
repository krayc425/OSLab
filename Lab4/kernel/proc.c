
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

extern void sleep(int milli_sec);
extern void wakeup(PROCESS*);
/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule(){
	PROCESS* p;
	int	 greatest_ticks = 0;

    //调度进程
	while (!greatest_ticks) {
		for (p = proc_table; p < proc_table + NR_TASKS; p++) {
			if (p->ticks > greatest_ticks && p->sleep >= 0) {   //当目标进程不处于睡眠状态，切换进程
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}

        //重新赋值 ticks
		if (!greatest_ticks) {
			for (p = proc_table; p < proc_table + NR_TASKS; p++) {
				p->ticks = p->priority;
			}
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
        milli_delay(1000);
		clearScreen();
	}
}

/*======================================================================*
                           sys_process_sleep
 *======================================================================*/
PUBLIC void sys_process_sleep(int mill_seconds){
	p_proc_ready->sleep = mill_seconds / 10;
}

/*======================================================================*
                           sys_process_wakeup
 *======================================================================*/
PUBLIC void sys_process_wakeup(PROCESS* p) {
	p->sleep = 0;
}

/*======================================================================*
                           sys_sem_p
 *======================================================================*/
PUBLIC void sys_sem_p(SEMAPHORE* s){
	s->value--;     //信号量值 - 1
	if (s->value < 0) {     //结果小于 0，执行 P 操作的进程被阻塞
		s->list[s->head] = p_proc_ready;    //排入 list 队列中
		s->head = (s->head + 1) % QUEUE_LENGTH;
		sleep(-10);     //让它一直睡眠状态
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
		s->tail = (s->tail + 1) % QUEUE_LENGTH;
		wakeup(p);          //并将其转换为就绪态
	}
}
