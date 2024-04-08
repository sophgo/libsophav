/*
 *
 *       UMRBus PCI(e) Kernel Mode Device Driver for Linux
 *
 *            Copyright (C) 2001 - 2015 Synopsys, Inc.
 *
 *  This software is licensed under the terms of the GNU General Public License, version 2 (GPLv2),
 *  as published by the Free Software Foundation, and you may redistribute and/or modify this
 *  software under the terms of that license.
 *
 *  To the maximum extent permitted by law, this program is provided on an "AS IS" BASIS, WITHOUT 
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied, including, without limitation, 
 *  any warranties or conditions of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A 
 *  PARTICULAR PURPOSE. You are solely responsible for determining the appropriateness of using or 
 *  redistributing the software and assume any risks  associated with your exercise of permissions under the GPLv2.
 *  See the GPLv2 for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef MODULE
#  error "DRIVER SUPPORTS ONLY MODULES"
#endif

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18))
#include <linux/config.h> /* retrieve the CONFIG_* macros */
#endif

/* usually defined in autoconf.h (or /boot/vmlinuz.autoconf.h resp.); see also [LDD2 p. 22] */
#ifdef CONFIG_SMP
#  define __SMP__
#endif

/* check kernel version */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)) /* linux version must be 2.4.x or greater */
#error "PLEASE UPGRADE TO LINUX 2.4.x OR HIGHER"
#endif

/* remember about the current version */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
#  define LINUX_24
#else
#  define LINUX_26
#endif

#ifdef CONFIG_MODVERSIONS
#  ifndef MODVERSIONS
#    define MODVERSIONS   /* force it on */
#  endif
#endif

#ifndef LINUX_26
#ifdef MODVERSIONS
#  include <linux/modversions.h>
#endif
#endif

#include <linux/module.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/moduleparam.h>
#endif

#include <linux/init.h> /* module_init(), module_exit() */

#if defined (LINUX_24) || defined (LINUX_26) /* use "vm_pgoff" to get an offset */
#define VMA_OFFSET(vma)  ((vma)->vm_pgoff << PAGE_SHIFT)
#else /* use "vm_offset" */
#define VMA_OFFSET(vma)  ((vma)->vm_offset)
#endif

/* choosing the right 'remap_page_range':

   The number of arguments to remap_page_range has changed from 4 to 5
   in Red Hat 9.0 (kernel 2.4.20) and in kernel 2.6.x.
   (In SuSE 9.0 (kernel 2.4.21) remap_page_range still has 4 arguments, however!)
   Let's try to infer the correct version of remap_page_range:
*/
#ifndef UMRDRV1_REMAP_PAGE_RANGE_5ARGS
#if (((LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,20)) && \
       defined(RED_HAT_LINUX_KERNEL) && (RED_HAT_LINUX_KERNEL == 1)) || \
       (LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)))
#define UMRDRV1_REMAP_PAGE_RANGE_5ARGS 1
#endif
#endif
/* If the automatic detection above fails, you can enforce the 5-argument version on the
   command line: make MOREFLAGS='-DUMRDRV1_REMAP_PAGE_RANGE_5ARGS'

   Btw, RED_HAT_LINUX_KERNEL is defined in rhconfig.h, which gets included from version.h 
   ./linux/autoconf.h:#define CONFIG_SUSE_KERNEL 1
*/

/* Another complication: As of kernel 2.6.11 the function 'remap_page_range'
   has vanished; it was replaced with 'remap_pfn_range'.
   That's why we now only use the macro REMAP_PAGE_RANGE, which should
   expand appropriately:
*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,11))
#define REMAP_PAGE_RANGE(vma, uvaddr, paddr, size, prot)		\
       remap_pfn_range(vma, uvaddr, paddr >> PAGE_SHIFT, size, prot)
#else
#define REMAP_PAGE_RANGE remap_page_range
#endif

/* 2.6.23 (October 9, 2007): unregister_chrdev() now returns void.
   [http://lwn.net/Articles/183225/]
*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,23))
#define UNREGISTER_CHRDEV(major, name)               \
    unregister_chrdev(major, name)
#else
#define UNREGISTER_CHRDEV(major, name)               \
    if((unregister_chrdev(major, name)))  \
        printk("UMRDrv1: unable to unregister character device \"name\"\n")
#endif /* >= 2.6.23 */

/* Although it was stated in [LDD p.409] that 'pci_register_driver' *not* be used directly 
   by modularized code, the 'pci_module_init' wrapper was deprecated meanwhile
   (probably deprecated since kernel 2.6.18 and removed in kernel 2.6.22).
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18))
#define pci_module_init pci_register_driver
#endif

/* until kernel 2.4.x this used to be defined in the Makefile: */
#ifndef UMRDRV1_MAJOR
#  define UMRDRV1_MAJOR 60
#endif

/* string format specifier  */
#ifdef _LP64
#define UMRBUS_LONG_FORMAT    "%i"
#define UMRBUS_ULONG_FORMAT    "%u"
#define UMRBUS_HEX_FORMAT   "0x%08X"
#else
#define UMRBUS_LONG_FORMAT    "%i"
#define UMRBUS_ULONG_FORMAT    "%u"
#define UMRBUS_HEX_FORMAT   "0x%08X"
#endif

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <asm/uaccess.h>
#include <asm/errno.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/pci.h>
#include <linux/interrupt.h>

/* 2.6.24 (January 24, 2008):
     The long-deprecated SA_* interrupt flags have been removed in favor of the IRQF_* equivalents.
   2.6.22 (July 8, 2007):
     The old SA_* interrupt flags have not been removed as originally scheduled, 
     but their use will now generate warnings at compile time.
   2.6.18 (September 19, 2006):
     The generic IRQ layer  has been merged. The SA_* flags to request_irq()  have been renamed; 
     the new prefix is IRQF_. A long series of patches has converted in-tree drivers over to the new names; 
     The old names are scheduled for removal in January, 2007. 

   [http://lwn.net/Articles/183225/]
   
   If at all, IRQF_SHARED is defined in linux/interrupt.h.
*/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18))
#define IRQF_SHARED SA_SHIRQ
#endif /* < 2.6.18 */

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,16))
#include <linux/irqreturn.h>
#endif
#include <linux/random.h>
#ifdef _LP64
#ifdef LINUX_26
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,16))
#include <linux/ioctl32.h>
#endif
#else
#include <asm/ioctl32.h>
#endif
#endif

#include <asm/io.h>

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,21))
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#include <linux/signal.h>

#include "umrdrv1.h"

MODULE_AUTHOR("Synopsys, Inc.");
MODULE_DESCRIPTION("Driver for Synopsys, Inc. CHIPit and HAPS PCI/PCIe cards featuring a PCI master controller");
MODULE_LICENSE("GPL");

#ifdef LINUX_26
MODULE_VERSION("3.0.11");
#endif

#ifndef VM_RESERVED            /* Added 2.4.0-test10 */
#  define VM_RESERVED 0
#endif

#ifdef DEBUG
#define DPRINT(S) \
  printk ("UMRDrv1: "); \
  printk S ; \
  printk ("\n")
#else
#define DPRINT(S) while(0)
#endif

#ifdef ISRDEBUG
#define ISRDPRINT(S) \
  printk ("UMRDrv1_ISR: "); \
  printk S ; \
  printk ("\n")
#else
#define ISRDPRINT(S) while(0)
#endif

#ifdef UMRDEBUG
#define UMRDPRINT(S) \
  printk ("UMRDrv1 t=%li: ", jiffies); \
  printk S ; \
  printk ("\n")
#else
#define UMRDPRINT(S) while(0)
#endif

/* timing debug */
#define CPU_CLK_PERIODE_PS 375
#ifdef TDEBUG
static unsigned long TStart[10];
static unsigned long TStop[10];
#define TDPRINT(S) \
  printk ("UMRDrv1_T: "); \
  printk S ; \
  printk ("\n")
#else
#define TDPRINT(S) while(0)
#endif

/* Local Function Prototypes */

static int umrdrv1_init_module(void);
static void umrdrv1_cleanup_module(void);
static int umrdrv1_probe(struct pci_dev *pdev, const struct pci_device_id *ent);
static void umrdrv1_remove(struct pci_dev *pdev);

/* driver open, close, read and write functions */
static int     umrdrv1_open(struct inode *inode, struct file *file);
static int     umrdrv1_release(struct inode *inode, struct file *file);
static ssize_t umrdrv1_read(struct file *file, char *buffer, size_t count, loff_t *offset);
static ssize_t umrdrv1_write(struct file *file, const char *buffer, size_t count, loff_t *offset);

/* driver internal functions */
static int     umrdrv1_umrbus_scan(umrdrv1_main_type *pmain, int umrbus);

/* 2.6.19 (November 29, 2006):
   The prototype for interrupt handler functions has changed. In short, the regs argument has been removed, 
   since almost nobody used it. Any interrupt handler which needs the pre-interrupt register state can use 
   get_irq_regs() to obtain it.
   [http://lwn.net/Articles/183225/]
 */
#ifdef LINUX_26
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
static irqreturn_t  umrdrv1_master_isr(int irq, void *dev_id, struct pt_regs *regs);
#else
static irqreturn_t  umrdrv1_master_isr(int irq, void *dev_id);
#endif /* < 2.6.19 */
#else
static void    umrdrv1_master_isr(int irq, void *dev_id, struct pt_regs *regs);
#endif

static int     umrdrv1_master_readwrite(umrdrv1_main_type *pmain, u32 header,
                                       char *buffer, u32 length, int mode);
static int     umrdrv1_master_init(struct pci_dev *pdev,umrdrv1_main_type *pmain);
static void    /* __devexit */ umrdrv1_master_deinit(struct pci_dev *pdev,umrdrv1_main_type *pmain);

/* IOCTL functions */
/* static int     umrdrv1_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long value); */
#ifdef _LP64
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14))
static long    umrdrv1_compat_ioctl(struct file *file, unsigned int cmd, unsigned long value);
#else
/* up to kernel 2.6.13 we use register_ioctl32_conversion instead */
#endif
#endif
static int     umrdrv1_ioctl_get_revision(void *value);
static int     umrdrv1_ioctl_client_open(struct file *file, void *value);
static int     umrdrv1_ioctl_scan(struct file *file, void *value);
static int     umrdrv1_ioctl_interrupt(struct file *file, void *value);
static int     umrdrv1_ioctl_testframe(struct file *file, void *value);

/* The list of different types of PCI devices that this driver supports. */
/* (used in struct pci_driver) */
static struct pci_device_id umrdrv1_driver_ids[] = {
    { PCI_VENDOR_ID_SYNOPSYS, PCI_DEVICE_ID_SYNOPSYS_PCIMASTER, PCI_ANY_ID, PCI_ANY_ID, PCI_CLASS_NOT_DEFINED, 0, 0 },
    { PCI_VENDOR_ID_SYNOPSYS, PCI_DEVICE_ID_SYNOPSYS_PCIEXPRESS, PCI_ANY_ID, PCI_ANY_ID , PCI_CLASS_NOT_DEFINED, 0, 0},
    { PCI_VENDOR_ID_SYNOPSYS, PCI_DEVICE_ID_SYNOPSYS_HAPS31, PCI_ANY_ID, PCI_ANY_ID , PCI_CLASS_NOT_DEFINED, 0, 0},
    { PCI_VENDOR_ID_SYNOPSYS, PCI_DEVICE_ID_SYNOPSYS_DIRECT_UMR_HAPS70,  PCI_ANY_ID, PCI_ANY_ID , PCI_CLASS_NOT_DEFINED, 0, 0},
    { PCI_VENDOR_ID_SYNOPSYS, PCI_DEVICE_ID_SYNOPSYS_DIRECT_UMR_HAPSDX7_DWC,  PCI_ANY_ID, PCI_ANY_ID , PCI_CLASS_NOT_DEFINED, 0, 0},
    { PCI_VENDOR_ID_SYNOPSYS, PCI_DEVICE_ID_SYNOPSYS_DIRECT_UMR_HAPSDX7,  PCI_ANY_ID, PCI_ANY_ID , PCI_CLASS_NOT_DEFINED, 0, 0},
    { PCI_VENDOR_ID_SYNOPSYS, PCI_DEVICE_ID_SYNOPSYS_DIRECT_UMR_HAPS80,  PCI_ANY_ID, PCI_ANY_ID , PCI_CLASS_NOT_DEFINED, 0, 0},
    { 0, }, /* terminating entry */
};

