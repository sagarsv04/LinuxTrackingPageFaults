/*
 *  pf_probe_A.c
 *  Contains implementation of kernel module to prints the virtual addresses that cause page faults to the system log.
 *
 *  Author :
 *  Sagar Vishwakarma (svishwa2@binghamton.edu)
 *  State University of New York, Binghamton
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sagar Vishwakarma");
MODULE_DESCRIPTION("A Simple Linux Page Faults Tracking Device");
MODULE_VERSION("1.0");

#define PROBE_DEBUG 0

#define PROBE_NAME "pf_probe_a"

#define PROBE_STR_LEN 128
#define PROBE_BUFFER_SIZE	500
#define MAX_SYMBOL_LEN	64
// static char symbol[MAX_SYMBOL_LEN] = "_do_fork";
static char symbol[MAX_SYMBOL_LEN] = "pf_probe_a";

static pid_t process_id = -1;
// static int probe_buffer_counter = 0;
// static int probe_position = 0;
static int probe_open_counter = 0;
static int major_number;

typedef struct page_fault_data {
	unsigned long address;
	long time;
} page_fault_data;


module_param(process_id, int, 0);
// module_param_string(PROBE_NAME, PROBE_NAME, sizeof(PROBE_NAME), 0644);
module_param_string(symbol, symbol, sizeof(symbol), 0644);

static page_fault_data page_fault_data_buffer[PROBE_BUFFER_SIZE];



/* Function Declarations */
static int handler_pre(struct kprobe *, struct pt_regs *);
static void handler_post(struct kprobe *, struct pt_regs *, unsigned long);
static int handler_fault(struct kprobe *, struct pt_regs *, int);


static int dev_open(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static int dev_close(struct inode *, struct file *);

static void get_fault_info(char *, loff_t *);


/* For each probe you need to allocate a kprobe structure */
static struct kprobe dev_kp = {
	.symbol_name	= PROBE_NAME,
	.pre_handler = handler_pre,
	.post_handler = handler_post,
	.fault_handler = handler_fault,
};


static struct file_operations dev_file_op = {
	.owner		= THIS_MODULE,
	.open			= dev_open,
	.read			= dev_read,
	.release	= dev_close,
};


/* Pass fault Info based on how many lines already read */
static void get_fault_info(char *message, loff_t *offset) {

	int skip_node = (int)(*offset);

	if (skip_node >= PROBE_BUFFER_SIZE) {
		if (PROBE_DEBUG) {
			printk(KERN_ALERT "DEV Module: Read All Buffer Entry\n");
		}
		strcpy(message, "EXIT_CODE\n");
	}
	else {
		// strcpy(message, page_fault_data_buffer[skip_node])
		sprintf(message, "PID = %8d  Page Fault at Address 0x%lx at Time %ld\n", process_id, page_fault_data_buffer[skip_node].address, page_fault_data_buffer[skip_node].time);
		*offset += 1;
	}
}


/* file_operations open implementation */
static int dev_open(struct inode *pinode, struct file *pfile) {

	pid_t pid;
	struct task_struct *task = current;
	pid = task->pid;
	probe_open_counter += 1;
	printk(KERN_INFO "DEV Module: Process %d has called %s function of Dev Page Fault Driver\n", pid, __FUNCTION__);
	if (PROBE_DEBUG) {
		printk(KERN_INFO "DEV Module: Device opened %d Times\n", probe_open_counter);
	}
	return 0;
}


/* file_operations close implementation */
static int dev_close(struct inode *pinode, struct file *pfile) {

	pid_t pid;
	struct task_struct *task = current;
	pid = task->pid;
	probe_open_counter -= 1;
	printk(KERN_INFO "DEV Module: Process %d has called %s function of Dev Page Fault Driver\n", pid, __FUNCTION__);
	if (PROBE_DEBUG) {
		printk(KERN_INFO "DEV Module: Device opened %d Times\n", probe_open_counter);
	}
	return 0;
}


/* file_operations read implementation, copy info to user space */
static ssize_t dev_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) {

	int errors = 0;
	char message[PROBE_STR_LEN];
	int message_len = 0;
	pid_t pid;

	pid = current->pid;
	get_fault_info(message, offset);
	message_len = strlen(message);
	errors = copy_to_user(buffer, message, message_len);
	if (PROBE_DEBUG) {
		printk(KERN_INFO "DEV Module: Process %d has called %s function with Offset %lld\n", pid, __FUNCTION__, *offset);
		if (errors != 0) {
			printk(KERN_INFO "DEV Module: Failed to Copy Fault Info to Process %d with Offset %lld\n", pid, *offset);
		}
	}
	return errors == 0 ? message_len : -EFAULT;
}


