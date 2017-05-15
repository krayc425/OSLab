
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PUBLIC void flush(CONSOLE* p_con);

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;
    
    /**
     * Modified here
     */
    /* 第一个控制台沿用原来的光标位置 */
    p_tty->p_console->cursor = disp_pos / 2;
    disp_pos = 0;

	set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == &console_table[nr_current_console]);
}


/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch, int color)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
    //查找模式下，字符存入临时数组
    /**
     *  Modified here
     */
    if(is_search_mode == 1){
        search_arr[search_size++] = ch;
    }
    
	switch(ch) {
        case '\n':
            if (p_con->cursor < p_con->original_addr + p_con->v_mem_limit - SCREEN_WIDTH) {
                while(p_con->cursor < p_con->original_addr + SCREEN_WIDTH *
                      ((p_con->cursor - p_con->original_addr) / SCREEN_WIDTH + 1) - 1){
                    p_con->cursor+=1;
                    *p_vmem++ = '\0';
                    *p_vmem++ = color;
                }
                p_con->cursor++;
                *p_vmem++ = '\0';
                *p_vmem++ = color;
            }
            break;
        case '\b':
            if(is_search_mode == 0) {
                if (p_con->cursor > p_con->original_addr) {
                    //前面一个不是0，是正常字符，就删一个
                    if(*(p_vmem-2) != '\0'){
                        p_con->cursor--;
                        *(p_vmem-1) = color;
                        *(p_vmem-2) = ' ';
                        p_vmem-=2;
                    }else{
                        //不然就连续删0，直到遇到一个字符
                        //连续删回车：一直删0，直到遇到前面一个是到达屏幕最左端（为什么-1？因为一开始 cursor 在下一行第一个，-1以后相当于回到了上一行的最后一个，但是这样最后就会剩下一个，所以等会再删掉）
                        while(*(p_vmem-2) == '\0' && ((p_con->cursor - 1) % SCREEN_WIDTH > 0)){
                            p_con->cursor--;
                            *(p_vmem-1) = color;
                            *(p_vmem-2) = ' ';
                            p_vmem-=2;
                        }
                        //删回车时剩下一个，也删掉
                        if(*(p_vmem-2) == '\0'){
                            p_con->cursor--;
                            *(p_vmem-1) = color;
                            *(p_vmem-2) = ' ';
                            p_vmem-=2;
                        }
                    }
                }
            }
            break;
            /**
             *  Modified here
             */
        case '\t':
            if (p_con->cursor <
                p_con->original_addr + p_con->v_mem_limit - 1 - 4) {
                int positionLeft = 4 - p_con->cursor % 4;
                while(positionLeft > 0){
                    positionLeft--;
                    p_con->cursor+=1;
                    *p_vmem++ = '\0';
                    *p_vmem++ = color;
                }
            }
            break;
        default:
            if (p_con->cursor <
                p_con->original_addr + p_con->v_mem_limit - 1) {
                *p_vmem++ = ch;
                *p_vmem++ = color;
                p_con->cursor++;
            }
            break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PUBLIC void flush(CONSOLE* p_con)
{
    set_cursor(p_con->cursor);
    set_video_start_addr(p_con->current_start_addr);
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}

/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	set_cursor(console_table[nr_console].cursor);
	set_video_start_addr(console_table[nr_console].current_start_addr);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	set_video_start_addr(p_con->current_start_addr);
	set_cursor(p_con->cursor);
}

