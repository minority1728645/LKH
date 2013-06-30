#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/string.h>

#include <asm/uaccess.h>
#include <asm/system.h>

#define USAGE "Write this file:\n\tlistvma:show all virtual memory area.\n\tfindpage address:print the physical address\n\twriteval value:set the value of address.\n"

extern struct task_struct *current;

static unsigned long v2p(unsigned long va) 
{ 
    pgd_t *pgd_tmp=NULL; 
    pud_t *pud_tmp=NULL; 
    pmd_t *pmd_tmp=NULL; 
    pte_t *pte_tmp=NULL; 
 
    if(!find_vma(current->mm,va)){
        printk("<0>" "translation not found.\n"); 
        return 0; 
    } 
    pgd_tmp = pgd_offset(current->mm,va); 
    if(pgd_none(*pgd_tmp)){
        printk("<0>" "translation not found.\n");
        return 0; 
    } 
    pud_tmp = pud_offset(pgd_tmp,va); 
    if(pud_none(*pud_tmp)){ 
        printk("<0>" "translation not found.\n"); 
        return 0; 
    } 
    pmd_tmp = pmd_offset(pud_tmp,va); 
    if(pmd_none(*pmd_tmp)){ 
        printk("<0>" "translation not found.\n"); 
    } 
 
    pte_tmp = pte_offset_kernel(pmd_tmp,va); 
 
    if(pte_none(*pte_tmp)){ 
        printk("<0>" "translation not found.\n");
        return 0; 
    } 
    if(!pte_present(*pte_tmp)){ 
        printk("<0>" "translation not found.\n");
        return 0; 
    } 
    return (pte_val(*pte_tmp)&PAGE_MASK)|(va&~PAGE_MASK); 
} 

static int mtest_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, USAGE);
	return 0;
}

static int mtest_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtest_proc_show, NULL);
}

static int mtest_proc_write(struct file *file, const char __user *buffer,size_t count, loff_t *lof){
    char mtest_cmd[1024];
    if(copy_from_user(mtest_cmd,buffer,count)){
        printk("<0>" "Fail to copy from user!");
        return -EFAULT;
    }
    
    mtest_cmd[count]='\0';
    
    if(!strncmp(mtest_cmd,"listvma",7)){
        struct vm_area_struct *vm=current->mm->mmap;
        for(;vm;vm=vm->vm_next){
            char permission[4]="---";
            if(vm->vm_flags & VM_READ)
                permission[0]='r';
            if(vm->vm_flags & VM_WRITE)
                permission[1]='w';
            if(vm->vm_flags & VM_EXEC)
                permission[2]='x';
            printk("<0>" "%08lx %08lx %s\n",vm->vm_start,vm->vm_end,permission);
        }
    }
    else if(!strncmp(mtest_cmd,"findpage",8)){
        unsigned int addr;
        char cmd[10];
        sscanf(mtest_cmd,"%s %x",cmd,&addr);
        printk("<0>" "%lx\n",v2p(addr));
    }
    else if(!strncmp(mtest_cmd,"writeval",8)){
        int addr;
        unsigned long value;
        char cmd[10];
        sscanf(mtest_cmd,"%s %x %lu",cmd,&addr,&value);
        *(unsigned long *)((void *)v2p(addr)+PAGE_OFFSET)=value;
    }

    return count;
}

static const struct file_operations mtest_proc_fops = {
	.open		= mtest_proc_open,
	.read		= seq_read,
    .write      = mtest_proc_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_mtest_init(void)
{
	proc_create("mtest", 0, NULL, &mtest_proc_fops);
	return 0;
}

static void __exit proc_mtest_exit(void)
{
	remove_proc_entry("mtest", NULL);
}

module_init(proc_mtest_init);
module_exit(proc_mtest_exit);
