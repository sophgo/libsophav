/*
 *
 *       UMRBus PCI(e) Kernel Mode Device Driver for Linux
 *
 *            Copyright (C) 2001 - 2014 Synopsys, Inc.
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

#ifndef __umrdrv1_h__
#define __umrdrv1_h__

/* **************************************************
   driver specific stuff
   ************************************************** */
#define UMRDRV1_MAX_DEVICES       4

#define UMRDRV1_REVISION_MAJOR    1
#define UMRDRV1_REVISION_MINOR    1 
#define UMRDRV1_REVISION          ((UMRDRV1_REVISION_MAJOR << 16) | UMRDRV1_REVISION_MINOR)

/* **************************************************
   supported PCI boards (vendor and device IDs)
   ************************************************** */
#define PCI_VENDOR_ID_SYNOPSYS                        0x1482
#define PCI_DEVICE_ID_SYNOPSYS_PCIMASTER              0x2000
#define PCI_DEVICE_ID_SYNOPSYS_PCIEXPRESS             0x3000
#define PCI_DEVICE_ID_SYNOPSYS_HAPS31                 0x3100
#define PCI_DEVICE_ID_SYNOPSYS_DIRECT_UMR_HAPS70      0x4000
#define PCI_DEVICE_ID_SYNOPSYS_DIRECT_UMR_HAPSDX7_DWC 0x4001
#define PCI_DEVICE_ID_SYNOPSYS_DIRECT_UMR_HAPSDX7     0x4002
#define PCI_DEVICE_ID_SYNOPSYS_DIRECT_UMR_HAPS80      0x4003

/* **************************************************
   master specific stuff
   ************************************************** */

/* device address offsets */
#define UMRDRV1_MASTER_INTREGOFFSET          0x00
#define UMRDRV1_MASTER_MASKREGOFFSET         0x01
#define UMRDRV1_MASTER_UMRNUMBREGOFFSET      0x02
#define UMRDRV1_MASTER_MASTERADDRESSOFFSET   0x03
#define UMRDRV1_MASTER_UMRHEADEROFFSET       0x04
#define UMRDRV1_MASTER_DMAADDRESSOFFSET      0x05
#define UMRDRV1_MASTER_DMACONFIGOFFSET       0x06
#define UMRDRV1_MASTER_MEMSIZEOFFSET         0x08
                                      
/* device address ranges */           
#define UMRDRV1_MASTER_ADDRESSSPACE          0x1000L
#define UMRDRV1_MASTER_MEMSIZE               512

/* valid interrupts 
- Bit  0 = Write-Puffer Interrupt
- Bit  1 = Read-Puffer Interrupt
- Bit  2 = UmrBus "0" Interrupt

the interrups are depending on the number of
UMRBusses the device provides
*/
#define UMRDRV1_MASTER_MASK_INT_WR             0x00000001
#define UMRDRV1_MASTER_MASK_INT_RD             0x00000002
#define UMRDRV1_MASTER_MASK_INT_UB             0x000003FC
#define UMRDRV1_MASTER_MASK_INT_UB0            0x00000004

#define UMRDRV1_MASTER_INT_INVALID             0
#define UMRDRV1_MASTER_INT_WRITE               1
#define UMRDRV1_MASTER_INT_READ                2
#define UMRDRV1_MASTER_INT_UMRBUS              3

#define UMRDRV1_MASTER_TIMEOUT                 5
#define UMRDRV1_MASTER_TIMEOUT_IO              1

/* **************************************************
   UMRBus specific stuff
   ************************************************** */
struct umrbus_interrupt_struct
{
    int mode;
    int interrupt;
    int timeout;
    int capiinttype;
};
typedef struct umrbus_interrupt_struct umrbus_interrupt_type;

#define UMRDRV1_MASK_UMR_PRESENT      0x00000001
#define UMRDRV1_MASK_UMR_CONNECTED    0x00000002
#define UMRDRV1_MASK_UMR_DISCONNECTED 0x00000004
#define UMRDRV1_MASK_UMR_INTERRUPT    0x00000008
#define UMRDRV1_MASK_UMR_NODEV        0x00000000
#define UMRDRV1_MASK_UMR_CLITYPE      0xFFFF0000
#define UMRDRV1_MASK_UMR_ADDRESS      0x0000007F

#define UMRDRV1_UMRBUS_SCAN_FRAME_HEADER      0x00410060
#define UMRDRV1_UMRBUS_SCAN_FRAME_LENGTH      64
                                      
#define UMRDRV1_UMRBUS_INT_FRAME_HEADER       0x004100A0
#define UMRDRV1_UMRBUS_INT_FRAME_LENGTH       64
                                      