/* Export pci_device_id to user space (for hotplug and module loading system) [LDD3 p.311] */
MODULE_DEVICE_TABLE(pci, umrdrv1_driver_ids);

/* pci_driver struct == the core of hot-plug support */
static struct pci_driver umrdrv1_driver = {
    .name="umrdrv1_driver",
    .id_table=umrdrv1_driver_ids,
    .probe=umrdrv1_probe,
   .remove=umrdrv1_remove,
};

/* umrdrv1 dynamic major number */
static int UMRDRV1_MAJOR_DYNAMIC=0;

/* driver main structure */
static umrdrv1_main_type *umrdrv1_pmain[UMRDRV1_MAX_DEVICES];

static struct file_operations umrdrv1_fops = {
    owner   : THIS_MODULE,      /* for tracking usage count (krnl 2.4, 2.6) */
    read    : umrdrv1_read,     /* read */
    write   : umrdrv1_write,    /* write */ 
    unlocked_ioctl   : umrdrv1_compat_ioctl,    /* ioctl */
#ifdef _LP64
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14))
    compat_ioctl : umrdrv1_compat_ioctl,
#else
/* up to kernel 2.6.13 we use register_ioctl32_conversion instead */
#endif
#endif
    open    : umrdrv1_open,     /* open */
    release : umrdrv1_release,  /* release */
};

#ifdef _LP64
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,14))
/* store for succesful ioctl32 registration*/
static int ioctl32registered = 0;
#else
#endif
#endif

/******************************************************************************
 * umrdrv1_probe - Device Initialization Routine
 *
 * @param pdev PCI device information struct
 * @param ent entry in umrdrv1_driver_ids
 *
 * @return 0 on success, negative on failure
*******************************************************************************/
static int umrdrv1_probe(struct pci_dev *pdev,
            const struct pci_device_id *ent)
{
    int retvalue=0,j;
    
    DPRINT(("     >>> umrdrv1_probe"));
    
    for (j=0; j<UMRDRV1_MAX_DEVICES; j++) {
        if (umrdrv1_pmain[j]) {
            DPRINT(("umrdrv1_pmain[%i] already exists", j));
            if (umrdrv1_pmain[j]->dev_number != j) {
                printk(KERN_WARNING "UMRDrv1: umrdrv1_probe: dev_number %i != %i\n",umrdrv1_pmain[j]->dev_number,j);
            }
            continue;
        } else {
            if ((umrdrv1_pmain[j] = (umrdrv1_main_type *) kmalloc(sizeof(umrdrv1_main_type), GFP_KERNEL)) == NULL) {
                printk(KERN_WARNING "UMRDrv1: umrdrv1_probe: unable to allocate memory for umrdrv1 main structure\n");
                retvalue=-ENOMEM;
                goto exit;
            }
            /* clear main structure */
            memset(umrdrv1_pmain[j], 0, sizeof(umrdrv1_main_type));
            umrdrv1_pmain[j]->dev = pdev;
            umrdrv1_pmain[j]->dev_number = j;
            printk("UMRDrv1: umrdrv1_probe: Initializing device number %i ...\n",umrdrv1_pmain[j]->dev_number);
            break;
        }
    }
    
    if (j >= UMRDRV1_MAX_DEVICES) {
        printk(KERN_WARNING "UMRDrv1: umrdrv1_probe: Maximum number of PCI devices reached\n");
        retvalue = -ENODEV;
        goto exit;
    }
    
    if ((umrdrv1_master_init(pdev,umrdrv1_pmain[j]))) {
        retvalue=-1;
        goto exit_deinit;
    }
    
    pci_set_drvdata(pdev, umrdrv1_pmain[j]);
    goto exit;
    
 exit_deinit:
	umrdrv1_master_deinit(pdev,umrdrv1_pmain[j]);

 exit:
    DPRINT(("     <<< umrdrv1_probe"));
    return (retvalue);
}

/******************************************************************************
 * umrdrv1_remove - Device Removal Routine
 *
 * @param pdev PCI device information struct
 *
 * umrdrv1_remove is called by the PCI subsystem to alert the driver
 * that it should release a PCI device.  The could be caused by a
 * Hot-Plug event, or because the driver is going to be removed from
 * memory.
*******************************************************************************/
static void umrdrv1_remove(struct pci_dev *pdev)
{
    umrdrv1_main_type *pmain = pci_get_drvdata(pdev);
    int j=0;

    DPRINT(("     >>> umrdrv1_remove"));

    if(pmain) {
        umrdrv1_master_deinit(pdev,pmain);
        kfree(pmain);
        for(j=0;j<UMRDRV1_MAX_DEVICES;j++) {
            if(umrdrv1_pmain[j]==pmain) {
                DPRINT(("umrdrv1_pmain pointer of device #%i: 0x%p. Reset to NULL.\n",j,umrdrv1_pmain[j]));
                umrdrv1_pmain[j]=NULL;
                break;
            }
        }
        if(j >= UMRDRV1_MAX_DEVICES) {
            printk(KERN_WARNING "UMRDrv1: umrdrv1_remove: Inconsistency detected; could not reset pmain pointer to NULL!\n");
        }
        pmain=NULL;
        pci_set_drvdata(pdev, NULL);
    } else {
        printk(KERN_WARNING "UMRDrv1: umrdrv1_remove: pmain is 0!");
    }

    pci_disable_device(pdev);

    DPRINT(("     <<< umrdrv1_remove"));
    return;
}

/******************************************************************************
 init function for load umrdrv1 module
*******************************************************************************/
static int __init umrdrv1_init_module(void)
{
    int retvalue=0;

#ifdef _LP64
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,14))
    /* variable for adding ioctl32 registration */
    int ret;
#else
#endif
#endif

    DPRINT((">>> init_module"));

    /* register char device */
    UMRDRV1_MAJOR_DYNAMIC = register_chrdev(UMRDRV1_MAJOR_DYNAMIC, "umrdrv1", &umrdrv1_fops);
    if(UMRDRV1_MAJOR_DYNAMIC < 0)
	{
	    printk("UMRDrv1: unable to register character device \"umrdrv1\"\n");
	    retvalue=-EIO;  // was: -1
	    goto exit;
	}
    DPRINT(("     umrdrv1 successfully registered with major number: %i",UMRDRV1_MAJOR_DYNAMIC));

    retvalue=pci_module_init(&umrdrv1_driver); 
    /* == a wrapper over the pci_register_driver function (which is *not* used directly by modularized code); [LDD2 p.490] */

    if (retvalue < 0) {
        goto exit_unregister_chrdev;
    }    

#ifdef _LP64
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,14))
    /* Add ioctl32 registration */
    ret =  register_ioctl32_conversion(IOCTL_UMRDRV_GET_REVISION,       NULL);
    ret |= register_ioctl32_conversion(IOCTL_UMRDRV_CLIENT_OPEN,        NULL);
    ret |= register_ioctl32_conversion(IOCTL_UMRDRV_INTERRUPT,          NULL);
    ret |= register_ioctl32_conversion(IOCTL_UMRDRV_TESTFRAME,          NULL);
    ret |= register_ioctl32_conversion(IOCTL_UMRDRV_SCAN,               NULL);
    if(ret) {
        printk("UMRDrv1: Error registering ioctl32 conversion.\n");
        ioctl32registered = 0;
    } else {
        ioctl32registered = 1;
        DPRINT(("Successful registering ioctl32 conversion."));
    }
#else
/* (un)register_ioctl32_conversion was removed in kernel 2.6.14; replaced with compat_ioctl */
#endif
#endif

    printk("UMRDrv1: module loaded successfully\n");
    goto exit;

 exit_unregister_chrdev:
    /* unregister char device */
    UNREGISTER_CHRDEV(UMRDRV1_MAJOR_DYNAMIC, "umrdrv1");

 exit:
    DPRINT(("<<< init_module"));
    return (retvalue);
}

/******************************************************************************
* cleanup function for unloading umrdrv1 module
*******************************************************************************/
static void __exit umrdrv1_cleanup_module(void)
{
#ifdef _LP64
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,14))
    int ret;
#else
#endif
#endif
    DPRINT((">>> cleanup_module"));

#ifdef _LP64
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,14))
    /* remove ioctl32 registration */
    if(ioctl32registered) {
        ret =  unregister_ioctl32_conversion(IOCTL_UMRDRV_GET_REVISION);
        ret |= unregister_ioctl32_conversion(IOCTL_UMRDRV_CLIENT_OPEN);
        ret |= unregister_ioctl32_conversion(IOCTL_UMRDRV_INTERRUPT);
        ret |= unregister_ioctl32_conversion(IOCTL_UMRDRV_TESTFRAME);
        ret |= unregister_ioctl32_conversion(IOCTL_UMRDRV_SCAN);
        if(ret) {
            printk("UMRDrv1: Error unregistering ioctl32 conversion.\n");
        } else {
            DPRINT((" Successful unregistering ioctl32 conversion."));
        }
    }
#else
/* (un)register_ioctl32_conversion was removed in kernel 2.6.14; replaced with compat_ioctl */
#endif
#endif

    pci_unregister_driver (&umrdrv1_driver);

    /* unregister char device */
    UNREGISTER_CHRDEV(UMRDRV1_MAJOR_DYNAMIC, "umrdrv1");
    printk("UMRDrv1: module unloaded successfully\n");
    DPRINT(("<<< cleanup_module"));
}

/******************************************************************************
 * open device
 *******************************************************************************/
static int umrdrv1_open(struct inode *inode, struct file *file)
{
    int minor = MINOR(inode->i_rdev);
    umrdrv1_umrbus_client_des_type *clides;

    DPRINT((">>> umrdrv1_open"));

    /* check for valid minor number */
    if((minor <0) || (minor >= UMRDRV1_MAX_DEVICES))
        return (-ENODEV);
    /* check if device exists */
    if(!(umrdrv1_pmain[minor]))
        return (-ENODEV);
    /* allocate memory for client descriptor */
    if((clides = (umrdrv1_umrbus_client_des_type *)kmalloc(sizeof(umrdrv1_umrbus_client_des_type),GFP_KERNEL)) == NULL)
        return (-ENOMEM);
    
    /* initialize client descriptor */
    clides->device = minor;
    clides->bus = 0;
    clides->address = 0;
    
    file->private_data = (void *)clides;
    
    DPRINT(("     device #%i: opened",clides->device));
    DPRINT(("<<< umrdrv1_open"));
    return 0;
}

/******************************************************************************
 * release device
 *******************************************************************************/
static int umrdrv1_release(struct inode *inode, struct file *file)
{
    umrdrv1_umrbus_client_des_type *clides = (umrdrv1_umrbus_client_des_type *)file->private_data;
    umrdrv1_main_type *pmain;
    
    DPRINT((">>> umrdrv1_release"));

    /* get pointer to main structure and check it */
    if(!(pmain=umrdrv1_pmain[clides->device])) {
        DPRINT(("     error access main structure"));
        goto exit;
    }
    
    /* check for valid values */
    if((clides->address <= 0) || (clides->address > UMRDRV1_MAX_UMRBUS_CLIENTS) ||
       (clides->bus >= pmain->number_umrbus)) {
        DPRINT(("     no valid device"));
        goto exit;
    }
    
    /* reset connected and disconnected flag of current client */
    if (down_interruptible(&(pmain->mutex_umrbus_clients)))
        return -ERESTARTSYS;
    
    pmain->umrbus_clients[clides->bus][clides->address-1].flags &= ~UMRDRV1_MASK_UMR_CONNECTED;
    pmain->umrbus_clients[clides->bus][clides->address-1].flags &= ~UMRDRV1_MASK_UMR_DISCONNECTED;
    up(&(pmain->mutex_umrbus_clients));
    
    DPRINT(("     client #%i: umrbus #%i: device #%i: closed",clides->address,clides->bus,clides->device));
    
 exit:
    /* free client descriptor */
    kfree(file->private_data);
    
    DPRINT(("<<< umrdrv1_release"));
    return 0;
}

/******************************************************************************
 * read from device
 *******************************************************************************/
