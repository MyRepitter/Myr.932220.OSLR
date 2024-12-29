#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/proc_fs.h> 
#include <linux/uaccess.h>
#include <linux/version.h>

MODULE_LICENSE("GPL");
#define PROC_FILE_NAME "lab4"
static struct proc_dir_entry *our_proc_file;

static unsigned long minutesAfterChelyabinskMeteorite(void) {
    struct tm meteorite_time = {
        .tm_year = 2013 - 1900,
        .tm_mon = 2 - 1,
        .tm_mday = 15, 
        .tm_hour = 9, 
        .tm_min = 20,
        .tm_sec = 0
    };
    struct timespec64 just_now;
    ktime_get_real_ts64(&just_now);
    time64_t meteorite_timestamp = mktime64(meteorite_time.tm_year + 1900,
                                          meteorite_time.tm_mon + 1,
                                          meteorite_time.tm_mday,
                                          meteorite_time.tm_hour,
                                          meteorite_time.tm_min,
                                          meteorite_time.tm_sec);
    time64_t difference = just_now.tv_sec - meteorite_timestamp;
    return difference / 60;
}

// Функция, вызываемая при чтении файла
static ssize_t procfile_read(struct file *file_pointer, char __user *buffer,
                             size_t buffer_length, loff_t *offset) {
    unsigned long result = minutesAfterChelyabinskMeteorite();
    char str[128];
    int length;

    length = snprintf(str, sizeof(str), "%lu minutes have passed since the fall of the Chelyabinsk meteorite.\n", result);

    if (*offset > 0) {
        return 0;
    }
    if (copy_to_user(buffer, str, length)) {
        return -EFAULT;
    }
    *offset = length;

    pr_info("procfile read %s\n", file_pointer->f_path.dentry->d_name.name);
    return length;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
};
#else
static const struct file_operations proc_file_fops = {
    .read = procfile_read,
};
#endif

static int __init procfs1_init(void) {
    our_proc_file = proc_create(PROC_FILE_NAME, 0644, NULL, &proc_file_fops);
    if (our_proc_file == NULL) {
        pr_err("Не удалось создать /proc/%s\n", PROC_FILE_NAME);
        return -ENOMEM;
    }
    pr_info("/proc/%s создан\n", PROC_FILE_NAME);
    return 0;
}

static void __exit procfs1_exit(void) {
    proc_remove(our_proc_file);
    pr_info("/proc/%s удален\n", PROC_FILE_NAME);
}

module_init(procfs1_init);
module_exit(procfs1_exit);
