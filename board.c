/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-07-24     Tanek        the first version
 * 2018-11-12     Ernest Chen  modify copyright
 * 2020-03-06	  CCHs         add support for hifive devB
 */
 
#include <stdint.h>
#include <rthw.h>
#include <rtthread.h>
#include <riscv-ops.h>

#include <metal/machine.h>
#include <metal/uart.h>

#define CONSOLE_UART	(&__metal_dt_serial_10013000.uart)

#define RTC_FREQ 		(32768UL)
#define TICK_COUNT  (RTC_FREQ / RT_TICK_PER_SECOND)

#define MTIME       (*((volatile uint64_t *)(METAL_RISCV_CLINT0_0_BASE_ADDRESS + METAL_RISCV_CLINT0_MTIME)))
#define MTIMECMP    (*((volatile uint64_t *)(METAL_RISCV_CLINT0_0_BASE_ADDRESS + METAL_RISCV_CLINT0_MTIMECMP_BASE)))

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
#define RT_HEAP_SIZE 1024
static uint32_t rt_heap[RT_HEAP_SIZE];     // heap default size: 4K(1024 * 4)
RT_WEAK void *rt_heap_begin_get(void)
{
    return rt_heap;
}

RT_WEAK void *rt_heap_end_get(void)
{
    return rt_heap + RT_HEAP_SIZE;
}
#endif

/* fixed misaligned bug for qemu */
void *__wrap_memset(void *s, int c, size_t n)
{
    return rt_memset(s, c, n);
}

/* system tick interrupt */
void handle_m_time_interrupt()
{
	//clear_csr(mie, METAL_LOCAL_INTERRUPT_TMR);
    MTIMECMP = MTIME + TICK_COUNT;
    rt_tick_increase();
    //set_csr(mie, METAL_LOCAL_INTERRUPT_TMR);
}

//systerm timer init
static void rt_hw_timer_init(void)
{
    MTIMECMP = MTIME + TICK_COUNT;

    /*  enable timer interrupt*/
    set_csr(mie, METAL_LOCAL_INTERRUPT_TMR);
}

#if defined(CONSOLE_UART)
void rt_hw_console_init()
{
	metal_uart_init(CONSOLE_UART,115200);
}

void rt_hw_console_output(const char *str)
{
	rt_size_t i, size;
	char c;
	size = rt_strlen(str);
	for (i = 0; i < size; i++){
		c = *(str+i);
		if (c == '\n') {
		        metal_uart_putc(CONSOLE_UART, '\r');
	    }
		metal_uart_putc(CONSOLE_UART, c);
	}
}

char rt_hw_console_getchar(void)
{
	int ch;
	if(metal_uart_getc(CONSOLE_UART,&ch) ==0){
		return ch;
	}
	return -1;
}

#else
void rt_hw_console_init()
{
#pragma message("No console defined");
}
#endif

/**
 * This function will initial your board.
 */
void rt_hw_board_init()
{
	//initialize console
	rt_hw_console_init();

	/* initialize hardware interrupt */
    rt_hw_interrupt_init();

	/* initialize timer0 */
	rt_hw_timer_init();

    /* Call components board initial (use INIT_BOARD_EXPORT()) */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

#if defined(RT_USING_USER_MAIN) && defined(RT_USING_HEAP)
    rt_system_heap_init(rt_heap_begin_get(), rt_heap_end_get());
#endif
}



