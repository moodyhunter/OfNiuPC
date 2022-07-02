#include "linux/binfmts.h"
#include "linux/printk.h"
#include "linux/sched.h"
#include "linux/stddef.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/processor.h>
#include <linux/seq_file.h>

#define KPROBE_LOOKUP 1
#include <linux/kprobes.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Moody");
MODULE_DESCRIPTION("Hook your cpuinfo up");
MODULE_VERSION("1.00");

struct seq_operations *cpuinfo_ptr = NULL;
static int (*original_show_fn)(struct seq_file *m, void *v);

#define MOD "OfNiuPC: "

inline void mywrite_cr0(unsigned long val)
{
    asm volatile("mov %0,%%cr0" : "+r"(val) : : "memory");
}

inline void enable_write_protection(void)
{
    unsigned long cr0 = read_cr0();
    set_bit(16, &cr0);
    mywrite_cr0(cr0);
}

inline void disable_write_protection(void)
{
    unsigned long cr0 = read_cr0();
    clear_bit(16, &cr0);
    mywrite_cr0(cr0);
}

struct seq_operations *get_cpuinfo_op(void)
{
    static struct kprobe kp = { .symbol_name = "cpuinfo_op" };
    struct seq_operations *cpuinfo_ptr;
    register_kprobe(&kp);
    cpuinfo_ptr = (struct seq_operations *) kp.addr;
    unregister_kprobe(&kp);
    return cpuinfo_ptr;
}

static int custom_show_function(struct seq_file *m, void *v)
{
    struct linux_binfmt *binfmt = current->mm->binfmt;

    pr_warn(MOD "hooked call, from PID: %d, module: %s", current->pid, binfmt->module->name);
    seq_printf(m, "processor   : %lld\n", m->index);
    seq_printf(m, "vendor_id   : Moody\n");
    // return original_show_fn(m, v);
    return 0;
}

static int mod_init(void)
{
    pr_warn(MOD "acquiring cpuinfo_op...");
    cpuinfo_ptr = get_cpuinfo_op();
    if (!cpuinfo_ptr)
    {
        pr_err(MOD "failed to acquire 'cpuinfo_ptr'");
        return 1;
    }

    pr_warn(MOD "original show function: %p", cpuinfo_ptr->show);
    original_show_fn = cpuinfo_ptr->show;

    disable_write_protection();
    cpuinfo_ptr->show = custom_show_function;
    enable_write_protection();

    pr_warn(MOD "fully initialised");
    return 0;
}

static void mod_exit(void)
{
    disable_write_protection();
    cpuinfo_ptr->show = original_show_fn;
    enable_write_protection();

    pr_warn(MOD "clean up finished, bye~");
}

module_init(mod_init);
module_exit(mod_exit);