static ssize_t umrdrv1_read(struct file *file, char *buffer, size_t count, loff_t *offset)
{
    umrdrv1_umrbus_client_des_type *clides = (umrdrv1_umrbus_client_des_type *)file->private_data;
    umrdrv1_main_type *pmain;
    unsigned long framesize, header, size=count;
    char *address;
    int retvalue=0;
    
    DPRINT((">>> umrdrv1_read"));

#ifdef TDEBUG
    rdtscl(TStart[0]);
#endif

    /* get pointer to main structure and check it */
    if(!(pmain=umrdrv1_pmain[clides->device])) {
        DPRINT(("     error access main structure"));
        return (-ENODEV);
    }
    
    /* check for valid values */
    if((clides->address <= 0) || (clides->address > UMRDRV1_MAX_UMRBUS_CLIENTS) ||
       (clides->bus >= pmain->number_umrbus)) {
        DPRINT(("     no valid device"));
        return (-ENODEV);
    }
    
    /* is client still connected */
    if (down_interruptible(&(pmain->mutex_umrbus_clients)))
        return -ERESTARTSYS;
    
    if ((pmain->umrbus_clients[clides->bus][clides->address-1].flags & UMRDRV1_MASK_UMR_DISCONNECTED)) {
        DPRINT(("     client is disconnected"));
        up(&(pmain->mutex_umrbus_clients));
        return (-ENODEV);
    }
    up(&(pmain->mutex_umrbus_clients));
    
    /* if nothing to read then return */
    if(count <= 0) {
        DPRINT(("     nothing to read"));
        return 0;
    }
    
    /* check for valid data size */
    if((count & 0x03l)) {
        DPRINT(("     it isn't a valid word size"));
        return (-EINVAL);
    }
    
    /* build header */
    header  = (clides->address << 8);
    header |= UMRDRV1_UMRBUS_READ_FRAME_HEADER;
    header |= ((clides->bus << 1) & 0x0El);
    
#ifdef TDEBUG
    rdtscl(TStop[0]);
#endif

    /* now read data */
    while(count) {
        /* set buffer offset */
        address  = (void *)buffer;
        address += (size-count);
        
        /* set framesize */
        if(count > pmain->max_framesize_bytes)
            framesize = pmain->max_framesize_bytes;
        else
            framesize = count;
        
        header &= 0x0000FFFF;                 /* set length field zero */
        header |= (((framesize >> 2)+1)<<16); /* length in words + 1 word umrbus header */ 
        
        /* read frame */
        if((retvalue=(pmain->readwrite)(pmain, header, address, framesize, UMRDRV1_RWMODE_READ)))
            break;
        count -= framesize;
    }
    
    DPRINT(("     %li bytes read from client %i, umrbus %i, device %i",
            size-count, clides->address, clides->bus, clides->device));
    
    file->f_pos=0;
	
#ifdef TDEBUG
    rdtscl(TStop[1]);
    TDPRINT(("READ PREPARATION: %ld",(TStop[0]-TStart[0])*CPU_CLK_PERIODE_PS));
    TDPRINT(("READ LENGTH=%lu : %ld",size,(TStop[1]-TStart[0])*CPU_CLK_PERIODE_PS));
    TDPRINT(("RW 0x%08lX WH  R: %ld",header,(TStop[4]-TStart[4])*CPU_CLK_PERIODE_PS));
    TDPRINT(("RW 0x%08lX INT R: %ld",header,(TStop[5]-TStart[5])*CPU_CLK_PERIODE_PS));
    TDPRINT(("RW 0x%08lX CTU R: %ld",header,(TStop[6]-TStart[6])*CPU_CLK_PERIODE_PS));
#ifndef LOWLATENCY
    TDPRINT(("RW 0x%08lX H2I R: %ld",header,(TStop[7]-TStop[4])*CPU_CLK_PERIODE_PS));    
    TDPRINT(("RW 0x%08lX WU  R: %ld",header,(TStop[5]-TStart[7])*CPU_CLK_PERIODE_PS));    
    TDPRINT(("RW 0x%08lX RIR R: %ld",header,(TStop[9]-TStart[9])*CPU_CLK_PERIODE_PS));    
#else
    if (!pmain->low_latency_mode_available) {
        TDPRINT(("RW 0x%08lX H2I R: %ld",header,(TStop[7]-TStop[4])*CPU_CLK_PERIODE_PS));    
        TDPRINT(("RW 0x%08lX WU  R: %ld",header,(TStop[5]-TStart[7])*CPU_CLK_PERIODE_PS));    
        TDPRINT(("RW 0x%08lX RIR R: %ld",header,(TStop[9]-TStart[9])*CPU_CLK_PERIODE_PS));    
    }
#endif
#endif

    DPRINT(("<<< umrdrv1_read"));
    if(retvalue)
        return (retvalue);
    return (size-count);
}

/******************************************************************************
 * write to device
 *******************************************************************************/
static ssize_t umrdrv1_write(struct file *file, const char *buffer, size_t count, loff_t *offset)
{
    umrdrv1_umrbus_client_des_type *clides = (umrdrv1_umrbus_client_des_type *)file->private_data;
    umrdrv1_main_type *pmain;
    u32 framesize, header, size=count;
    char *address;
    int retvalue=0;
    
    DPRINT((">>> umrdrv1_write"));    
    
#ifdef TDEBUG
    rdtscl(TStart[0]);
#endif
    
    /* get pointer to main structure and check it */
    if(!(pmain=umrdrv1_pmain[clides->device])) {
        DPRINT(("     error access main structure"));
        return (-ENODEV);
    }
    
    /* check for valid values */
    if((clides->address <= 0) || (clides->address > UMRDRV1_MAX_UMRBUS_CLIENTS) ||
       (clides->bus >= pmain->number_umrbus)) {
        DPRINT(("     no valid device"));
        return (-ENODEV);
    }
    
    /* is client still connected */
    if (down_interruptible(&(pmain->mutex_umrbus_clients)))
        return -ERESTARTSYS;
    
    if ((pmain->umrbus_clients[clides->bus][clides->address-1].flags & UMRDRV1_MASK_UMR_DISCONNECTED)) {
        DPRINT(("     client is disconnected"));
        up(&(pmain->mutex_umrbus_clients));
        return (-ENODEV);
    }
    up(&(pmain->mutex_umrbus_clients));
    
    /* if nothing to write then return */
    if(count <= 0) {
        DPRINT(("     nothing to write"));
        return 0;
    }
    
    /* check for valid data size */
    if((count & 0x03l)) {
        DPRINT(("     it isn't a valid word size"));
        return (-EINVAL);
    }
    
    /* build header */
    header  = (clides->address << 8);
    header |= UMRDRV1_UMRBUS_WRITE_FRAME_HEADER;
    header |= ((clides->bus << 1) & 0x0El);
    
#ifdef TDEBUG
    rdtscl(TStop[0]);
#endif
    
    /* now write data */
    while(count) {
        /* set buffer offset */
        address  = (void *)buffer;
        address += (size-count);
        
        /* set framesize */
        if(count > pmain->max_framesize_bytes)
            framesize = pmain->max_framesize_bytes;
        else
            framesize = count;
        
        header &= 0x0000FFFF;                 /* set length field zero */
        header |= (((framesize >> 2)+1)<<16); /* length in words + 1 word umrbus header */ 
        
        /* write frame */
        if((retvalue=(pmain->readwrite)(pmain, header, address, framesize, UMRDRV1_RWMODE_WRITE)))
            break;
        count -= framesize;
    }
    
    DPRINT(("     %li bytes write to client %i, umrbus %i, device %i",
            (unsigned long)(size-count), clides->address, clides->bus, clides->device));
    
    file->f_pos=0;
    
#ifdef TDEBUG
    rdtscl(TStop[1]);
    TDPRINT(("WRITE PREPARATION: %lu",(TStop[0]-TStart[0])*CPU_CLK_PERIODE_PS));
    TDPRINT(("WRITE LENGTH=%d : %lu",size,(TStop[1]-TStart[0])*CPU_CLK_PERIODE_PS));
    TDPRINT(("RW 0x%08X CFU W: %lu",header,(TStop[2]-TStart[2])*CPU_CLK_PERIODE_PS));
    TDPRINT(("RW 0x%08X WH  W: %lu",header,(TStop[3]-TStart[3])*CPU_CLK_PERIODE_PS));
    TDPRINT(("RW 0x%08X INT W: %lu",header,(TStop[5]-TStart[5])*CPU_CLK_PERIODE_PS));
#ifndef LOWLATENCY
    TDPRINT(("RW 0x%08X H2I W: %lu",header,(TStop[7]-TStop[3])*CPU_CLK_PERIODE_PS));    
    TDPRINT(("RW 0x%08X WU  W: %lu",header,(TStop[5]-TStart[7])*CPU_CLK_PERIODE_PS));    
    TDPRINT(("RW 0x%08X RIR W: %lu",header,(TStop[9]-TStart[9])*CPU_CLK_PERIODE_PS));    
#else
    if (!pmain->low_latency_mode_available) {
        TDPRINT(("RW 0x%08X H2I W: %lu",header,(TStop[7]-TStop[3])*CPU_CLK_PERIODE_PS));    
        TDPRINT(("RW 0x%08X WU  W: %lu",header,(TStop[5]-TStart[7])*CPU_CLK_PERIODE_PS));    
        TDPRINT(("RW 0x%08X RIR W: %lu",header,(TStop[9]-TStart[9])*CPU_CLK_PERIODE_PS));    
    }
#endif
#endif

    DPRINT(("<<< umrdrv1_write"));    
    if(retvalue)
        return (retvalue);
    return (size-count);
}

/******************************************************************************
 * ioctl functions
 *******************************************************************************/
// static int umrdrv1_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long value)
// {
//     int retvalue;
    
//     DPRINT((">>> umrdrv1_ioctl"));
    
//     switch(cmd)
//         {
//         case IOCTL_UMRDRV_GET_REVISION :
//             DPRINT(("     IOCTL_UMRDRV_GET_REVISION"));
//             retvalue=umrdrv1_ioctl_get_revision((void *)value);
//             break;
            
//         case IOCTL_UMRDRV_CLIENT_OPEN :
//             DPRINT(("     IOCTL_UMRDRV_CLIENT_OPEN"));
//             retvalue=umrdrv1_ioctl_client_open(file, (void *)value);
//             break;
            
//         case IOCTL_UMRDRV_INTERRUPT :
//             DPRINT(("     IOCTL_UMRDRV_INTERRUPT"));
//             retvalue=umrdrv1_ioctl_interrupt(file, (void *)value);
//             break;
            
//         case IOCTL_UMRDRV_TESTFRAME :
//             DPRINT(("     IOCTL_UMRDRV_TESTFRAME"));
//             retvalue=umrdrv1_ioctl_testframe(file, (void *)value);
//             break;
            
//         case IOCTL_UMRDRV_SCAN :
//             DPRINT(("     IOCTL_UMRDRV_SCAN"));
//             retvalue=umrdrv1_ioctl_scan(file, (void *)value);
//             break;
            
//         default :
//             DPRINT(("     INVALID MODE"));
//             retvalue=(-ENOSYS);
//             break;
//         }
    
//     DPRINT(("<<< umrdrv1_ioctl"));
//     return (retvalue);
//}

#ifdef _LP64
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,14))
static long umrdrv1_compat_ioctl(struct file *file, unsigned int cmd, unsigned long value)
{
    int retvalue;
    
    DPRINT((">>> umrdrv1_compat_ioctl"));
    
    switch(cmd)
        {
        case IOCTL_UMRDRV_GET_REVISION :
            DPRINT(("     IOCTL_UMRDRV_GET_REVISION"));
            retvalue=umrdrv1_ioctl_get_revision((void *)value);
            break;
            
        case IOCTL_UMRDRV_CLIENT_OPEN :
            DPRINT(("     IOCTL_UMRDRV_CLIENT_OPEN"));
            retvalue=umrdrv1_ioctl_client_open(file, (void *)value);
            break;
            
        case IOCTL_UMRDRV_INTERRUPT :
            DPRINT(("     IOCTL_UMRDRV_INTERRUPT"));
            retvalue=umrdrv1_ioctl_interrupt(file, (void *)value);
            break;
            
        case IOCTL_UMRDRV_TESTFRAME :
            DPRINT(("     IOCTL_UMRDRV_TESTFRAME"));
            retvalue=umrdrv1_ioctl_testframe(file, (void *)value);
            break;
            
        case IOCTL_UMRDRV_SCAN :
            DPRINT(("     IOCTL_UMRDRV_SCAN"));
            retvalue=umrdrv1_ioctl_scan(file, (void *)value);
            break;
            
        default :
            DPRINT(("     INVALID MODE"));
            retvalue=(-ENOSYS);
            break;
        }
    
    DPRINT(("<<< umrdrv1_compat_ioctl"));
    return ((long) retvalue);
}
#else
/* up to kernel 2.6.13 we use register_ioctl32_conversion instead */
#endif
#endif