#define UMRDRV1_UMRBUS_INT_MODE_POLLING       0
#define UMRDRV1_UMRBUS_INT_MODE_WAITEVENT     1
                                      
#define UMRDRV1_UMRBUS_TEST_FRAME_HEADER      0x00000030
                                      
#define UMRDRV1_UMRBUS_READ_FRAME_HEADER      0x00000020
#define UMRDRV1_UMRBUS_WRITE_FRAME_HEADER     0x00000010

/* error definitions */
#define UMRDRV1_UMRBUS_SUCCESS       0
#define UMRDRV1_UMRBUS_EFRMLEN       1
#define UMRDRV1_UMRBUS_EREQADR       2
#define UMRDRV1_UMRBUS_ECLIADR       4
#define UMRDRV1_UMRBUS_EDISCON       8

/* modes for ReadWrite routine */
#define UMRDRV1_RWMODE_READ          0
#define UMRDRV1_RWMODE_SCAN          1
#define UMRDRV1_RWMODE_WRITE         2
#define UMRDRV1_RWMODE_TEST          3
#define UMRDRV1_RWMODE_INTF          4

#define UMRDRV1_MAX_UMRBUS                    8     // the maximum UMR buses a device can have
#define UMRDRV1_MAX_UMRBUS_CLIENTS            63    // the maximum CAPIMs a UMR bus can have

#define UMRDRV1_PCIE_DEVICE_CONTROL_REGISTER_OFFSET 0x78

typedef struct umrdrv1_umrbus_client_des_struct {
    s32 device;
    s32 bus;
    s32 address;
} umrdrv1_umrbus_client_des_type;

typedef struct umrdrv_umrbus_client_struct {
    unsigned int type;                              // CAPIM type
    unsigned int capiinttype;                       // CAPIM interrupt type
    int flags;
    /* flags - Bit 0 = present (if avaliable = 1)
             - Bit 1 = connected (if connection opened = 1)
             - Bit 2 = disconnected (if after scan not present or changed but still connected = 1) 
             - Bit 3 = interrupt (if interrupt request = 1) */
} umrdrv_umrbus_client_type;

/* readwrite routine function pointer definition */ 
typedef struct umrdrv1_main_struct umrdrv1_main_type; 
typedef int (* umrdrv1_readwrite_routine)(umrdrv1_main_type *, u32, char *, u32, int); 

/* wait queue wrapper kernel version 2.2/2.4 */
#ifndef DECLARE_WAIT_QUEUE_HEAD
   typedef struct wait_queue *wait_queue_head_t;
#  define init_waitqueue_head(head) (*(head)) = NULL
#endif

/* Queue structures and macros*/
struct struct_QueueItem {
    char TrueWakeUpMQ;
    wait_queue_head_t wait;
    struct struct_QueueItem  *prev;
    struct struct_QueueItem  *next;
};

struct struct_MainQueue {
    struct struct_QueueItem  *first;
    struct struct_QueueItem  *last;
};

#define queue_item_init(cq) \
  init_waitqueue_head(&(cq.wait)); \
  cq.TrueWakeUpMQ = 0; \
  cq.prev = NULL; \
  cq.next = NULL

#define queue_item_add(cq,mq) \
  cq.prev = mq.last; \
  if(mq.last) mq.last->next = &cq; \
  mq.last = &cq; \
  if(!(mq.first)) mq.first = &cq

#define queue_item_del(cq,mq) \
  if(!(cq.prev) && !(cq.next)) {mq.first = NULL; mq.last= NULL;} \
  if(!(cq.prev) && (cq.next)) {mq.first = cq.next; cq.next->prev= NULL;} \
  if((cq.prev) && !(cq.next)) {mq.last = cq.prev; cq.prev->next= NULL;} \
  if((cq.prev) && (cq.next)) {cq.prev->next = cq.next; cq.next->prev=cq.prev;}

/* **************************************************
   the main device data structure 
   ************************************************** */

