#include <rtthread.h>


int main(void)
{
	int count = 0;
    while(1){
    	rt_kprintf("%d\n",count++);
    	rt_thread_mdelay(1000);
    }
    return 0;
}