/******************************************************************************
 * master device initialization 
 *******************************************************************************/
static int umrdrv1_master_init(struct pci_dev *pdev,umrdrv1_main_type *pmain)
{
    int retvalue=0,err,i;
    unsigned long memstart=0L, memlen=0L, ioflags=0L, ulMasterMemSize = UMRDRV1_MASTER_MEMSIZE;
    unsigned short usPcieDeviceControlRegister;
    void*      tVirtDmaAddr = NULL;
    dma_addr_t tPhysDmaAddr = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
    /*    irqreturn_t (*handler) = umrdrv1_master_isr; */
#else
    irq_handler_t handler = umrdrv1_master_isr;
#endif /* < 2.6.19 */
    
    DPRINT(("     >>> umrdrv1_master_init"));
    
    printk("UMRDrv1: vendor|devID: 0x%4.4x|0x%4.4x: \"umrdrv1 device #%i\"\n", pmain->dev->vendor, pmain->dev->device, pmain->dev_number);
    
    /* set up mutexes and spinlock */
   sema_init(&(pmain->mutex_umrbus_clients),1);
    sema_init(&(pmain->data.master.mutex_MainQueue),1);
    sema_init(&(pmain->data.master.mutex_TrueWakeUpRWQ),1);
    sema_init(&(pmain->data.master.mutex_UMRBusInt),1);
    spin_lock_init(&(pmain->data.master.spinlock_TrueWakeUpRWQ));
    spin_lock_init(&(pmain->data.master.spinlock_UMRBusInt));

    /* enable pci (route irq) */
    if ((err=pci_enable_device(pmain->dev))) {
        printk(KERN_WARNING "UMRDrv1: pci_enable_device failed!");
        retvalue=err;
        goto exit;
    }
    
    /* we want to use the device in bus mastering mode: */
    pci_set_master(pmain->dev);
    
   /* set MaxReadRequest to 4096 for DW PCIe core */
    if (pmain->dev->device == PCI_DEVICE_ID_SYNOPSYS_DIRECT_UMR_HAPSDX7_DWC) {
        printk("UMRDrv1: device #%i: Set MaxReadRequest to 4096\n",pmain->dev_number);

        /* read PCIe Device Control Register */
        retvalue = pci_read_config_word(pmain->dev, 
                                        UMRDRV1_PCIE_DEVICE_CONTROL_REGISTER_OFFSET, 
                                        &usPcieDeviceControlRegister);
        if (retvalue != PCIBIOS_SUCCESSFUL) {
            printk("UMRDrv1: device #%i: Could not read 'PCIe Device Control Register' from PCI configuration space\n",pmain->dev_number);
            goto exit;
        }
        
        /* set MaxReadRequest to 4096 */
        usPcieDeviceControlRegister &= ~(0x07 << 12);
        usPcieDeviceControlRegister |=  (0x05 << 12); 
        
        /* update PCIe MaxReadRequestSize */
        retvalue=pci_write_config_word(pmain->dev, 
                                       UMRDRV1_PCIE_DEVICE_CONTROL_REGISTER_OFFSET, 
                                       usPcieDeviceControlRegister);
        if (retvalue != PCIBIOS_SUCCESSFUL) {
            printk("UMRDrv1: device #%i: Could not write 'PCIe Device Control Register' from PCI configuration space\n",pmain->dev_number);
            goto exit;
        }

        // retvalue = pcie_set_readrq(pmain->dev, 4096);
    }

    /* get memory base address */
    if((retvalue=pci_read_config_dword(pmain->dev, PCI_BASE_ADDRESS_0, &(pmain->data.master.MemBaseAddress))) != PCIBIOS_SUCCESSFUL) {
        printk("UMRDrv1: device #%i: Could not read from PCI configuration space\n",pmain->dev_number);
        goto exit;
    }
    DPRINT(("          device #%i: MemBaseAddress: 0x%08X",pmain->dev_number,pmain->data.master.MemBaseAddress));
    
    /* get revision ID and check available features */
    if((retvalue=pci_read_config_byte(pmain->dev, PCI_REVISION_ID, &(pmain->data.master.RevisionID))) != PCIBIOS_SUCCESSFUL) {
        printk("UMRDrv1: device #%i: Could not read from PCI configuration space\n",pmain->dev_number);
        goto exit;
    }
    DPRINT(("          device #%i: Revision ID: 0x%02X",pmain->dev_number,pmain->data.master.RevisionID));
    if((pmain->data.master.RevisionID & 0x0F) > 0) {
        pmain->low_latency_mode_available=1;
        pmain->master_memory_size_available=1;
    } else {
        pmain->low_latency_mode_available=0;
        pmain->master_memory_size_available=0;
    }
    
    DPRINT(("          device #%i: Features : MasterMemorySizeRegister=%i, LowLatencyMode=%i",pmain->dev_number,pmain->master_memory_size_available,pmain->low_latency_mode_available));
    
    /* alternative (and recommended) approach to get the memory base address: */
    memstart=pci_resource_start(pmain->dev, 0); /* get the first I/O region */
    memlen=pci_resource_len(pmain->dev, 0);
    ioflags=pci_resource_flags(pmain->dev, 0);
    if(memstart != pmain->data.master.MemBaseAddress) {
        printk(KERN_ERR "UMRDrv1: device #%i: Inconsistency between PCI config space and pci_resource_start: 0x%08X != 0x%08lX\n",
               pmain->dev_number, pmain->data.master.MemBaseAddress, memstart);
        retvalue=-1;
        goto exit;
    }
    DPRINT(("          device #%i: MemBaseAddress (memstart): 0x%08lX",pmain->dev_number,memstart));
    DPRINT(("          device #%i: memlen: 0x%08lX",pmain->dev_number,memlen));
    DPRINT(("          device #%i: ioflags: 0x%08lX",pmain->dev_number,ioflags));

    if(request_mem_region(memstart, memlen, "umrdrv1")==NULL) {
        printk(KERN_ERR "UMRDrv1: I/O address conflict for device #%i\n", pmain->dev_number);
        retvalue=-1;
        goto exit;
    }

    /* map device memory into virtual memory */
    if((pmain->data.master.MemAddress = (unsigned int *)ioremap(pmain->data.master.MemBaseAddress,UMRDRV1_MASTER_ADDRESSSPACE))==NULL) {
        printk("UMRDrv1: device #%i: could not map device memory into virtual memory\n",pmain->dev_number);
        retvalue=-1;
        goto exit_release_region;
    }
    pmain->data.master.IntRegAddress           = pmain->data.master.MemAddress + UMRDRV1_MASTER_INTREGOFFSET;
    pmain->data.master.MaskRegAddress          = pmain->data.master.MemAddress + UMRDRV1_MASTER_MASKREGOFFSET;
    pmain->data.master.UMRNumRegAddress        = pmain->data.master.MemAddress + UMRDRV1_MASTER_UMRNUMBREGOFFSET;
    pmain->data.master.MasterAddressRegAddress = pmain->data.master.MemAddress + UMRDRV1_MASTER_MASTERADDRESSOFFSET;
    pmain->data.master.UMRBusHeaderRegAddress  = pmain->data.master.MemAddress + UMRDRV1_MASTER_UMRHEADEROFFSET;
    pmain->data.master.MasterMemorySize        = pmain->data.master.MemAddress + UMRDRV1_MASTER_MEMSIZEOFFSET;
    
    DPRINT(("          device #%i: device memory successfully mapped",pmain->dev_number));
    
    /* get DMA memory size */
    if (pmain->master_memory_size_available) {
        ulMasterMemSize = (*(pmain->data.master.MasterMemorySize) & 0xffff) * 512;
    } else {
        /* check if DMA memory size register is available */
        *(pmain->data.master.MasterMemorySize) = 0x534c;
        if ((~(*(pmain->data.master.MasterMemorySize) >> 16) & 0xffff) == 0x534c) {
            ulMasterMemSize = (*(pmain->data.master.MasterMemorySize) & 0xffff) * 512;
        }
    }

    /* DMA memory allocation probing */
    do {
        tVirtDmaAddr = pci_alloc_consistent(pdev,ulMasterMemSize*sizeof(u32),&tPhysDmaAddr);
        if ( tVirtDmaAddr == NULL )
            ulMasterMemSize = ulMasterMemSize / 2;
        else
            break;
    } while (ulMasterMemSize >= UMRDRV1_MASTER_MEMSIZE);
    
    /* check if memory is available */
    if ( tVirtDmaAddr == NULL ) {
        printk("UMRDrv1: device #%i: unable to allocate memory for DMA buffer",pmain->dev_number);
        /* unmap device memory */
        vfree(pmain->data.master.MemAddress);
        retvalue=-ENOMEM;
        goto exit_release_region;
    } else {
        DPRINT(("          device #%i: allocated %lu words of memory for DMA buffer",pmain->dev_number, ulMasterMemSize));
    }
    
    /* set virtual and physical master addresses */
    pmain->data.master.DataBuffer                 = tVirtDmaAddr; 
    *(pmain->data.master.MasterAddressRegAddress) = tPhysDmaAddr;
    
    DPRINT(("          VIR_ADR: 0x%p",pmain->data.master.DataBuffer));
    DPRINT(("          BUS_ADR: 0x%llX",(unsigned long long)tPhysDmaAddr));
    DPRINT(("read back BUS_ADR: 0x%X",*(pmain->data.master.MasterAddressRegAddress)));
    DPRINT(("          device #%i: dma memory successfully initialized",pmain->dev_number));
    
    /* set number of UMRBusses */
    pmain->number_umrbus = (*pmain->data.master.UMRNumRegAddress & 0x0000000Fl);
    DPRINT(("          device #%i: Number of UMRBusses #%i",pmain->dev_number, pmain->number_umrbus));
    
    /* calculate number of interrupts and set interrupt mask*/
    pmain->interrupt_mask = 0;
#ifdef LOWLATENCY
    if (pmain->low_latency_mode_available) {    
        pmain->interrupt_mask=0;
        printk("UMRDrv1: device #%i is running in low latency mode\n",pmain->dev_number);
    } else {
#endif
        pmain->interrupt_mask=UMRDRV1_MASTER_MASK_INT_WR | UMRDRV1_MASTER_MASK_INT_RD;
#ifdef LOWLATENCY
    }
#endif            
    
    for(i=0;i<pmain->number_umrbus;i++)
        pmain->interrupt_mask |= (0x04 << i);
    DPRINT(("          device #%i: interrupt mask: 0x%08X",pmain->dev_number, pmain->interrupt_mask));
    
    /* reset and initialize device */
    *(pmain->data.master.IntRegAddress)  = pmain->interrupt_mask;
    *(pmain->data.master.MaskRegAddress) = pmain->interrupt_mask;
    
    DPRINT(("          device #%i: device successfully reset",pmain->dev_number));
    
    /* reset UMRBus header register */
    *(pmain->data.master.UMRBusHeaderRegAddress) = 0x0l;    
    
    /* install ISR */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
    err = request_irq(pmain->dev->irq, &umrdrv1_master_isr, IRQF_SHARED, "umrdrv1", pmain);
#else
    err = request_irq(pmain->dev->irq, handler, IRQF_SHARED, "umrdrv1", pmain);
#endif    /* < 2.6.19 */
    if(err) { 
        printk("UMRDrv1: device #%i: could not install interrupt service routine\n",pmain->dev_number);
        /* free DMA memory */
        pci_free_consistent(pdev,ulMasterMemSize*sizeof(u32),pmain->data.master.DataBuffer,*(pmain->data.master.MasterAddressRegAddress));
        
        /* unmap device memory */
        vfree(pmain->data.master.MemAddress);
        retvalue=-1;
        goto exit_release_region;
    }
    DPRINT(("          device #%i: ISR installed with IRQ: #%i",pmain->dev_number,pmain->dev->irq));
    
    /* enforce sending an initial interrupt frame (at first call to umrbus_interrupt())
       to fetch interrupts that are already pending when the driver is being initialized 
       (can happen if an CHIPit Iridium is already up and running before the host PC 
       has booted)
    */
    pmain->data.master.UMRBusInt = pmain->interrupt_mask;    
    
    /* set valid ReadWrite routine */
    pmain->readwrite = umrdrv1_master_readwrite;
    
    /* init RWQueue */
    init_waitqueue_head(&(pmain->data.master.RWQueue));

    /* set frame size */
    pmain->max_framesize_bytes = ulMasterMemSize * sizeof(u32);
    printk("UMRDrv1: device #%i MasterMemSize: %lu words\n",pmain->dev_number,ulMasterMemSize);

    goto exit;
    
 exit_release_region:
    release_mem_region(memstart, memlen);
    DPRINT(("          device #%i: I/O memory region at 0x%08lX released; bailing out.",pmain->dev_number, memstart));

 exit:
    DPRINT(("     <<< umrdrv1_master_init"));
    return (retvalue);
}

