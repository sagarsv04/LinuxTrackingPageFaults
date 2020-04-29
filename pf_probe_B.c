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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sagar Vishwakarma");
MODULE_DESCRIPTION("A Simple Linux Page Faults Tracking Device");
MODULE_VERSION("1.0");

#define DRIVER_DEBUG 0

#define DRIVER_NAME "pf_probe_B"

#define MAX_SYMBOL_LEN	64
static char symbol[MAX_SYMBOL_LEN] = "_do_fork";
module_param_string(symbol, symbol, sizeof(symbol), 0644);

/* For each probe you need to allocate a kprobe structure */
static struct kprobe kp = {
	.symbol_name	= symbol,
};


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
	kp.pre_handler = handler_pre;
	kp.post_handler = handler_post;
	kp.fault_handler = handler_fault;

	ret = register_kprobe(&kp);

	if (ret < 0) {
		pr_err("register_kprobe failed, returned %d\n", ret);
		return ret;
	}
	pr_info("Planted kprobe at %p\n", kp.addr);
	if (DRIVER_DEBUG) {
		printk(KERN_INFO "%s Module: Installed ...\n", DRIVER_NAME);
	}
	return 0;
}


static void __exit pf_probe_exit(void) {

	unregister_kprobe(&kp);
	pr_info("kprobe at %p unregistered\n", kp.addr);
	if (DRIVER_DEBUG) {
		printk(KERN_INFO "%s Module: Removed ...\n", DRIVER_NAME);
	}
}


module_init(pf_probe_init);
module_exit(pf_probe_exit);
