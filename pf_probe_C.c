/*
 *  pf_probe_C.c
 *  Contains implementation of kernel module which writes up to three separate scatter plots - one for page faults in the user's code segment, one for page faults in the user's data segment, and one for page faults that are not in either the code segment or the data segment.
 *
 *  Author :
 *  Sagar Vishwakarma (svishwa2@binghamton.edu)
 *  State University of New York, Binghamton
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sagar Vishwakarma");
MODULE_DESCRIPTION("A Simple Linux Page Faults Tracking Device");
MODULE_VERSION("1.0");

#define DRIVER_DEBUG 0
