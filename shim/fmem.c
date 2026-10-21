/* /dev/fmem — physical memory read/write for lilac bring-up debug. */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/uaccess.h>

static ssize_t fmem_read(struct file *f, char __user *ub, size_t len,
			 loff_t *ppos)
{
	char *kbuf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	ssize_t ret = 0;

	if (!kbuf)
		return -ENOMEM;
	while (len) {
		phys_addr_t base = *ppos & ~((phys_addr_t)PAGE_SIZE - 1);
		unsigned off = *ppos - base;
		size_t done = min(len, (size_t)(PAGE_SIZE - off));
		void *m = memremap(base, PAGE_SIZE, MEMREMAP_WB);

		if (!m) {
			ret = ret ? ret : -EFAULT;
			break;
		}
		memcpy(kbuf, m + off, done);
		memunmap(m);
		if (copy_to_user(ub, kbuf, done)) {
			ret = ret ? ret : -EFAULT;
			break;
		}
		ub += done;
		*ppos += done;
		len -= done;
		ret += done;
	}
	kfree(kbuf);
	return ret;
}

static ssize_t fmem_write(struct file *f, const char __user *ub, size_t len,
			  loff_t *ppos)
{
	char *kbuf = kmalloc(PAGE_SIZE, GFP_KERNEL);
	ssize_t ret = 0;

	if (!kbuf)
		return -ENOMEM;
	while (len) {
		phys_addr_t base = *ppos & ~((phys_addr_t)PAGE_SIZE - 1);
		unsigned off = *ppos - base;
		size_t done = min(len, (size_t)(PAGE_SIZE - off));
		void *m;

		if (copy_from_user(kbuf, ub, done)) {
			ret = ret ? ret : -EFAULT;
			break;
		}
		m = memremap(base, PAGE_SIZE, MEMREMAP_WB);
		if (!m) {
			ret = ret ? ret : -EFAULT;
			break;
		}
		memcpy(m + off, kbuf, done);
		memunmap(m);
		ub += done;
		*ppos += done;
		len -= done;
		ret += done;
	}
	kfree(kbuf);
	return ret;
}

static int fmem_mmap(struct file *f, struct vm_area_struct *vma)
{
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	return remap_pfn_range(vma, vma->vm_start,
			       (unsigned long)f->f_pos >> PAGE_SHIFT,
			       vma->vm_end - vma->vm_start, vma->vm_page_prot);
}

static const struct file_operations fmem_fops = {
	.owner = THIS_MODULE,
	.llseek = default_llseek,
	.read = fmem_read,
	.write = fmem_write,
	.mmap = fmem_mmap,
};

static struct miscdevice fmem_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "fmem",
	.fops = &fmem_fops,
	.mode = 0600,
};

static int __init fmem_init(void)
{
	return misc_register(&fmem_misc);
}

static void __exit fmem_exit(void)
{
	misc_deregister(&fmem_misc);
}

module_init(fmem_init);
module_exit(fmem_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("physical memory access for bring-up debug");