/******************************************************************************
 * master device deinitialization 
 *******************************************************************************/
static void /* __devexit */ umrdrv1_master_deinit(struct pci_dev *pdev,umrdrv1_main_type *pmain)
{
    unsigned long memstart=0L, memlen=0L, ioflags=0L;
    
    DPRINT(("     >>> umrdrv1_master_deinit"));
    
    /* alternative (and recommended) approach to get the memory base address: */
    memstart=pci_resource_start(pmain->dev, 0); /* get the first I/O region */
    memlen=pci_resource_len(pmain->dev, 0);
    ioflags=pci_resource_flags(pmain->dev, 0);
    if(memstart != pmain->data.master.MemBaseAddress) {
        printk(KERN_ERR "UMRDrv1: device #%i: Inconsistency between PCI config space and pci_resource_start: 0x%08X != 0x%08lX\n",
               pmain->dev_number, pmain->data.master.MemBaseAddress, memstart);
    }
    DPRINT(("          device #%i: MemBaseAddress (memstart): 0x%08lX",pmain->dev_number,memstart));
    DPRINT(("          device #%i: memlen: 0x%08lX",pmain->dev_number,memlen));
    DPRINT(("          device #%i: ioflags: 0x%08lX",pmain->dev_number,ioflags));
    
    /* free DMA memory */
    pci_free_consistent(pdev,pmain->max_framesize_bytes,pmain->data.master.DataBuffer,*(pmain->data.master.MasterAddressRegAddress));
    
    /* reset and deinitialize device */
    *(pmain->data.master.MaskRegAddress) = 0x0l;
    *(pmain->data.master.IntRegAddress) = pmain->interrupt_mask;
    *(pmain->data.master.MasterAddressRegAddress) = 0x0l;
    *(pmain->data.master.UMRBusHeaderRegAddress) = 0x0l;
    DPRINT(("          device #%i: device successfully reset",pmain->dev_number));

    /* deinstall ISR */
    free_irq(pmain->dev->irq, pmain);
    DPRINT(("          device #%i: ISR uninstalled",pmain->dev_number));

    /* unmap device memory */
    vfree(pmain->data.master.MemAddress);

    pmain->data.master.IntRegAddress = NULL;
    pmain->data.master.MaskRegAddress = NULL;
    pmain->data.master.MasterAddressRegAddress = NULL;
    pmain->data.master.UMRBusHeaderRegAddress = NULL;
    DPRINT(("          device #%i: device memory unmapped",pmain->dev_number));
    
    /* release the registered I/O memory region */
    release_mem_region(memstart, memlen);
    DPRINT(("          device #%i: I/O memory region at 0x%08lX released.",pmain->dev_number,memstart));
    
    DPRINT(("     <<< umrdrv1_master_deinit"));
}

/******************************************************************************
 * returns the driver revision
 *******************************************************************************/
static int umrdrv1_ioctl_get_revision(void *value)
{
    u32 drv_rev=UMRDRV1_REVISION;
    int retvalue=0;
    
    DPRINT(("     >>> umrdrv1_ioctl_get_revision"));
    
    DPRINT(("          driver revision: %i." UMRBUS_ULONG_FORMAT ,(drv_rev>>16),(u32)(drv_rev & 0xFFFFl)));
    
    retvalue=copy_to_user(value, &drv_rev, sizeof(drv_rev));
    
    DPRINT(("     <<< umrdrv1_ioctl_get_revision : %i",retvalue));
    return (retvalue);
}

/******************************************************************************
 * open a connection to a UMRBus client
 *******************************************************************************/
static int umrdrv1_ioctl_client_open(struct file *file, void *value)
{
    umrdrv1_umrbus_client_des_type *clides = (umrdrv1_umrbus_client_des_type *)file->private_data;
    umrdrv1_main_type *pmain;
    int retvalue=0;
    u32 client;
    int umrbus;

    DPRINT(("     >>> umrdrv1_ioctl_client_open"));

    /* get pointer to main structure and check it */
    if(!(pmain=umrdrv1_pmain[clides->device])) {
        retvalue = -ENODEV;
        goto exit;
    }
    
    /* get requested client address and umrbus from user */
    if((copy_from_user((void *)&client, value, sizeof(u32)))) {
        retvalue = -EBADF;
        goto exit;
    }
    umrbus = (client>>8) & 0xFFl;
    client = client & 0xFFl;
    
    /* check for valid values */
    if((client <= 0) || (client > UMRDRV1_MAX_UMRBUS_CLIENTS) ||
       (umrbus >= pmain->number_umrbus)) {
        retvalue = -ENODEV;
        goto exit;
    }
    
    /* scan requested umrbus */
    umrdrv1_umrbus_scan(pmain, umrbus);
    
    /* check if client present and unused */
    if (down_interruptible(&(pmain->mutex_umrbus_clients))) {
        retvalue = -ERESTARTSYS;
        goto exit;
    }
    if ((pmain->umrbus_clients[umrbus][client-1].flags & UMRDRV1_MASK_UMR_PRESENT)) {
        if (!(pmain->umrbus_clients[umrbus][client-1].flags & UMRDRV1_MASK_UMR_CONNECTED)) {
            /* set client connected */
		    pmain->umrbus_clients[umrbus][client-1].flags |= UMRDRV1_MASK_UMR_CONNECTED;
		    clides->address = client;
		    clides->bus = umrbus;
		    DPRINT(("          client #%i, umrbus #%i, device #%i opened",client,umrbus,clides->device));
		} else {
            /* Error, client is already connected */
            DPRINT(("          client #%i, umrbus #%i, device #%i busy",client,umrbus,clides->device));
            retvalue = -EBUSY;
        }
    } else {
        /* Error, client is not present */
        DPRINT(("          client #%i, umrbus #%i, device #%i no such device",client,umrbus,clides->device));
        retvalue = -ENODEV;
	}
    
    up(&(pmain->mutex_umrbus_clients));
    
 exit:
    DPRINT(("     <<< umrdrv1_ioctl_client_open"));
    return (retvalue);
}

/******************************************************************************
 * master interrupt service routine
 *******************************************************************************/
#ifdef LINUX_26
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
static irqreturn_t  umrdrv1_master_isr(int irq, void *dev_id, struct pt_regs *regs)
#else
static irqreturn_t  umrdrv1_master_isr(int irq, void *dev_id)
#endif /* < 2.6.19 */
#else
static void    umrdrv1_master_isr(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
    u32 regvalue;
    umrdrv1_main_type *pmain = dev_id;
    int i;

    ISRDPRINT(("#### ENTRY ####"));

    /* check if it's our interrupt */
#ifdef TDEBUG
    rdtscl(TStart[9]);
#endif
    if((regvalue = (*pmain->data.master.IntRegAddress) & pmain->interrupt_mask) == 0) {
#ifdef TDEBUG
    rdtscl(TStop[9]);
#endif
	ISRDPRINT(("it isn't our interrupt"));
	ISRDPRINT(("#### EXIT ####"));
#ifdef LINUX_26
	return IRQ_NONE;
#else
	return;
#endif
    }
#ifdef TDEBUG
    rdtscl(TStop[9]);
#endif
#ifdef TDEBUG
    rdtscl(TStop[7]);
#endif

    /* check reason */
    ISRDPRINT(("     select ISR mode"));

#ifdef LOWLATENCY
    /* process only UMRBus interrupts */
    if (pmain->low_latency_mode_available) {
        if((regvalue & UMRDRV1_MASTER_MASK_INT_UB & pmain->interrupt_mask))
            pmain->data.master.ISRMode = UMRDRV1_MASTER_INT_UMRBUS;
        else
            pmain->data.master.ISRMode = UMRDRV1_MASTER_INT_INVALID;
    } else {
#endif
        /* process read/write and UMRBus interrupts */ 
        if((regvalue & UMRDRV1_MASTER_MASK_INT_WR))
            pmain->data.master.ISRMode = UMRDRV1_MASTER_INT_WRITE;
        else {
            if((regvalue & UMRDRV1_MASTER_MASK_INT_RD))
                pmain->data.master.ISRMode = UMRDRV1_MASTER_INT_READ;
            else {
                if((regvalue & UMRDRV1_MASTER_MASK_INT_UB & pmain->interrupt_mask))
                    pmain->data.master.ISRMode = UMRDRV1_MASTER_INT_UMRBUS;
                else
                    pmain->data.master.ISRMode = UMRDRV1_MASTER_INT_INVALID;
            }
        }
#ifdef LOWLATENCY
    }
#endif
    
    switch(pmain->data.master.ISRMode) {
    case UMRDRV1_MASTER_INT_WRITE:
        ISRDPRINT(("     write mode"));
        /* clear write interrupt */
        (*pmain->data.master.IntRegAddress)=UMRDRV1_MASTER_MASK_INT_WR;
        /* set true waked up */
        spin_lock(&(pmain->data.master.spinlock_TrueWakeUpRWQ));
        pmain->data.master.TrueWakeUpRWQ=1;
        spin_unlock(&(pmain->data.master.spinlock_TrueWakeUpRWQ));
        /* wake up process waiting for finish */
#ifdef TDEBUG
        rdtscl(TStart[7]);
#endif	
        wake_up_interruptible(&(pmain->data.master.RWQueue));
        ISRDPRINT(("#### EXIT ####"));
#ifdef LINUX_26
        return IRQ_HANDLED;
#else
        return;
#endif

    case UMRDRV1_MASTER_INT_READ:
        ISRDPRINT(("     read mode"));
        /* clear read interrupt */
        (*pmain->data.master.IntRegAddress)=UMRDRV1_MASTER_MASK_INT_RD;
        /* set true waked up */	
        spin_lock(&(pmain->data.master.spinlock_TrueWakeUpRWQ));
        pmain->data.master.TrueWakeUpRWQ=1;
        spin_unlock(&(pmain->data.master.spinlock_TrueWakeUpRWQ));
        /* wake up process waiting for finish */
#ifdef TDEBUG
        rdtscl(TStart[7]);
#endif	
        wake_up_interruptible(&(pmain->data.master.RWQueue));
        ISRDPRINT(("#### EXIT ####"));
#ifdef LINUX_26
        return IRQ_HANDLED;
#else
        return;
#endif
        
    case UMRDRV1_MASTER_INT_UMRBUS:
        ISRDPRINT(("     umrbus mode"));
        /* Check which UMRBus interrupt is active and update the interrupt flag
         * for this bus. This flag is set if an interrupt is pending on the
         * corresponding bus and an interrupt frame needs to be sent. 
         * Further clear the interrupt request on hardware (HIM) and wake up the
         * first waiting thread in the UMRBusIntQueue if there is one.  
         */
        spin_lock(&(pmain->data.master.spinlock_UMRBusInt));
        for (i = 0; i < pmain->number_umrbus; i++) {
            if ((regvalue & (UMRDRV1_MASTER_MASK_INT_UB0 << i))) {
                pmain->data.master.UMRBusInt |= (UMRDRV1_MASTER_MASK_INT_UB0 << i);
                /* clear umrbus interrupt */
                (*pmain->data.master.IntRegAddress) = (UMRDRV1_MASTER_MASK_INT_UB0 << i);
                if ((pmain->data.master.UMRBusIntQueue[i].first)) {
                    wake_up_interruptible(&(pmain->data.master.UMRBusIntQueue[i].first->wait));
                }
            }
        }
        spin_unlock(&(pmain->data.master.spinlock_UMRBusInt));
        ISRDPRINT(("#### EXIT ####"));
#ifdef LINUX_26
        return IRQ_HANDLED;
#else
        return;
#endif
        
    default:
        printk("UMRDrv1: invalid interrupt mode: INT_REG 0x%08X\n",regvalue);
        ISRDPRINT(("#### EXIT ####"));
#ifdef LINUX_26
        return IRQ_HANDLED;
#else
        return;
#endif
    }
}

