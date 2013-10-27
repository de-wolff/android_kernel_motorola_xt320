#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <mach/msm_smsm.h>

static unsigned int power_up_reason;

static int bootinfo_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "POWERUPREASON : 0x%08X\n", power_up_reason);
	return 0;
}

static int bootinfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, bootinfo_proc_show, NULL);
}

static const struct file_operations bootinfo_proc_fops = {
	.open		= bootinfo_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_bootinfo_init(void)
{
	unsigned int size;
	power_up_reason = *(unsigned int *)smem_get_entry(SMEM_POWER_ON_STATUS_INFO, &size);
	proc_create("bootinfo", 0, NULL, &bootinfo_proc_fops);
	return 0;
}
module_init(proc_bootinfo_init);