struct umrdrv1_main_struct {
    struct pci_dev              *dev;
    int                         dev_number;
    int                         number_umrbus;
    int                         low_latency_mode_available;
    int                         master_memory_size_available;
    unsigned int                interrupt_mask;
    umrdrv_umrbus_client_type   umrbus_clients[UMRDRV1_MAX_UMRBUS][UMRDRV1_MAX_UMRBUS_CLIENTS];
    struct semaphore            mutex_umrbus_clients;
    u32                         max_framesize_bytes;
    umrdrv1_readwrite_routine   readwrite;
    union {
	struct {
            u32                     MemBaseAddress;
            u8                      RevisionID;
            u32                     *MemAddress;
            volatile u32            *IntRegAddress;
            u32                     *MaskRegAddress;
            u32                     *UMRNumRegAddress;
            u32                     *MasterAddressRegAddress;
            u32                     *UMRBusHeaderRegAddress;
            u32                     *MasterMemorySize;
	    s32                     ISRMode;
	    s32                     UMRBusInt;
	    s32                     TrueWakeUpRWQ;
	    struct struct_MainQueue MainQueue;
	    struct struct_MainQueue UMRBusIntQueue[UMRDRV1_MAX_UMRBUS];
	    wait_queue_head_t       RWQueue;
	    u32                     *DataBuffer;
	    struct semaphore        mutex_MainQueue;     /* mutex for MainQueue access */
	    struct semaphore        mutex_TrueWakeUpRWQ;
	    struct semaphore        mutex_UMRBusInt; /* mutex for UMRBusInt access */
	    spinlock_t              spinlock_TrueWakeUpRWQ;
	    spinlock_t              spinlock_UMRBusInt;
	} master;
	struct {
	    s32 dummy;
	} slave;
    } data;
};

/* **************************************************
   ioctrl's to access the driver
   ************************************************** */
#define IOCTL_UMRDRV_GET_REVISION       _IOR(UMRDRV1_MAJOR,0,u32)
#define IOCTL_UMRDRV_CLIENT_OPEN        _IOW(UMRDRV1_MAJOR,1,u32)
#define IOCTL_UMRDRV_INTERRUPT          _IOR(UMRDRV1_MAJOR,2,u32)
#define IOCTL_UMRDRV_TESTFRAME          _IOW(UMRDRV1_MAJOR,3,u32)
#define IOCTL_UMRDRV_SCAN               _IOR(UMRDRV1_MAJOR,4,u32)

/* **************************************************
   compatibility stuff
   ************************************************** */

/* Kernel 2.6 defines wait_event_interruptible_timeout in <linux/wait.h>, 
   whereas kernel 2.4 does not.
   The umrdrv_wait_event_interruptible_timeout 'hack' is based on
   wait_event_interruptible_timeout, but with the signal handling
   stuff being removed (because this led to the system hang up, if
   you hit Ctrl-c in a constellation where the macro would return
   with timeout otherwise). Signal handling is performed in the
   umrlib anyway..
*/
#ifndef __umrdrv_wait_event_interruptible_timeout
#define __umrdrv_wait_event_interruptible_timeout(wq, mutex, condition, ret) \
do {									\
	wait_queue_t __wait;                                \
    int __cond;                                         \
    u32 _ret, ___ret;                                    \
    ___ret = ret;                                        \
	init_waitqueue_entry(&__wait, current);				\
	add_wait_queue(&wq, &__wait);                       \
	for (;;) {                                          \
        if (down_interruptible(&(mutex))) {             \
            current->state = TASK_RUNNING;              \
            remove_wait_queue(&wq, &__wait);            \
            return -ERESTARTSYS;                        \
        }                                               \
        __cond = (condition);                           \
        up(&(mutex));                                   \
		set_current_state(TASK_INTERRUPTIBLE);			\
		if (__cond)                                     \
			break;                                      \
        _ret = schedule_timeout(ret/10);                   \
        if (!_ret) {                                    \
            if (down_interruptible(&(mutex))) {             \
                current->state = TASK_RUNNING;              \
                remove_wait_queue(&wq, &__wait);            \
                return -ERESTARTSYS;                        \
            }                                               \
            __cond = (condition);                           \
            up(&(mutex));                                   \
            if (__cond) {                                   \
                ret = 1;                                    \
                break;                                      \
            }                                               \
            ___ret = ___ret - (ret / 10);                     \
            if (!___ret) {                                   \
                ret = 0;                                    \
                break;                                      \
            }                                               \
        }                                                   \
        continue;                                           \
    }                                                       \
    current->state = TASK_RUNNING;                          \
    remove_wait_queue(&wq, &__wait);                        \
} while (0)
#endif

#ifndef umrdrv_wait_event_interruptible_timeout
#define umrdrv_wait_event_interruptible_timeout(wq, mutex, condition, timeout) \
({                                                                  \
    long __ret = timeout;                                               \
    int __cond;                                                         \
    if (down_interruptible(&(mutex)))                                   \
        return -ERESTARTSYS;                                            \
    __cond = (condition);                                               \
    up(&(mutex));                                                       \
    if (!__cond)                                                        \
		__umrdrv_wait_event_interruptible_timeout(wq, mutex, condition, __ret); \
	__ret;								\
})
#endif

#endif
 