/* kprobe pre_handler: called just before the probed instruction is executed */
static int handler_pre(struct kprobe *p, struct pt_regs *regs) {

	#ifdef CONFIG_X86
		pr_info("<%s> CONFIG_X86 pre_handler: p->addr = 0x%p, ip = %lx, flags = 0x%lx\n",
							p->symbol_name, p->addr, regs->ip, regs->flags);
	#endif

	#ifdef CONFIG_PPC
		pr_info("<%s> CONFIG_PPC pre_handler: p->addr = 0x%p, nip = 0x%lx, msr = 0x%lx\n",
							p->symbol_name, p->addr, regs->nip, regs->msr);
	#endif

	#ifdef CONFIG_MIPS
		pr_info("<%s> CONFIG_MIPS pre_handler: p->addr = 0x%p, epc = 0x%lx, status = 0x%lx\n",
							p->symbol_name, p->addr, regs->cp0_epc, regs->cp0_status);
	#endif

	#ifdef CONFIG_ARM64
		pr_info("<%s> CONFIG_ARM64 pre_handler: p->addr = 0x%p, pc = 0x%lx, pstate = 0x%lx\n",
							p->symbol_name, p->addr, (long)regs->pc, (long)regs->pstate);
	#endif

	#ifdef CONFIG_S390
		pr_info("<%s> CONFIG_S390 pre_handler: p->addr, 0x%p, ip = 0x%lx, flags = 0x%lx\n",
							p->symbol_name, p->addr, regs->psw.addr, regs->flags);
	#endif
	/* A dump_stack() here will give a stack backtrace */
	return 0;
}


/* kprobe post_handler: called after the probed instruction is executed */
static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags) {

	#ifdef CONFIG_X86
		pr_info("<%s> CONFIG_X86 post_handler: p->addr = 0x%p, flags = 0x%lx\n",
							p->symbol_name, p->addr, regs->flags);
	#endif

	#ifdef CONFIG_PPC
		pr_info("<%s> CONFIG_PPC post_handler: p->addr = 0x%p, msr = 0x%lx\n",
							p->symbol_name, p->addr, regs->msr);
	#endif

	#ifdef CONFIG_MIPS
		pr_info("<%s> CONFIG_MIPS post_handler: p->addr = 0x%p, status = 0x%lx\n",
							p->symbol_name, p->addr, regs->cp0_status);
	#endif

	#ifdef CONFIG_ARM64
		pr_info("<%s> CONFIG_ARM64 post_handler: p->addr = 0x%p, pstate = 0x%lx\n",
							p->symbol_name, p->addr, (long)regs->pstate);
	#endif

	#ifdef CONFIG_S390
		pr_info("<%s> CONFIG_S390 post_handler: p->addr, 0x%p, flags = 0x%lx\n",
							p->symbol_name, p->addr, regs->flags);
	#endif
}


/*
 * fault_handler: this is called if an exception is generated for any
 * instruction within the pre- or post-handler, or when Kprobes
 * single-steps the probed instruction.
 */
static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr) {

	pr_info("fault_handler: p->addr = 0x%p, trap #%dn", p->addr, trapnr);
	/* Return 0 because we don't handle the fault. */
	return 0;
}


static int __init pf_probe_init(void) {

	int ret;

	major_number = register_chrdev(0, PROBE_NAME, &dev_file_op);
	if (major_number < 0) {
		printk(KERN_ALERT "DEV Module: Failed to register a major number\n");
		return major_number;
	}
	else {
		printk(KERN_INFO "DEV Module: Dev Page Fault Driver Registered with major number %d\n", major_number);
	}

	ret = register_kprobe(&dev_kp);
	if (ret < 0) {
		printk(KERN_ALERT "DEV Module: Register KPROBE Failed Return Code %d\n", ret);
		return ret;
	}
	printk(KERN_ALERT "DEV Module: Registered KPROBE for PID %d at Address %p\n", process_id, dev_kp.addr);
	if (PROBE_DEBUG) {
		printk(KERN_INFO "DEV Module: %s Installed ...\n", PROBE_NAME);
	}
	return 0;
}


static void __exit pf_probe_exit(void) {

	unregister_chrdev(major_number, PROBE_NAME);
	unregister_kprobe(&dev_kp);
	pr_info("kprobe at %p unregistered\n", dev_kp.addr);
	if (PROBE_DEBUG) {
		printk(KERN_INFO "%s Module: Removed ...\n", PROBE_NAME);
	}
}


module_init(pf_probe_init);
module_exit(pf_probe_exit);
