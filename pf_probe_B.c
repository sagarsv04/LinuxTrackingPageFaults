/*
 *  pf_probe_B.c
 *  Contains implementation of kernel module that stores the time and address of each page fault related to a specific process.
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
#define PROBE_PRINT 0 // off while submiting the code
#define CONT_STORE 0

#define PROBE_NAME "pf_probe_B"

#define PROBE_STR_LEN 128
#define PROBE_BUFFER_SIZE	1000
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
static void dev_print_chart(void);
static int find_nearest_index(long *, long, int);
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
	if (PROBE_PRINT) {
		printk(KERN_INFO "DEV Module: Process %d has called %s function of Dev Page Fault Driver\n", pid, __FUNCTION__);
	}
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
	if (PROBE_PRINT) {
		printk(KERN_INFO "DEV Module: Process %d has called %s function of Dev Page Fault Driver\n", pid, __FUNCTION__);
	}
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

	// struct timespec current_time;
	ktime_t current_time;

	if (current->pid == process_id) {

		#ifdef CONFIG_X86
			// current_time = current_kernel_time();
			current_time = ktime_get();
			if (CONT_STORE) {
				if(data_buffer_idx == PROBE_BUFFER_SIZE) {
					data_buffer_idx = 0;
				}
				else {
					page_fault_data_buffer[data_buffer_idx].address = regs->si;
					// page_fault_data_buffer[data_buffer_idx].time = current_time.tv_nsec;
					page_fault_data_buffer[data_buffer_idx].time = (long)ktime_to_ns(current_time);
					data_buffer_idx += 1;
				}
			}
			else {
				if(data_buffer_idx != PROBE_BUFFER_SIZE) {
					page_fault_data_buffer[data_buffer_idx].address = regs->si;
					// page_fault_data_buffer[data_buffer_idx].time = current_time.tv_nsec;
					page_fault_data_buffer[data_buffer_idx].time = (long)ktime_to_ns(current_time);
					data_buffer_idx += 1;
				}
			}
			if (PROBE_PRINT) {
				printk(KERN_INFO "DEV Module: <%s> pre_handler:   pid = %8d, vertual->addr = %lx, time = %ld\n", p->symbol_name, current->pid, regs->si, (long)ktime_to_ns(current_time));
			}
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
			if (PROBE_PRINT) {
				printk(KERN_INFO "DEV Module: <%s> post_handler:  pid = %8d, vertual->addr = %lx, flags = 0x%lx\n", p->symbol_name, current->pid, regs->si, regs->flags);
			}
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
			if (PROBE_PRINT) {
				printk(KERN_ALERT "DEV Module: <%s> fault_handler: pid = %8d, vertual->addr = %lx, trap #%dn\n", p->symbol_name, current->pid, regs->si, trapnr);
			}
		#endif
	}
	else {
		if (PROBE_DEBUG) {
			printk(KERN_INFO "DEV Module: Process %d has called %s function of Dev Page Fault Driver\n", current->pid, __FUNCTION__);
		}
	}
	/* Return 0 because we don't handle the fault. */
	return 0;
}


static int find_nearest_index(long *array, long target, int range) {

	int near_index = 0;
	long near_diff = abs(array[0] - target);
	int idx;
	for (idx = 1; idx < range; idx++) {
	  // Is the difference from the target value for this entry smaller than the currently recorded one?
	  if (abs(array[idx] - target) < near_diff) {
	    // Yes, so save the index number along with the current difference.
	    near_index = idx;
	    near_diff = abs(array[idx] - target);
	  }
	}
	return near_index;
}



static void dev_print_chart(void) {

	char char_array[30][71];
	char char_x_axis[71];
	char addr_str[30];
	long addr_array[30];
	long time_array[70];
	long max_addr_lng;
	long min_addr_lng;
	long addr_lng;
	long addr_scale;
	long time_scale;

	int row;
	int col;
	int idx;
	int jdx;
	int near_addr;
	int near_time;

	unsigned long min_address = page_fault_data_buffer[0].address;
	unsigned long max_address = page_fault_data_buffer[0].address;
	long min_time = page_fault_data_buffer[0].time;
	long max_time = page_fault_data_buffer[0].time;

	for (idx=0; idx<PROBE_BUFFER_SIZE; idx++) {
		// find max address and max time
		if (page_fault_data_buffer[idx].address > max_address) {
			max_address = page_fault_data_buffer[idx].address;
		}
		if (page_fault_data_buffer[idx].time > max_time) {
			max_time = page_fault_data_buffer[idx].time;
		}
		// find min address and min time
		if (page_fault_data_buffer[idx].address != 0) {
			if (page_fault_data_buffer[idx].address < min_address) {
				min_address = page_fault_data_buffer[idx].address;
			}
		}
		if (page_fault_data_buffer[idx].time != 0) {
			if (page_fault_data_buffer[idx].time < min_time) {
				min_time = page_fault_data_buffer[idx].time;
			}
		}
	}
	printk(KERN_ALERT "DEV Module: Hex Info :: pid = %8d, addr range = %lx - %lx, time range = %ld - %ld\n", process_id, min_address, max_address, min_time, max_time);

	sprintf(addr_str, "%lx", max_address);
	kstrtol(addr_str, 16, &max_addr_lng);

	sprintf(addr_str, "%lx", min_address);
	kstrtol(addr_str, 16, &min_addr_lng);

	printk(KERN_INFO "DEV Module: Dec Info :: pid = %8d, addr range = %ld - %ld, time range = %ld - %ld\n", process_id, min_addr_lng, max_addr_lng, min_time, max_time);

	for(idx = 0; idx < 30; idx++) {
		for(jdx = 0; jdx < 71; jdx++) {
			if (jdx == 70) {
				char_array[idx][jdx] = '\0';
			}
			else {
				char_array[idx][jdx] = ' ';
			}
		}
	}

	addr_scale = (max_addr_lng - min_addr_lng)/30;
	for (row = 0; row < 30; row++) {
		addr_array[row] = (max_addr_lng-(addr_scale*row));
	}
	time_scale = (max_time - min_time)/70;
	for (col = 0; col < 71; col++) {
		if (col == 70) {
			char_x_axis[col] = '\0';
		}
		else {
			time_array[col] = (max_time-(time_scale*col));
			char_x_axis[col] = '_';
		}
	}

	for (idx = 0; idx < PROBE_BUFFER_SIZE; idx++) {
		sprintf(addr_str, "%lx", page_fault_data_buffer[idx].address);
		kstrtol(addr_str, 16, &addr_lng);
		near_addr = find_nearest_index(addr_array, addr_lng, 30);
		near_time = find_nearest_index(time_array, page_fault_data_buffer[idx].time, 70);
		char_array[near_addr][near_time] = '*';
	}

	for(idx = 0; idx < 30; idx++) {
		printk(KERN_INFO "%20ld | %s\n", addr_array[idx], char_array[idx]);
	}
	printk(KERN_INFO "%20d # %s\n", 0, char_x_axis);
	printk(KERN_INFO "%20d # %ld\t %ld\t %ld\t %ld\t %ld\n", 0, time_array[0], time_array[15], time_array[30], time_array[50], time_array[69]);
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
	dev_print_chart();
	dev_cleanup();
	if (PROBE_DEBUG) {
		printk(KERN_INFO "%s Module: Removed ...\n", PROBE_NAME);
	}
}


module_init(pf_probe_init);
module_exit(pf_probe_exit);
