/*
 * File      : interrupt.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2018, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include <rthw.h>
#include <riscv-plic.h>
#include <metal/machine.h>
#include <interrupt.h>

#define MCAUSE_INT      0x80000000
#define MCAUSE_CAUSE    0x7FFFFFFF

#define MAX_HANDLERS    __METAL_PLIC_SUBINTERRUPTS

/* exception and interrupt handler table */
static struct rt_irq_desc irq_desc[MAX_HANDLERS];

/**
 * This function will mask a interrupt.
 * @param vector the interrupt number
 */
void rt_hw_interrupt_mask(int irq)
{
    __plic_irq_disable(irq);
}

/**
 * This function will un-mask a interrupt.
 * @param vector the interrupt number
 */
void rt_hw_interrupt_unmask(int irq)
{
    __plic_irq_enable(irq);
    __plic_set_priority(irq, 1);
}

rt_isr_handler_t rt_hw_interrupt_handle(rt_uint32_t vector, void *param)
{
    rt_kprintf("UN-handled interrupt %d occurred!!!\n", vector);
    return RT_NULL;
}

void rt_hw_interrupt_init(void)
{
    int idx;

    /*  disable global interrupt*/
    rt_uint32_t sourse;
    for(sourse = 1; sourse < MAX_HANDLERS; sourse++){
        __plic_irq_disable(sourse);
        __plic_set_priority(sourse,0);
    }
    __plic_set_threshold(0);

    /* init exceptions table */
    for (idx = 0; idx < MAX_HANDLERS; idx++)
    {
        rt_hw_interrupt_mask(idx);
        irq_desc[idx].handler = (rt_isr_handler_t)rt_hw_interrupt_handle;
        irq_desc[idx].param = RT_NULL;
#ifdef RT_USING_INTERRUPT_INFO
        rt_snprintf(irq_desc[idx].name, RT_NAME_MAX - 1, "default");
        irq_desc[idx].counter = 0;
#endif
    }


    //disable sw interrupt
    //write_csr(mip,0UL);
    clear_csr(mie, METAL_LOCAL_INTERRUPT_SW);
    // enable machine external interrupt 
    set_csr(mie, METAL_LOCAL_INTERRUPT_EXT);

}

rt_uint32_t rt_hw_interrupt_get_active(rt_uint32_t fiq_irq)
{
    return (rt_uint32_t)__plic_irq_claim();
}

void rt_hw_interrupt_ack(rt_uint32_t fiq_irq, rt_uint32_t id)
{
    __plic_irq_complete(id);
}

/**
 * This function will install a interrupt service routine to a interrupt.
 * @param vector the interrupt number
 * @param handler the interrupt service routine to be installed
 * @param param the interrupt service function parameter
 * @param name the interrupt name
 * @return old handler
 */
rt_isr_handler_t rt_hw_interrupt_install(int vector, rt_isr_handler_t handler,
        void *param, const char *name)
{
    rt_isr_handler_t old_handler = RT_NULL;

    if(vector < MAX_HANDLERS)
    {
        old_handler = irq_desc[vector].handler;
        if (handler != RT_NULL)
        {
            irq_desc[vector].handler = (rt_isr_handler_t)handler;
            irq_desc[vector].param = param;
#ifdef RT_USING_INTERRUPT_INFO
            rt_snprintf(irq_desc[vector].name, RT_NAME_MAX - 1, "%s", name);
            irq_desc[vector].counter = 0;
#endif
        }
    }

    return old_handler;
}

/**
 * This function will be call when external machine-level 
 * interrupt from PLIC occurred.
 */
void handle_m_ext_interrupt(void)
{
    rt_isr_handler_t isr_func;
    rt_uint32_t irq;
    void *param;

    /* get irq number */
    irq = rt_hw_interrupt_get_active(0);

    /* get interrupt service routine */
    isr_func = irq_desc[irq].handler;
    param = irq_desc[irq].param;

    /* turn to interrupt service routine */
    isr_func(irq, param);
    rt_hw_interrupt_ack(0, irq);

#ifdef RT_USING_INTERRUPT_INFO
    irq_desc[irq].counter ++;
#endif
}

#ifdef USE_PLIC
extern void handle_m_ext_interrupt();
#endif

#ifdef USE_M_TIME
extern void handle_m_time_interrupt();
#endif

#ifdef USE_LOCAL_ISR
typedef void (*interrupt_ptr_t) (void);
extern interrupt_ptr_t vector_table[];
#endif

uintptr_t handle_trap(uintptr_t _mcause, uintptr_t _epc)
{
	rt_uint32_t mcause, epc;
	mcause = _mcause;
	epc = _epc;
  if (0){
#ifdef USE_PLIC
    // External Machine-Level interrupt from PLIC
  } else if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE) == METAL_INTERRUPT_ID_EXT)) {
    handle_m_ext_interrupt();
    return epc;
#endif
#ifdef USE_M_TIME
    // Timer Machine-Level interrupt
  } else if ((mcause & MCAUSE_INT) && ((mcause & MCAUSE_CAUSE) == METAL_INTERRUPT_ID_TMR)){
	//rt_kprintf("into m timer int");
    handle_m_time_interrupt();
    return epc;
#endif
#ifdef USE_LOCAL_ISR
  } else if (mcause & MCAUSE_INT) {
    vector_table[mcause & MCAUSE_CAUSE] ();
#endif
  }
  //else{
  else if(mcause & MCAUSE_INT){
    rt_kprintf("Unhandled Trap:%d\n",mcause);
    _exit(mcause);
  }
  return epc;
}
