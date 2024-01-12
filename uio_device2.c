/*
 * This module creates a user input/output device: https://www.kernel.org/doc/html/v4.13/driver-api/uio-howto.html
 * This type of device gives the possibility to handle a fake or virtual interrupt on the device created.
 * In fact, real interrupts are only handled on real devices.
 * To compile the module and the user app type: make
 * After what you can insert it by doing: sudo insmod uio_device.ko
 * Before inserting an uio device, you have to load the uio module: sudo modprobe uio
 * If everything is ok, this will create the device at /dev/uio0
 * You can then launch the user app with: sudo ./uio_user "/dev/uio0"
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/uio_driver.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/pid.h>

#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/timer.h>

#include <linux/interrupt.h>
#include <linux/preempt.h>
#include <asm-generic/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TP8");
MODULE_DESCRIPTION("UIO driver");

#define MAXPID 10

static struct uio_info* info;
static struct platform_device *pdev;
static struct preempt_notifier notifier;
static struct task_struct *task_scheduled_in;
static int count;

void *shared_mem_buffer = NULL;
unsigned long *fill_buffer = NULL;//this array will be mapped to the shared_mm_buff to fill it

static int    pidarray[MAXPID] = { 0,0,0,0,0,0,0,0,0,0 };
module_param_array(pidarray, int, NULL, 0);
MODULE_PARM_DESC(pidarray, "The list of vcpu-thread to be tracked.");

/* When releasing the module, method to unregister the uio device. */
void unregister_uio(void)
{
    if (info) {
        pr_info("Unregistering uio\n");
        uio_unregister_device(info);
        kfree(info);
    }
}

static int uio_device_open(struct uio_info *info, struct inode *inode)
{
    pr_info("%s called\n", __FUNCTION__);
    return 0;
}

static int uio_device_release(struct uio_info *info, struct inode *inode)
{
    pr_info("%s called\n", __FUNCTION__);
    return 0;
}

int uio_hotplug_mmap(struct uio_info *info, struct vm_area_struct *vma){
    unsigned long len, pfn;
    int ret ;

    len = vma->vm_end - vma->vm_start;
    pfn = virt_to_phys((void *)shared_mem_buffer)>>PAGE_SHIFT;

    ret = remap_pfn_range(vma, vma->vm_start, pfn, len, vma->vm_page_prot);
    if (ret < 0) {
        pr_err("could not map the address area\n");
        kfree(shared_mem_buffer);
        return -EIO;
    }

    pr_info("memory map called success \n");

    return ret;
}

/*****************************************************************************/
static void sched_in_notify(struct preempt_notifier *notifier, int cpu_curr)
{
    struct proccs *proc;
    task_scheduled_in = current; printk("sched in - current : %d\n", task_scheduled_in->pid);

    fill_buffer[0] = ktime_get_ns();
    fill_buffer[1] = task_scheduled_in->pid;
    fill_buffer[2] = 0;
    if ((char*)fill_buffer - (char*)shared_mem_buffer > 4093 && (char*)fill_buffer - (char*)shared_mem_buffer < 4093)
    {
      fill_buffer = (unsigned long *) shared_mem_buffer;
    }
    else
    {
      fill_buffer += 3;
    }
    uio_event_notify(info);
}

static void sched_out_notify(struct preempt_notifier *notifier, struct task_struct *next)
{
    fill_buffer[0] = ktime_get_ns(); printk("sched out - current : %d\n", current->pid);
    fill_buffer[1] = current->pid;
    fill_buffer[2] = 1;
    uio_event_notify(info);
}

static struct preempt_ops operations = {
    .sched_in = sched_in_notify,
    .sched_out = sched_out_notify,
};

/**
 * preempt_notifier_register_process - tell me when a process (task) is being preempted & rescheduled
 * @notifier: notifier struct to register
 */
void preempt_notifier_register_process(struct preempt_notifier *notifier, struct task_struct *task)
{printk("task %d\n", task->pid);
	hlist_add_head(&notifier->link, &task->preempt_notifiers);
}

