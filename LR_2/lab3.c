#include<linux/kernel.h> // Базовый заголовок ядра Linux
#include <linux/module.h> // Заголовок создания модулей ядра
#include <linux/printk.h>// Заголовок вывода сообщений в лог ядра
#include<linux/proc_fs.h> // Заголовок работы с файловой системой /proc
#include<linux/uaccess.h> // Заголовок передачи данных между пространством ядра и пользовательским пространством
#include <linux/version.h> // Заголовок проверки версии ядра

static int __init f_init(void) // Функция инициализации для модуля ядра
{
    pr_info("Welcome to the Tomsk State University\n"); // Выводит информационное сообщение в журнал ядра
    return 0;
}

static void __exit f_exit(void) // Функциия выхода для модуля ядра
{
    pr_info("Tomsk State University forever!\n");  // Выводит информационное сообщение в журнал ядра
}

module_init(f_init); // Указываем функцию инициализации, которая будет вызвана при загрузке модуля в ядро
module_exit(f_exit); // Указываем функцию, которая будет вызвана при выгрузке модуля из ядра
MODULE_LICENSE("GPL"); // Модуль лицензирован под GNU General Public License