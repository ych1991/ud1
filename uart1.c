 #include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <mach/mx28.h>
#include <asm/cacheflush.h>
#include <mach/hardware.h>
#include <mach/device.h>
#include <mach/dmaengine.h>     

#define IRQ_TIMER2	50
#define IRQ_TIMER1  49
#define IRQ_AUART0 112
#define HW_UARTAPP_BASE  (0x8006a000)
 
#define TIME_BASE       (0x80068000)	
#define UARTAPP_CTRL1     (0x10)
#define UARTAPP_CTRL2     (0x20)
#define UARTAPP_LINECTRL  (0x30)
#define UARTAPP_LINECTRL2 (0x40)
#define UARTAPP_INTR      (0x50)
#define UARTAPP_DATA      (0x60)
#define UARTAPP_STAT      (0x70)
#define UARTAPP_DEBUG     (0x80)
#define UARTAPP_VERSION   (0x90)
#define UARTAPP_AUTOBAUD  (0xa0)
#define TIM2CTRL          (0xa0)
#define FIXED2_COUNT      (0xc0)
#define TIM1CTRL		  (0x60)
#define FIXED1_COUNT      (0x80)
#define TIME_CT           (0x80040080)


static void __iomem *Uartviraddr;
static void __iomem *Timeviraddr;
static char*rs,*ws;
static char*rbuf,*wbuf;
static void Config_UARTAPP_CTRL2( );
static void Config_UARTAPP_LINECTRL( );
static void uart_read( );
static void uart_write( );
static unsigned long recvstat;
static unsigned long trasstat;


static irqreturn_t t2read_interrupt(int irq, void *dev_id )
{  
    writel(0x8000, Timeviraddr+TIM2CTRL+0x08); //清除中断
     while(((readl(Uartviraddr + UARTAPP_STAT)&(1<<24)) ==0))
	  	{
	  		printk("DATA:%c\n",readb(Uartviraddr +UARTAPP_DATA));
	  	}

		
	 /* while((readl(Uartviraddr + UARTAPP_STAT)&(1<<25)) == 0)
	    {//判断写fifo
		  writeb( (char)i,(Uartviraddr +UARTAPP_DATA));
		 i++;
	   // printk("UARTAPP_STAT3:%08x\n",readl(Uartviraddr +UARTAPP_STAT));
		}*/
		return IRQ_HANDLED; 
}

static irqreturn_t err_irq(int irq,void *dev_id )
{
   writel(0x400,Uartviraddr + UARTAPP_INTR+0x08);
   printk("DATA1:%c\n",readb(Uartviraddr +UARTAPP_DATA));
   return IRQ_HANDLED;
}


static void Config_Time1( )
{
	writel(0x404b,(Timeviraddr + TIM1CTRL));
     writel(0x41,(Timeviraddr + FIXED1_COUNT));
}

static void Config_Time2( )
{
	writel(0x404b,(Timeviraddr + TIM2CTRL));
    writel(0x20,(Timeviraddr + FIXED2_COUNT));// 1ms中断 0x20
}

static void Config_UARTAPP_CTRL2( ) 
{
   writel(0x00030000,Uartviraddr);
   writel(0x00250b01,(Uartviraddr + UARTAPP_CTRL2));
   //writel(0x00500000,(Uartviraddr + UARTAPP_INTR));
}

static void Config_UARTAPP_LINECTRL( ) //波特率、FIFO
{
   writel(0x02710aee,(Uartviraddr +UARTAPP_LINECTRL));
}

static void Config_UARTAPP_INTR ( )
{
   (*(volatile unsigned long *) (Uartviraddr + UARTAPP_INTR)) |= (1<<20); //Receive Interrupt Enable.
   (*(volatile unsigned long *) (Uartviraddr + UARTAPP_INTR)) |= (1<<21); //Transmit Interrupt Enable.
   (*(volatile unsigned long *) (Uartviraddr + UARTAPP_INTR)) |= (1<<26);
}

static int  uart_init(void)
{  
   
   Uartviraddr = ioremap(HW_UARTAPP_BASE,0x100);
   Timeviraddr = ioremap(TIME_BASE,0x100);
   Config_UARTAPP_CTRL2 ( );
   Config_UARTAPP_LINECTRL( );
   Config_UARTAPP_INTR( );
   Config_Time1( );
   Config_Time2( );
  
    request_irq(IRQ_TIMER2,t2read_interrupt,IRQF_DISABLED | IRQF_TIMER,"t1uart0_read",NULL);
    //request_irq(IRQ_AUART0,err_irq,IRQF_DISABLED,"uart0_err",NULL);
  
	/*while(1)
		{
			//printk("UARTAPP_CTRL0:%08x\n",readl(Uartviraddr));
   			//printk("UARTAPP_CTRL1:%08x\n",readl(Uartviraddr +UARTAPP_CTRL1));
   			//printk("UARTAPP_CTRL2:%08x\n",readl(Uartviraddr +UARTAPP_CTRL2));
   			//printk("UARTAPP_LINECTRL:%08x\n",readl(Uartviraddr +UARTAPP_LINECTRL));
   			//printk("UARTAPP_LINECTRL2:%08x\n",readl(Uartviraddr +UARTAPP_LINECTRL2));
   			printk("UARTAPP_INTR:%08x\n",readl(Uartviraddr +UARTAPP_INTR));
   		//	printk("UARTAPP_DATA:%08x\n",readl(Uartviraddr +UARTAPP_DATA));
   			printk("UARTAPP_STAT:%08x\n",readl(Uartviraddr +UARTAPP_STAT));
   			//printk("UARTAPP_DEBUG:%08x\n",readl(Uartviraddr +UARTAPP_DEBUG));
   			//printk("UARTAPP_VERSION:%08x\n",readl(Uartviraddr +UARTAPP_VERSION));
   			//printk("UARTAPP_AUTOBAUD:%08x\n",readl(Uartviraddr +UARTAPP_AUTOBAUD));
	            // printk("UARTAPP_STAT:%08x\n",readl(Uartviraddr +UARTAPP_STAT));	
        	   // mdelay(1000);
	    }*/
  
   
 return 0;
}

static void uart_exit(void)
{
  free_irq(IRQ_TIMER2,NULL);
  //free_irq(IRQ_AUART0,NULL);
  printk("over");
}

MODULE_LICENSE("GPL");
module_init(uart_init);
module_exit(uart_exit);








