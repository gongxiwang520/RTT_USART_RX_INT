#include <rtthread.h>

#define SAMPLE_UART_NAME        "uart2"

// 用于接收消息的信号量
static struct rt_semaphore rx_sem;
static rt_device_t serial;

/* 接收数据回调函数 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
  rt_sem_release(&rx_sem);

  return RT_EOK;
}

static void serial_thread_entry(void *parameter)
{
  char ch;

  while (1)
  {// 从串口读取一个字节数据,没有读取到则等待接收信号量
    while (rt_device_read(serial, -1, &ch, 1) != 1)
    {// 阻塞等待接收信号量,等到信号量后再次读取数据
      rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
    }

    // 接收到数据加1输出
    ch = ch + 1;
    rt_device_write(serial, 0, &ch, 1);
  }
}

// 命令调用格式：uart_sample uart2
static int uart_sample(int argc, char *argv[])
{
  rt_err_t ret = RT_EOK;
  char uart_name[RT_NAME_MAX];
  char str[] = "hello RT-Thread!\r\n";

  if (argc == 2)
  {// 如果命令有2个参数,则第2个参数为设备名
    rt_strncpy(uart_name, argv[1], RT_NAME_MAX);
  }
  else
  {// 否则设备名使用默认名称
    rt_strncpy(uart_name, SAMPLE_UART_NAME, RT_NAME_MAX);
  }

  // 查找系统中的串口设备
  serial = rt_device_find(uart_name);
  if (!serial)
  {
    rt_kprintf("find %s failed!\n", uart_name);
    return RT_ERROR;
  }
  
  // 初始化信号量
  rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);

  // 以中断接收 及 轮询发送模式打开串口设备
  rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);

  // 设置接收回调函数
  rt_device_set_rx_indicate(serial, uart_input);

  // 发送字符串
  rt_device_write(serial, 0, str, (sizeof(str) - 1));

  // 创建serial线程
  rt_thread_t thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 1024, 25, 10);

  if (thread != RT_NULL)
  {
    rt_thread_startup(thread);
  }
  else
  {
    ret = RT_ERROR;
  }

  return ret;
}

// 测试该命令只能发1次,否则会提示rt_object_init函数出错
MSH_CMD_EXPORT(uart_sample, uart device sample)