/******************************************************************************
 * master read/write routine
 *******************************************************************************/
static int umrdrv1_master_readwrite(umrdrv1_main_type *pmain, u32 header,
                                    char *buffer, u32 length, int mode)
{
    u32 timeout;
    unsigned long irqflags;
    int timeout_io;
    int retvalue=0;
    struct struct_QueueItem QueueItem;
    int busy;

    DPRINT(("     >>> umrdrv1_master_readwrite"));
    DPRINT(("          call with header 0x%08X, length: %i, mode: %i",header,length,mode));
    
    /* Prepare sleeping state */
    set_current_state(TASK_INTERRUPTIBLE);
    
    /* Acquire mutex for main queue access */
    if (down_interruptible(&(pmain->data.master.mutex_MainQueue))) {
        retvalue = -ERESTARTSYS;
        goto exit;
    }
    
    /* Check if device is currently busy */
    busy = (pmain->data.master.MainQueue.first != NULL);
    
    /* initalize and add QueueItem to MainQueue */
    queue_item_init(QueueItem);
    queue_item_add(QueueItem, pmain->data.master.MainQueue);
    
    /* Release main queue mutex */
    up(&(pmain->data.master.mutex_MainQueue));
    
    if (busy) { /* yes, device is busy */
        /* set timeout */
        timeout = HZ * UMRDRV1_MASTER_TIMEOUT;
        /* go sleeping */ 
        DPRINT(("          ReadWrite put to sleep with header " UMRBUS_HEX_FORMAT , header));
        timeout = wait_event_interruptible_timeout(
                                                   QueueItem.wait,
                                                   (QueueItem.TrueWakeUpMQ == 1),
                                                   timeout);
        /* set task running again */
        set_current_state(TASK_RUNNING);
        
        /* Timeout occured */
        if (timeout == 0) {
            DPRINT(("          ReadWrite with header 0x%08X timed out",header));
            retvalue=-ETIMEDOUT;
            goto exit_wakeup;
        }
        /* Signal received */ 
        if (timeout == -ERESTARTSYS) {
            DPRINT(("          ReadWrite with header 0x%08X got signal during busy wait",header));
            retvalue = -ERESTARTSYS;
            goto exit_wakeup;
        }
        /* device is free, now run read/write operation */        
        goto run_operation;
    } else {
    run_operation:
        /* no, device isn't busy */
        set_current_state(TASK_RUNNING);
        
        /* reset true wake up */
        spin_lock_irqsave(&(pmain->data.master.spinlock_TrueWakeUpRWQ), irqflags);
        pmain->data.master.TrueWakeUpRWQ=0;
        spin_unlock_irqrestore(&(pmain->data.master.spinlock_TrueWakeUpRWQ), irqflags);
        /* former versions used to enable all interrupts here: sti(); */
        
        /* select mode */
        switch(mode) {
        case UMRDRV1_RWMODE_WRITE:
            DPRINT(("          write mode"));
#ifdef TDEBUG
            rdtscl(TStart[2]);
#endif
            if(copy_from_user((pmain->data.master.DataBuffer), buffer, length)) {
                retvalue=-EFAULT;
                goto exit_wakeup;
            }
#ifdef TDEBUG
            rdtscl(TStop[2]);
#endif
#ifdef TDEBUG
            rdtscl(TStart[3]);
#endif
            (*(pmain->data.master.UMRBusHeaderRegAddress)) = header;
#ifdef TDEBUG
            rdtscl(TStop[3]);
#endif
            break;

        case UMRDRV1_RWMODE_TEST:
            DPRINT(("          test mode"));
            memcpy((pmain->data.master.DataBuffer), buffer, length);
            (*(pmain->data.master.UMRBusHeaderRegAddress)) = header;
            break;

        case UMRDRV1_RWMODE_SCAN:
        case UMRDRV1_RWMODE_INTF:
        case UMRDRV1_RWMODE_READ:
            DPRINT(("          read/scan/int mode"));
#ifdef TDEBUG
            rdtscl(TStart[4]);
#endif
            (*(pmain->data.master.UMRBusHeaderRegAddress)) = header;
#ifdef TDEBUG
            rdtscl(TStop[4]);
#endif
            break;
	    
        default:
            DPRINT(("          invalid mode #%i",mode));
            retvalue=-EPERM;
            goto exit_wakeup;
        }

#ifdef TDEBUG
        rdtscl(TStart[5]);
#endif

#ifdef LOWLATENCY
        if (pmain->low_latency_mode_available) {
            /* poll if operation is finished */ 
            timeout_io=1000;
            while (timeout_io-- && ((*pmain->data.master.IntRegAddress) & (UMRDRV1_MASTER_MASK_INT_WR|UMRDRV1_MASTER_MASK_INT_RD)) == 0);
            //            printk("poll %i\n",timeout_io);
            if(timeout_io == -1) {
                DPRINT(("          ReadWrite with header 0x%08X timed out during wait for finish",header));
                retvalue=-ETIMEDOUT;
                goto exit_wakeup;
            } else {
                (*pmain->data.master.IntRegAddress)=UMRDRV1_MASTER_MASK_INT_WR|UMRDRV1_MASTER_MASK_INT_RD;
            }
        } else {
#endif
            /* Acquire true wake-up mutex */
            if (down_interruptible(&(pmain->data.master.mutex_TrueWakeUpRWQ))) {
                retvalue = -ERESTARTSYS;
                goto exit_wakeup;
            }
            spin_lock_irqsave(&(pmain->data.master.spinlock_TrueWakeUpRWQ), irqflags);
            
            if(!(pmain->data.master.TrueWakeUpRWQ)) {
                spin_unlock_irqrestore(&(pmain->data.master.spinlock_TrueWakeUpRWQ), irqflags);
                up(&(pmain->data.master.mutex_TrueWakeUpRWQ));
                
                timeout_io = HZ*UMRDRV1_MASTER_TIMEOUT_IO;
            sleeprw:
                /* go sleeping for finish operation */
                DPRINT(("          ReadWrite with header 0x%08X goes sleeping for finish",header));
                
                timeout_io = umrdrv_wait_event_interruptible_timeout(pmain->data.master.RWQueue,
                                                                     pmain->data.master.mutex_TrueWakeUpRWQ,
                                                                     (pmain->data.master.TrueWakeUpRWQ),
                                                                     timeout_io);
                
                DPRINT(("          ReadWrite w. hdr 0x%08X returned from wait for finish with timeout " UMRBUS_HEX_FORMAT ,header,timeout_io));
                
                if(timeout_io == -ERESTARTSYS) {
                    DPRINT(("          ReadWrite with header 0x%08X got signal during wait for finish",header));
                    retvalue=-ERESTARTSYS;
                    goto exit_wakeup;
                }
                
                if(timeout_io == 0) {
                    DPRINT(("          ReadWrite with header 0x%08X timed out during wait for finish",header));
                    retvalue=-ETIMEDOUT;
                    goto exit_wakeup;
                }
                
                /* check if it's a wake up from external */
                if(!(pmain->data.master.TrueWakeUpRWQ)) {
                    DPRINT(("          waked up from external"));
                    goto sleeprw;
                }
                /* reset true wake up */
                spin_lock_irqsave(&(pmain->data.master.spinlock_TrueWakeUpRWQ), irqflags);
                pmain->data.master.TrueWakeUpRWQ=0;
                spin_unlock_irqrestore(&(pmain->data.master.spinlock_TrueWakeUpRWQ), irqflags);
            } else {
                /* former versions used to enable all interrupts here: sti(); */
                spin_unlock_irqrestore(&(pmain->data.master.spinlock_TrueWakeUpRWQ), irqflags);
                up(&(pmain->data.master.mutex_TrueWakeUpRWQ));
            }
            
#ifdef LOWLATENCY
        }
#endif
#ifdef TDEBUG
    rdtscl(TStop[5]);
#endif
	if(mode != UMRDRV1_RWMODE_WRITE) {
	    switch(mode) {
	    case UMRDRV1_RWMODE_READ:
#ifdef TDEBUG
            rdtscl(TStart[6]);
#endif
            if(copy_to_user(buffer, pmain->data.master.DataBuffer, length)) {
                retvalue=-EFAULT;
            }
#ifdef TDEBUG
            rdtscl(TStop[6]);
#endif
            break;
            
	    case UMRDRV1_RWMODE_SCAN:
	    case UMRDRV1_RWMODE_TEST:
	    case UMRDRV1_RWMODE_INTF:
            memcpy(buffer, pmain->data.master.DataBuffer, length);
            break;
	    default:
            break;
	    }
	}
	
    }
    
 exit_wakeup:
    
    /* Acquire mutex without signal interruption */
    down(&(pmain->data.master.mutex_MainQueue));
    
    /* remove QueueItem from MainQueue */
    queue_item_del(QueueItem, pmain->data.master.MainQueue);

    /* Wake up first pending thread in the queue */
    if (pmain->data.master.MainQueue.first) {
        /* set true wake up */
        pmain->data.master.MainQueue.first->TrueWakeUpMQ=1;
        wake_up_interruptible(&(pmain->data.master.MainQueue.first->wait));
    }
    
    /* Release mutex */
    up(&(pmain->data.master.mutex_MainQueue));
    
 exit:
    DPRINT(("     <<< umrdrv1_master_readwrite"));
    return (retvalue);
}

/******************************************************************************
 * scan UMR busses (real function)
 *******************************************************************************/