/**
 * preempt_notifier_register_process - tell me when a process (task) is being preempted & scheduled_out
 * @notifier: notifier struct to register
 */
void preempt_notifier_unregister_process(struct task_struct *task)
{
    struct hlist_head lh = task->preempt_notifiers;
	hlist_del(lh.first);
}

/**
 * find_process_by_pid - find a process with a matching PID value.
 * @pid: the pid in question.
 *
 * The task of @pid, if found. %NULL otherwise.
 */
struct pid_namespace *task_active_pid_ns(struct task_struct *tsk)
{
	return ns_of_pid(task_pid(tsk));
}

struct task_struct *find_task_by_pid_ns(pid_t nr, struct pid_namespace *ns)
{
	RCU_LOCKDEP_WARN(!rcu_read_lock_held(),
			 "find_task_by_pid_ns() needs rcu_read_lock() protection");
	return pid_task(find_pid_ns(nr, ns), PIDTYPE_PID);
}

struct task_struct *find_task_by_vpid(pid_t vnr)
{
	return find_task_by_pid_ns(vnr, task_active_pid_ns(current));
}

static struct task_struct *find_process_by_pid(pid_t pid)
{
	return pid ? find_task_by_vpid(pid) : current;//we needed to export this function from the core-kernel (think of other functions exported)
}
/*****************************************************************************/

static int __init uio_device_init(void)
{
    int i = 0;
    struct task_struct *task;

    count = 1;
    pdev = platform_device_register_simple("uio_device",
                                            0, NULL, 0);
    if (IS_ERR(pdev)) {
        pr_err("Failed to register platform device.\n");
        return -EINVAL;
    }

    info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);

    if (!info)
        return -ENOMEM;

    shared_mem_buffer = (void *) kzalloc(PAGE_SIZE, GFP_KERNEL);
    if (!shared_mem_buffer) {
        pr_err("not enough memory\n");
        return -ENOMEM;
    }
    fill_buffer = (unsigned long *) shared_mem_buffer;

    info->name = "uio_driver";
    info->mmap = uio_hotplug_mmap;
    info->version = "0.1";
    info->mem[0].addr = (phys_addr_t) shared_mem_buffer;// Allocate memory that can be accessed by the user
    if (!info->mem[0].addr)
        goto uiomem;
    info->mem[0].memtype = UIO_MEM_LOGICAL;
    info->mem[0].size = PAGE_SIZE;
    info->irq = UIO_IRQ_CUSTOM;
    info->handler = 0;
    info->open = uio_device_open;
    info->release = uio_device_release;

    if(uio_register_device(&pdev->dev, info)) {
        pr_err("Unable to register UIO device!\n");
        goto devmem;
    }

    preempt_notifier_inc();
    preempt_notifier_init(&notifier, &operations);

    /* Parse the array in argument to get the pids */
    while( pidarray[i] ){
        printk("pidarray[%d] = %d\n", i, pidarray[i]);
        rcu_read_lock();
        task = find_process_by_pid(pidarray[i]);
        if(task)
            preempt_notifier_register_process(&notifier, task);
        rcu_read_unlock();
        i++;
    }

    pr_info("UIO device loaded\n");
    return 0;

devmem:
uiomem:
    kfree(info);
    return -ENODEV;
}

static void __exit uio_device_cleanup(void)
{
   int k = 0;
   struct task_struct *proc;
   unregister_uio();

   if (pdev)
        platform_device_unregister(pdev);

   if (shared_mem_buffer)
        kfree(shared_mem_buffer);

   while( pidarray[k] ){
        rcu_read_lock();
        proc = find_process_by_pid(pidarray[k]);
        if(proc)
            preempt_notifier_unregister_process(proc);
        rcu_read_unlock();
        k++;
    }

   preempt_notifier_dec();

   pr_info("Cleaning up module.\n");
}

module_init(uio_device_init);
module_exit(uio_device_cleanup);
