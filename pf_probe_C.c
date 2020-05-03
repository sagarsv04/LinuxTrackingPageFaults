/*
 *  pf_probe_C.c
 *  Contains implementation of kernel module which writes up to three separate scatter plots - one for page faults in the user's code segment, one for page faults in the user's data segment, and one for page faults that are not in either the code segment or the data segment.
 *
 *  Author :
 *  Sagar Vishwakarma (svishwa2@binghamton.edu)
 *  State University of New York, Binghamton
 */

 #include <linux/kernel.h>
 #include <linux/module.h>
 #include <linux/kprobes.h>
 #include <linux/uaccess.h>
 #include <linux/ktime.h>
 #include <linux/memory.h>
 #include <linux/memcontrol.h>
 #include <linux/proc_fs.h>

 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("Sagar Vishwakarma");
 MODULE_DESCRIPTION("A Simple Linux Page Faults Tracking Device");
 MODULE_VERSION("1.0");

 #define PROBE_DEBUG 0

 #define PROBE_NAME "pf_probe_C"

 #define PROBE_STR_LEN 128
 #define PROBE_BUFFER_SIZE	500
 #define MAX_SYMBOL_LEN	64


 static pid_t process_id = 0;
 static int probe_open_counter = 0;
 static int probe_ret = -2;
 static int data_buffer_idx = -1;
 struct proc_dir_entry *dev_file_entry;


 typedef struct page_fault_data {
 	unsigned long address;
 	long time;
 } page_fault_data;


 static char symbol[MAX_SYMBOL_LEN] = "handle_mm_fault";
 static page_fault_data page_fault_data_buffer[PROBE_BUFFER_SIZE];


 module_param(process_id, int, 0);
 module_param_string(symbol, symbol, sizeof(symbol), 0644);


 /* Function Declarations */
 static int handler_pre(struct kprobe *, struct pt_regs *);
 static void handler_post(struct kprobe *, struct pt_regs *, unsigned long);
 static int handler_fault(struct kprobe *, struct pt_regs *, int);


 static int dev_open(struct inode *, struct file *);
 static int dev_close(struct inode *, struct file *);
 static ssize_t dev_read(struct file *, char *, size_t, loff_t *);


 static void get_fault_info(char *, loff_t *);
 static void dev_cleanup(void);


 static struct kprobe dev_kp = {
 	.symbol_name			= symbol,
 	.pre_handler			= handler_pre,
 	.post_handler			= handler_post,
 	.fault_handler		= handler_fault,
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
 		sprintf(message, "PID = %8d Page Fault at Address 0x%lx at Time %ld\n", process_id, page_fault_data_buffer[skip_node].address, page_fault_data_buffer[skip_node].time);
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

 	if (PROBE_DEBUG) {
 		printk(KERN_INFO "DEV Module: Process %d has called %s function with Offset %lld\n", pid, __FUNCTION__, *offset);
 	}

 	pid = current->pid;
 	get_fault_info(message, offset);
 	message_len = strlen(message);
 	errors = copy_to_user(buffer, message, message_len);
 	if (errors != 0) {
 		printk(KERN_INFO "DEV Module: Failed to Copy Fault Info to Process %d with Offset %lld\n", pid, *offset);
 	}
 	return errors == 0 ? message_len : -EFAULT;
 }


 /* kprobe pre_handler: called just before the probed instruction is executed */
 static int handler_pre(struct kprobe *p, struct pt_regs *regs) {

 	struct timespec current_time;

 	if (current->pid == process_id) {

 		#ifdef CONFIG_X86

 			if(data_buffer_idx == PROBE_BUFFER_SIZE-1) {
 				data_buffer_idx = 0;
 			}
 			else {
 				data_buffer_idx += 1;
 			}
 			current_time = current_kernel_time();
 			page_fault_data_buffer[data_buffer_idx].address = regs->si;
 			page_fault_data_buffer[data_buffer_idx].time = current_time.tv_nsec;
 			printk(KERN_INFO "DEV Module: <%s> pre_handler: probe->addr = 0x%p, vertual->addr = %lx\n", p->symbol_name, p->addr, regs->si);
 		#endif
 	}
 	else {
 		if (PROBE_DEBUG) {
 			printk(KERN_INFO "DEV Module: Process %d has called %s function of Dev Page Fault Driver\n", current->pid, __FUNCTION__);
 		}
 	}
 	/* A dump_stack() here will give a stack backtrace */
 	return 0;
 }


 /* kprobe post_handler: called after the probed instruction is executed */
 static void handler_post(struct kprobe *p, struct pt_regs *regs, unsigned long flags) {

 	if (current->pid == process_id) {
 		#ifdef CONFIG_X86
 			printk(KERN_INFO "DEV Module: <%s> post_handler: p->addr = 0x%p, flags = 0x%lx\n", p->symbol_name, p->addr, regs->flags);
 		#endif
 	}
 	else {
 		if (PROBE_DEBUG) {
 			printk(KERN_INFO "DEV Module: Process %d has called %s function of Dev Page Fault Driver\n", current->pid, __FUNCTION__);
 		}
 	}
 }


 /* fault_handler: this is called if an exception is generated for any instruction within the pre- or post-handler */
 static int handler_fault(struct kprobe *p, struct pt_regs *regs, int trapnr) {

 	if (current->pid == process_id) {
 		#ifdef CONFIG_X86
 			printk(KERN_ALERT "DEV Module: <%s> fault_handler: p->addr = 0x%p, trap #%dn\n", p->symbol_name, p->addr, trapnr);
 		#endif
 	}
 	else {
 		printk(KERN_INFO "DEV Module: Process %d has called %s function of Dev Page Fault Driver\n", current->pid, __FUNCTION__);
 		if (PROBE_DEBUG) {
 			;
 		}
 	}
 	/* Return 0 because we don't handle the fault. */
 	return 0;
 }


 static void dev_cleanup(void) {

 	if (dev_file_entry != NULL) {
 		remove_proc_entry(PROBE_NAME,NULL);
 		printk(KERN_INFO "DEV Module: Removed File Entry : /proc/%s\n", PROBE_NAME);
 	}

 	if (probe_ret >= 0) {
 		unregister_kprobe(&dev_kp);
 		printk(KERN_ALERT "DEV Module: Probe at %p Unregistered\n", dev_kp.addr);
 	}
 }


 static int __init pf_probe_init(void) {

 	dev_file_entry = proc_create(PROBE_NAME, 0, NULL, &dev_file_op);
 	if (dev_file_entry == NULL) {
 		printk(KERN_ALERT "DEV Module: Failed to Create File Entry for %s\n", PROBE_NAME);
 		return -EFAULT;
 	}
 	else {
 		printk(KERN_INFO "DEV Module: Created File Entry : /proc/%s, for User Space Program\n", PROBE_NAME);
 	}

 	probe_ret = register_kprobe(&dev_kp);
 	if (probe_ret < 0) {
 		printk(KERN_ALERT "DEV Module: Register Probe Failed Return Code %d\n", probe_ret);
 		dev_cleanup();
 		return probe_ret;
 	}
 	else {
 		printk(KERN_ALERT "DEV Module: Registered Probe for PID %d at Address %p\n", process_id, dev_kp.addr);
 		if (PROBE_DEBUG) {
 			printk(KERN_INFO "DEV Module: %s Probe Installed ...\n", PROBE_NAME);
 		}
 	}
 	return 0;
 }


 static void __exit pf_probe_exit(void) {
 	dev_cleanup();
 	if (PROBE_DEBUG) {
 		printk(KERN_INFO "%s Module: Removed ...\n", PROBE_NAME);
 	}
 }


 module_init(pf_probe_init);
 module_exit(pf_probe_exit);