static int umrdrv1_umrbus_scan(umrdrv1_main_type *pmain, int umrbus)
{
    u32 header;
    u32 *buffer;
    int retvalue=0, i;

    DPRINT(("     >>> umrdrv1_umrbus_scan"));
    
    header  = UMRDRV1_UMRBUS_SCAN_FRAME_HEADER;
    header |= ((umrbus << 1) & 0x0000000E);

    /* allocate memory for buffer */
    if((buffer = (u32 *)kmalloc((UMRDRV1_UMRBUS_SCAN_FRAME_LENGTH * sizeof(u32)), GFP_KERNEL)) == NULL) {
	retvalue=-ENOMEM;
	goto exit;
    }

    /* send frame */
    if((retvalue=(pmain->readwrite)(pmain, header,(char *)buffer, (UMRDRV1_UMRBUS_SCAN_FRAME_LENGTH * sizeof(u32)), UMRDRV1_RWMODE_SCAN)))
        {
	    if(retvalue != -ETIMEDOUT) {
		goto exit_free;
	    }
	    else /* time out error, remove all registered clients */
		{
		    if (down_interruptible(&(pmain->mutex_umrbus_clients)))
			return -ERESTARTSYS;

		    for(i=0;i<UMRDRV1_MAX_UMRBUS_CLIENTS;i++)
			{
			    pmain->umrbus_clients[umrbus][i].flags &= ~UMRDRV1_MASK_UMR_PRESENT;
			    if((pmain->umrbus_clients[umrbus][i].flags & UMRDRV1_MASK_UMR_CONNECTED))
				pmain->umrbus_clients[umrbus][i].flags |= UMRDRV1_MASK_UMR_DISCONNECTED;
			    pmain->umrbus_clients[umrbus][i].type = 0;
			}
		    up(&(pmain->mutex_umrbus_clients));
		    retvalue=0;
		    goto exit_free;
		}
	}

    for(i=0;i<UMRDRV1_MAX_UMRBUS_CLIENTS;i++)
	{
	    switch((*(buffer+i+1) & UMRDRV1_MASK_UMR_ADDRESS))
		{
		case 0:
		    if (down_interruptible(&(pmain->mutex_umrbus_clients)))
			return -ERESTARTSYS;

		    if((pmain->umrbus_clients[umrbus][i].flags & UMRDRV1_MASK_UMR_CONNECTED))
			{
			    pmain->umrbus_clients[umrbus][i].flags &= ~UMRDRV1_MASK_UMR_PRESENT;
			    pmain->umrbus_clients[umrbus][i].flags |= UMRDRV1_MASK_UMR_DISCONNECTED;
			    DPRINT(("            client #%i, umrbus #%i disconnected",i+1,umrbus));
			    retvalue |= UMRDRV1_UMRBUS_EDISCON;
			}
		    else
			{
			    pmain->umrbus_clients[umrbus][i].flags = UMRDRV1_MASK_UMR_NODEV;
			}
		    pmain->umrbus_clients[umrbus][i].type = 0;
		    up(&(pmain->mutex_umrbus_clients));
		    break;

		case 1:
		    if (down_interruptible(&(pmain->mutex_umrbus_clients)))
			return -ERESTARTSYS;

		    if((pmain->umrbus_clients[umrbus][i].flags & UMRDRV1_MASK_UMR_CONNECTED))
			{
			    if(((*(buffer+i+1) & UMRDRV1_MASK_UMR_CLITYPE) >> 16) != pmain->umrbus_clients[umrbus][i].type)
				{
				    pmain->umrbus_clients[umrbus][i].flags |= UMRDRV1_MASK_UMR_DISCONNECTED;
				    retvalue |= UMRDRV1_UMRBUS_EDISCON;
				    DPRINT(("            client #%i, umrbus #%i wrong type",i+1,umrbus));
				}
			}
		    pmain->umrbus_clients[umrbus][i].flags |= UMRDRV1_MASK_UMR_PRESENT;
		    pmain->umrbus_clients[umrbus][i].type = (*(buffer+i+1) & UMRDRV1_MASK_UMR_CLITYPE) >> 16;
		    DPRINT(("            client #%i, umrbus #%i, type 0x%08X found",i+1,umrbus,
			    pmain->umrbus_clients[umrbus][i].type));
		    up(&(pmain->mutex_umrbus_clients));
		    break;

		default:
		    retvalue |= UMRDRV1_UMRBUS_ECLIADR;
		    DPRINT(("            client #%i, umrbus #%i multiple address value: " UMRBUS_HEX_FORMAT ,i+1,umrbus,
			    (*(buffer+i+1) & UMRDRV1_MASK_UMR_ADDRESS)));
		    break;
		}
	}

    /* if UMRDRV1_UMRBUS_ECLIADR set all clients to not present */
    if((retvalue & UMRDRV1_UMRBUS_ECLIADR))
	{
	    if (down_interruptible(&(pmain->mutex_umrbus_clients)))
		return -ERESTARTSYS;

	    for(i=0;i<UMRDRV1_MAX_UMRBUS_CLIENTS;i++)
		{
		    pmain->umrbus_clients[umrbus][i].flags &= ~UMRDRV1_MASK_UMR_PRESENT;
		    if((pmain->umrbus_clients[umrbus][i].flags & UMRDRV1_MASK_UMR_CONNECTED))
			pmain->umrbus_clients[umrbus][i].flags |= UMRDRV1_MASK_UMR_DISCONNECTED;
		    pmain->umrbus_clients[umrbus][i].type = 0;
		}
	    up(&(pmain->mutex_umrbus_clients));
	}

 exit_free:
    /* free buffer */
    kfree(buffer);
    
 exit:
    DPRINT(("     <<< umrdrv1_umrbus_scan"));
    return (retvalue);
}

/******************************************************************************
 * scan UMR busses (IOCTL)
 *******************************************************************************/
static int umrdrv1_ioctl_scan(struct file *file, void *value)
{
    int retvalue=0, i;
    umrdrv1_umrbus_client_des_type *clides = (umrdrv1_umrbus_client_des_type *)file->private_data;
    umrdrv1_main_type *pmain;

    DPRINT(("     >>> umrdrv1_ioctl_scan"));

    /* get pointer to main structure and check it */
    if(!(pmain=umrdrv1_pmain[clides->device])) {
        retvalue=-ENODEV;
	goto exit;
    }

    for(i=0;i< pmain->number_umrbus;i++)
        umrdrv1_umrbus_scan(pmain, i);

    if(copy_to_user(value, &(pmain->umrbus_clients), sizeof(pmain->umrbus_clients))) {
        retvalue=-EFAULT;
    }

 exit:
    DPRINT(("     <<< umrdrv1_ioctl_scan"));
    return (retvalue);
}

/******************************************************************************
 * UMR bus interrupt
 *******************************************************************************/
