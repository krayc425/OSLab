
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
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

extern void enable_irq(int irq);
extern void milli_delay(int milli_sec);
extern void disp_color_int(int input, int color);
extern void strcpy(char*, char*);
PUBLIC void clearScreen();

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main(){
    clearScreen();

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
    
    //初始化进程表
    //每次从 TASK 结构中读取不同任务的入口地址、堆栈栈顶和进程名，然后赋给相应的进程表项。
	for (i = 0; i < NR_TASKS; i++) {
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid
        
		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	proc_table[0].ticks = proc_table[0].priority =  0;
	proc_table[1].ticks = proc_table[1].priority =  0;
	proc_table[2].ticks = proc_table[2].priority =  0;
	proc_table[3].ticks = proc_table[3].priority =  0;
    proc_table[4].ticks = proc_table[4].priority =  0;

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table;
    
    /**
     *  Modified Here
     */
	waiting			= 0;
	number			= 0;

	customers.value = 0;
	customers.head  = 0;
	customers.tail  = 0;

	barbers.value 	= 0;
	barbers.head  	= 0;
	barbers.tail  	= 0;

	mutex.value 	= 1;
	mutex.head  	= 0;
	mutex.tail  	= 0;
    
    init_clock();
    
	restart();

	while(1){}
}

void clearScreen(){
    int i = 0;
    disp_pos = 0;
    for(i = 0; i < 80 * 25; i++){
        disp_str(" \0");
    }
    disp_pos = 0;
	currentLineNum = 0;
}

/*======================================================================*
                            Sleep Barber Problem
 *======================================================================*/

void come(int customer){
    disp_color_int(customer, 0x07);
    disp_color_str_1(" come\n\0", 0x07);
}

void haircut(int customer){
	disp_color_int(customer, 0x06);
	disp_color_str_1(" cut\n\0", 0x06);
}

void full(int customer){
	disp_color_int(customer, 0x02);
	disp_color_str_1(" full\n\0", 0x02);
}

void customer(){
	int temp;
	while(1) {
		sem_p(&mutex);					//进入临界区
		number++;                       //顾客编号加1
		temp = number;                  //暂时记录现在的顾客是几号
		if (waiting < CHAIRS) {			//判断是否有空椅子
			waiting++;					//等待顾客加1
			come(temp);                 //打印来了哪个顾客
			sem_v(&customers);			//唤醒理发师
			sem_v(&mutex);				//退出临界区
			sem_p(&barbers);			//理发师忙，顾客坐着等待
			haircut(temp);              //给顾客剪头发
		} else {
			sem_v(&mutex);				//退出临界区
			full(temp);                 //人满了，顾客离开
        }
		process_sleep(1000);
	}
}

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA(){
	while (1) {
//        milli_delay(100);
	}
}

/*======================================================================*
                               TestB（理发师）
 *======================================================================*/
void TestB(){
	while(1){
		sem_p(&customers);				//判断是否有顾客，若无顾客，理发师睡眠
		sem_p(&mutex);					//若有顾客，进入临界区
		waiting--;						//等待顾客数减1
		sem_v(&barbers);				//理发师准备为顾客理发
        //理发师正在理发（非临界区）
        sem_v(&mutex);                    //退出临界区
		disp_color_str_1("Cut\n\0", 0x04);
		process_sleep(2000);
		disp_color_str_1("Done\n\0", 0x04);
	}
}

/*======================================================================*
                               TestC（顾客）
 *======================================================================*/
void TestC(){
	customer();
}

/*======================================================================*
                               TestD（顾客）
 *======================================================================*/
void TestD(){
	customer();
}

/*======================================================================*
                               TestE（顾客）
 *======================================================================*/
void TestE(){
	customer();
}