static int umrdrv1_ioctl_interrupt(struct file *file, void *value)
{
    int retvalue=0,i;
    int WasIntFrame=0;
    umrdrv1_umrbus_client_des_type *clides = (umrdrv1_umrbus_client_des_type *)file->private_data;
    umrdrv1_main_type *pmain;
    umrbus_interrupt_type umrint;
    u32 header;
    u32 *buffer;
    unsigned long irqflags;
    long timeout;
    struct struct_QueueItem QueueItem;
    struct struct_QueueItem *NextQueueItem;

    UMRDPRINT(("     >>> umrdrv1_ioctl_interrupt"));

    /* get umrbus_interrupt_struct */
    if(copy_from_user(&umrint, value,sizeof(umrbus_interrupt_type))) {
        retvalue=-EFAULT;
        goto exit;
    }
    
    /* reset umrint values */
    umrint.interrupt=0;
    umrint.capiinttype=0;
    
    /* get pointer to main structure and check it */
    if(!(pmain=umrdrv1_pmain[clides->device])) {
        UMRDPRINT(("     can't get pointer to main structure"));
        retvalue=-ENODEV;
        goto exit;
    }
    
    /* check for valid values */
    if((clides->address <= 0) || (clides->address > UMRDRV1_MAX_UMRBUS_CLIENTS) ||
       (clides->bus >= pmain->number_umrbus)) {
        UMRDPRINT(("     no valid device"));
        retvalue=-ENODEV;
        goto exit;
    }

    /* is client still connected */
    if (down_interruptible(&(pmain->mutex_umrbus_clients)))
        return -ERESTARTSYS;
    
    if ((pmain->umrbus_clients[clides->bus][clides->address-1].flags & UMRDRV1_MASK_UMR_DISCONNECTED)) {
        UMRDPRINT(("     client is disconnected"));
        retvalue=-ENODEV;
        up(&(pmain->mutex_umrbus_clients));
        goto exit;
    }
    up(&(pmain->mutex_umrbus_clients));
    
    UMRDPRINT(("     called with mode: %i, timeout: %i, client #%i, umrbus #%i, device #%i",
               umrint.mode,umrint.timeout,clides->address,clides->bus,clides->device));
    
    /* build header for INT frame */
    header  = UMRDRV1_UMRBUS_INT_FRAME_HEADER;
    header |= ((clides->bus << 1) & 0x0000000E);
    
    /* must sent an INT frame for current umrbus */
    /* former versions used to disable all interrupts here: cli(); */
    
    if (down_interruptible(&(pmain->data.master.mutex_UMRBusInt)))
        return -ERESTARTSYS;
    
    spin_lock_irqsave(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
    
    if((pmain->data.master.UMRBusInt & (UMRDRV1_MASTER_MASK_INT_UB0 << clides->bus))) {
        /* former versions used to enable all interrupts here: sti(); */
        
        UMRDPRINT(("     send interrupt frame (1)"));
        /* reset umrbus INT bit */
        pmain->data.master.UMRBusInt &= ~(UMRDRV1_MASTER_MASK_INT_UB0 << clides->bus);
        spin_unlock_irqrestore(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
        up(&(pmain->data.master.mutex_UMRBusInt));
        /* allocate memory for buffer */
        if((buffer = (u32 *)kmalloc((UMRDRV1_UMRBUS_INT_FRAME_LENGTH * sizeof(u32)), GFP_KERNEL)) == NULL) {
            retvalue=-ENOMEM;
            goto exit;
        }
        /* send INT frame */
        if((retvalue=(pmain->readwrite)(pmain, header, (char *) buffer,(UMRDRV1_UMRBUS_INT_FRAME_LENGTH * sizeof(u32)), UMRDRV1_RWMODE_INTF))) {
            kfree(buffer);
            goto exit;
        }
        
        if (down_interruptible(&(pmain->mutex_umrbus_clients)))
            return -ERESTARTSYS;
        
        for(i=0;i<UMRDRV1_MAX_UMRBUS_CLIENTS;i++) {
            if((buffer[i+1] & 0x01l)) {
                pmain->umrbus_clients[clides->bus][i].flags |= UMRDRV1_MASK_UMR_INTERRUPT;
                pmain->umrbus_clients[clides->bus][i].capiinttype = (buffer[i+1] >> 16);
            }
        }
        up(&(pmain->mutex_umrbus_clients));
        
        /* free allocated memory */
        kfree(buffer);
    } else {
        spin_unlock_irqrestore(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
        up(&(pmain->data.master.mutex_UMRBusInt));
    }
    
    /* is interrupt bit set */
    if (down_interruptible(&(pmain->mutex_umrbus_clients)))
        return -ERESTARTSYS;
    
    if((pmain->umrbus_clients[clides->bus][clides->address-1].flags & UMRDRV1_MASK_UMR_INTERRUPT)) {
        UMRDPRINT(("     now interrupt bit is set (1)"));
        /* reset capim int flag */
        pmain->umrbus_clients[clides->bus][clides->address-1].flags &= ~UMRDRV1_MASK_UMR_INTERRUPT;
        /* set umrbus_interrupt_struct */
        umrint.interrupt=1;
        umrint.capiinttype=pmain->umrbus_clients[clides->bus][clides->address-1].capiinttype;
        if((pmain->data.master.UMRBusIntQueue[clides->bus].first)) {
            NextQueueItem=pmain->data.master.UMRBusIntQueue[clides->bus].first;
            while(NextQueueItem) {
                wake_up_interruptible(&(NextQueueItem->wait));
                NextQueueItem=NextQueueItem->next;
            }
        }
        /* former versions used to enable all interrupts here: sti(); */
        
        up(&(pmain->mutex_umrbus_clients));
        goto exit;
    }
    up(&(pmain->mutex_umrbus_clients));
    
    /* if mode is polling now return */
    if(umrint.mode==UMRDRV1_UMRBUS_INT_MODE_POLLING) {
        /* former versions used to enable all interrupts here: sti(); */
        
        goto exit;
    }
    
    /* if mode is UMRDRV1_UMRBUS_INT_MODE_WAITEVENT now process go sleeping */
    /* set timeout */
    timeout = umrint.timeout * HZ;
 gosleep:
    /* initialize QueueItem */
    queue_item_init(QueueItem);
    
    set_current_state(TASK_INTERRUPTIBLE);
    
    if (down_interruptible(&(pmain->data.master.mutex_UMRBusInt)))
        return -ERESTARTSYS;
    spin_lock_irqsave(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
    
    /* add QueueItem to MainQueue */
    queue_item_add(QueueItem, pmain->data.master.UMRBusIntQueue[clides->bus]);
    
    spin_unlock_irqrestore(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
    up(&(pmain->data.master.mutex_UMRBusInt));
    
    /* go sleeping */
    UMRDPRINT(("     umrbus_interrupt for client #%i, umrbus #%i, device #%i go sleeping",
               clides->address,clides->bus,clides->device));
    
    timeout = wait_event_interruptible_timeout(
                                               QueueItem.wait,
                                               (pmain->data.master.UMRBusInt & (UMRDRV1_MASTER_MASK_INT_UB0 << clides->bus)), 
                                               timeout);
    
    set_current_state(TASK_RUNNING);
    
    // timeout
    if(timeout == 0) {
        UMRDPRINT(("     timed out for client #%i, umrbus #%i, device #%i",
                   clides->address,clides->bus,clides->device));
        
        down(&(pmain->data.master.mutex_UMRBusInt));
        spin_lock_irqsave(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
        
        UMRDPRINT(("time out: int status: %i int frame status: %i\n",
                   (pmain->umrbus_clients[clides->bus][clides->address-1].flags & UMRDRV1_MASK_UMR_INTERRUPT),
                   (pmain->data.master.UMRBusInt & (UMRDRV1_MASTER_MASK_INT_UB0 << clides->bus))));
        /* remove QueueItem from MainQueue */
        queue_item_del(QueueItem, pmain->data.master.UMRBusIntQueue[clides->bus]);
        /* former versions used to enable all interrupts here: sti(); */
        
        spin_unlock_irqrestore(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
        up(&(pmain->data.master.mutex_UMRBusInt));
        retvalue=-ETIMEDOUT;
        goto exit;
    }
    
    // signal received
    if(timeout == -ERESTARTSYS) {
        down(&(pmain->data.master.mutex_UMRBusInt));
        spin_lock_irqsave(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
        /* remove QueueItem from MainQueue */
        queue_item_del(QueueItem, pmain->data.master.UMRBusIntQueue[clides->bus]);
        spin_unlock_irqrestore(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
        up(&(pmain->data.master.mutex_UMRBusInt));
	    return -ERESTARTSYS;
    }
    
    UMRDPRINT(("     umrbus_interrupt for client #%i, umrbus #%i, device #%i waked up",
               clides->address,clides->bus,clides->device));
    
    down(&(pmain->data.master.mutex_UMRBusInt));
    spin_lock_irqsave(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
    
    /* remove QueueItem from IntQueue */
    queue_item_del(QueueItem, pmain->data.master.UMRBusIntQueue[clides->bus]);
    
    /* must send an interrupt frame for current umrbus */
    if((pmain->data.master.UMRBusInt & (UMRDRV1_MASTER_MASK_INT_UB0 << clides->bus))) {
        UMRDPRINT(("     send interrupt frame (2)"));
        /* set WasIntFrame */
        WasIntFrame=1;
        /* reset umrbus INT bit */
        pmain->data.master.UMRBusInt &= ~(UMRDRV1_MASTER_MASK_INT_UB0 << clides->bus);
        /* former versions used to enable all interrupts here: sti(); */
        spin_unlock_irqrestore(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
        up(&(pmain->data.master.mutex_UMRBusInt));
        
        /* allocate memory for buffer */
        if((buffer = (u32 *)kmalloc((UMRDRV1_UMRBUS_INT_FRAME_LENGTH * sizeof(u32)), GFP_KERNEL)) == NULL) {
            retvalue=-ENOMEM;
            goto exit;
        }
        /* send INT frame */
        if((retvalue=(pmain->readwrite)(pmain, header, (char *) buffer,(UMRDRV1_UMRBUS_INT_FRAME_LENGTH * sizeof(u32)), UMRDRV1_RWMODE_INTF))) {
            kfree(buffer);
            goto exit;
        }
        
        down(&(pmain->mutex_umrbus_clients));
        
        for(i=0;i<UMRDRV1_MAX_UMRBUS_CLIENTS;i++) {
            if((buffer[i+1] & 0x01l)) {
                pmain->umrbus_clients[clides->bus][i].flags |= UMRDRV1_MASK_UMR_INTERRUPT;
                pmain->umrbus_clients[clides->bus][i].capiinttype = (buffer[i+1] >> 16);
            }
        }
        up(&(pmain->mutex_umrbus_clients));
        
        /* free allocated memory */
        kfree(buffer);
    } else {
        spin_unlock_irqrestore(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
        up(&(pmain->data.master.mutex_UMRBusInt));
    }
    
    /* former versions used to enable all interrupts here: sti(); */
    
    /* is now interrupt bit set */
    down(&(pmain->mutex_umrbus_clients));
    
    if((pmain->umrbus_clients[clides->bus][clides->address-1].flags & UMRDRV1_MASK_UMR_INTERRUPT)) {
        UMRDPRINT(("     now interrupt bit is set (2)"));
        /* reset capim int flag */
        pmain->umrbus_clients[clides->bus][clides->address-1].flags &= ~UMRDRV1_MASK_UMR_INTERRUPT;
        /* set umrbus_interrupt_struct */
        umrint.interrupt=1;
        umrint.capiinttype=pmain->umrbus_clients[clides->bus][clides->address-1].capiinttype;
        /* former versions used to disable all interrupts here: cli(); */
        
        up(&(pmain->mutex_umrbus_clients));
        
        down(&(pmain->data.master.mutex_UMRBusInt));
        spin_lock_irqsave(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
        
        if(WasIntFrame) {
            if((pmain->data.master.UMRBusIntQueue[clides->bus].first)) {
                WasIntFrame=0;
                NextQueueItem=pmain->data.master.UMRBusIntQueue[clides->bus].first;
                while(NextQueueItem) {
                    wake_up_interruptible(&(NextQueueItem->wait));
                    NextQueueItem=NextQueueItem->next;
                }
            }
        }
        /* former versions used to enable all interrupts here: sti(); */
        
        spin_unlock_irqrestore(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
        up(&(pmain->data.master.mutex_UMRBusInt));
        goto exit;
    } else {
        up(&(pmain->mutex_umrbus_clients));
    }
    
    /* former versions used to disable all interrupts here: cli(); */
    
    down(&(pmain->data.master.mutex_UMRBusInt));
    spin_lock_irqsave(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
    
    if((pmain->data.master.UMRBusIntQueue[clides->bus].first)) {
        if(WasIntFrame) {
            WasIntFrame=0;
            NextQueueItem=pmain->data.master.UMRBusIntQueue[clides->bus].first;
            while(NextQueueItem) {
                wake_up_interruptible(&(NextQueueItem->wait));
                NextQueueItem=NextQueueItem->next;
            }
        }
    }
    
    /* former versions used to enable all interrupts here: sti(); */
    
    spin_unlock_irqrestore(&(pmain->data.master.spinlock_UMRBusInt), irqflags);
    up(&(pmain->data.master.mutex_UMRBusInt));
    goto gosleep;

 exit:
    /* write back umrbus_interrupt_struct */
    if(copy_to_user(value, &umrint, sizeof(umrbus_interrupt_type))) {
        retvalue=-EFAULT;
    }

    UMRDPRINT(("     <<< umrdrv1_ioctl_interrupt"));
    return (retvalue);
}

/******************************************************************************
 * check UMR bus for valid data transfer using testframes
 *******************************************************************************/
static int umrdrv1_ioctl_testframe(struct file *file, void *value)
{
    int retvalue=0;
    u32 i;
    umrdrv1_umrbus_client_des_type *clides = (umrdrv1_umrbus_client_des_type *)file->private_data;
    umrdrv1_main_type *pmain;
    unsigned char *buffer, *vglbuffer;
    u32 parameters, header, framesize;
    u32 size;
    int umrbus;
#ifdef TESTFRAMEREPORT
#define ERROR_OUTPUT_RANGE 64
    unsigned long li, lli, start;
    u32 *outbuffer, *inbuffer;
#endif

    DPRINT(("     >>> umrdrv1_ioctl_testframe"));

    /* get pointer to main structure and check it */
    if(!(pmain=umrdrv1_pmain[clides->device])) {
	retvalue=-ENODEV;
	goto exit;
    }

    /* get parameters */
    if(copy_from_user(&parameters, value, sizeof(u32))) {
        retvalue=-EFAULT;
        goto exit;
    }
    size = (parameters & 0x0000FFFF) << 2;
    umrbus = (parameters >> 16);

    /* check for valid umrbus */
    if((umrbus >= pmain->number_umrbus) || (umrbus < 0)) {
	retvalue=-ENODEV;
	goto exit;
    }

    /* check send size */
    if(size<=0) {
	retvalue=0;
	goto exit;
    }

    DPRINT(("          framesize = %i Bytes, umrbus #%i, device #%i", size, umrbus, clides->device));

    /* allocate memory */
    if((buffer = (unsigned char *)kmalloc(pmain->max_framesize_bytes * sizeof(char),GFP_KERNEL)) == NULL) {
	retvalue=-ENOMEM;
	goto exit;
    }
    if((vglbuffer = (unsigned char *)kmalloc(pmain->max_framesize_bytes * sizeof(char),GFP_KERNEL)) == NULL) {
	kfree(buffer);
	retvalue=-ENOMEM;
	goto exit;
    }        

    /* built header */
    header  = UMRDRV1_UMRBUS_TEST_FRAME_HEADER;
    header |= ((umrbus << 1) & 0x0El);

    /* now loop until frame was processed */
    while(size) {
	/* generate test values */
      	for(i=0; i<pmain->max_framesize_bytes; i=i+4) {
            /* the 'original' implementation, using sort of pseudo-random values: */
            buffer[i]= buffer[i]+i*jiffies;
            buffer[i+1] = buffer[i+1]+(i+1)*jiffies;
            buffer[i+2] = buffer[i+2]+(i+2)*jiffies;
            buffer[i+3] = buffer[i+3]+(i+3)*jiffies;
            /* temporary alternative (was for debugging only!): ascendent sequence of test values: */
            /*
            buffer[i]= i/4 & 0x000000FF;
            buffer[i+1]= (i/4>>8) & 0x000000FF;
            buffer[i+2]= (i/4>>16) & 0x000000FF;
            buffer[i+3]= (i/4>>24) & 0x000000FF;
            */
        }
	
	/* save values to vglbuffer */
	memcpy(vglbuffer, buffer, pmain->max_framesize_bytes);
	
	/* check framesize */
	if(size > pmain->max_framesize_bytes)
	    framesize = pmain->max_framesize_bytes;
	else
	    framesize = size;

	/* set framesize in header */
	header &= 0x0000FFFF;
	header |= (((framesize >> 2) + 1) << 16);

	/* write frame */
	if((retvalue=(pmain->readwrite)(pmain, header, buffer, framesize, UMRDRV1_RWMODE_TEST)))
	    goto exit_free;

	/* compare received frame */
	for(i=0; i<framesize; i++)
	    buffer[i] = ~buffer[i];
        if((memcmp(vglbuffer,buffer,framesize))) {
#ifdef TESTFRAMEREPORT 
            outbuffer = (u32 *)vglbuffer;
            inbuffer  = (u32 *)buffer;

            for (li = 0; li < framesize/sizeof(u32); li ++)
                if (* (outbuffer + li) != * (inbuffer + li))
                    {
                        printk ("UMRDrv1: testframe : Data error at address 0x%08lx\n", li);
                    }

            for (li = 0; li < framesize/sizeof(u32); li ++)
                if (* (outbuffer + li) != * (inbuffer + li))
                    {
                        printk ("UMRDrv1: testframe : address             expected values                      received values          errors\n");
                        printk ("UMRDrv1: testframe : ----------------------------------------------------------------------------------------\n");
                        lli = start = (li < (ERROR_OUTPUT_RANGE >> 1) ? 0 : li - (ERROR_OUTPUT_RANGE >> 1)) & 0xFFFFFFF0;

                        while (lli < start + ERROR_OUTPUT_RANGE)
                            {
                                printk ("UMRDrv1: testframe : %08lx  ", lli);
                                for (i = 0; i < 4; i ++)
                                    if (lli + i < framesize/sizeof(u32)) 
                                        printk ("%08lx ", (unsigned long)* (outbuffer + lli + i));
                                    else
                                        printk ("         ");
                                printk (" ");
                                for (i = 0; i < 4; i ++)
                                    if (lli + i < framesize/sizeof(u32)) 
                                        printk ("%08lx ", (unsigned long)* (inbuffer + lli + i));
                                    else
                                        printk ("         ");
                                printk (" ");
                                for (i = 0; i < 4; i ++)
                                    if (lli + i < framesize/sizeof(u32)) 
                                        {
                                            if (* (outbuffer + lli + i) != * (inbuffer + lli + i))
                                                printk ("#");
                                            else
                                                printk ("-");
                                        }
                                    else
                                        printk ("         ");
                                printk ("\n");
                                lli += 4;
                                if ((lli & 0x0000000F) == 0)
                                    printk ("UMRDrv1: testframe : ----------------------------------------------------------------------------------------\n");
                                if (lli >= framesize/sizeof(u32)) break;
                            }
                        break;;
                    }            
#endif
            retvalue=-EBADE;
            goto exit_free;
        }
	size -= framesize;
    }

 exit_free:
    kfree(buffer);
    kfree(vglbuffer);

 exit:
    DPRINT(("     <<< umrdrv1_ioctl_testframe"));
    return (retvalue);
}

module_init(umrdrv1_init_module);
module_exit(umrdrv1_cleanup_module);

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  tab-width: 4
 * End:
 */
 
