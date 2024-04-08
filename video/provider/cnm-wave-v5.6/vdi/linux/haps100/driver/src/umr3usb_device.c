/*_____________________________________________________________
 |                                                             |
 |      UMRBus 3.0 Kernel Mode Device Driver for Linux         |
 |         Copyright (C) 2001 - 2020 Synopsys, Inc.            |
 |_____________________________________________________________|
 |                                                             |
 | This program is free software: you can redistribute it      |
 | and/or modify it under the terms of the GNU General Public  |
 | License as published by the Free Software Foundation,       |
 | either version 3 of the License, or any later version.      |
 |                                                             |
 | This program is distributed in the hope that it will be     |
 | useful, but WITHOUT ANY WARRANTY; without even the implied  |
 | warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR     |
 | PURPOSE.  See the GNU General Public License for more       |
 | details.                                                    |
 |                                                             |
 | You should have received a copy of the GNU General Public   |
 | License along with this program.  If not, see               |
 |                                                             |
 |             <https://www.gnu.org/licenses/>                 |
 |_____________________________________________________________|
 |                                                             |
 | FILE NAME   : umr3pci_device.c                              |
 |____________________________________________________________*/

#include "umr3_driver_global.h"

static inline int umr3usb_start_threads(umr3_device *device) {
  int st = UMR3_STATUS_SUCCESS;

  device->us_thread = NULL;
  device->threads_en = 0;

  if ((device->us_thread = kthread_run(umr3_us_thread, (void* ) device, UMR3_DEVICE_NAME "%d_us_thread", device->minor)) == ERR_PTR(-ENOMEM)) {
    device->us_thread = NULL;
    umr3_log_device_error("Unable to start upstream thread.");
    return UMR3_STATUS_OUT_OF_MEMORY;
  }

  device->threads_en = 1;

  return st;
}

static inline void umr3usb_stop_threads(umr3_device *device) {
  if(device->threads_en) {
    if (device->us_thread) {
      umr3_log_device_info("Stopping kernel threads.")
      kthread_stop(device->us_thread);
    }
  }
}

int umr3_device_enable(umr3_device *device) {
  int st = UMR3_STATUS_SUCCESS;
  uint32_t config[UMR3_USB_STATUS_REG_MAX];
  uint32_t version, sparam;
  umr3_hctl hctl;

  INIT_LIST_HEAD(&device->fctx);
  umr3_mutex_init(device->tree);
  umr3_spin_lock_init(device->urb_lock);
  umr3_spin_lock_init(device->lock);

  atomic_set(&device->registered, 1);
  atomic_set(&device->freed, 0);

  if_umr3_failed(st, umr3_allocate_buffers(device, UMR3_MAX_URB_BUFFERS)) {
    return st;
  }

  umr3_init_lists_usb(device);

  if_umr3_failed(st, umr3usb_start_threads(device)) {
    goto exit;
  }

  if_umr3_failed(st, umr3_reset_him(device)) {
    umr3_log_device_failed("umr3_reset_him", st);
    goto exit;
  }

  if_umr3_failed(st, umr3_reg_read_size_retries(device, UMR3_USB_STATUS_VERSION, UMR3_USB_STATUS_REG_MAX, &config[0], device->timeout * UMR3_RETRY_COUNT)) {
    umr3_log_device_error("umr3_oc_h_reg_retries_usb timedout.");
  }

  version = config[UMR3_USB_STATUS_VERSION];
  device->him_major = HIM_VERSION_MAJOR(version);
  device->him_minor = HIM_VERSION_MINOR(version);

  // active reset
  if(UMR3_GET_BITS(config[UMR3_USB_STATUS_HSTAT], 0x1, 4) == 1) {
    st = UMR3_STATUS_NOT_POSSIBLE;
    umr3_log_device_error("UMRBus 3.0 reset is active. Communication is not possible.");
    goto exit;
  }

  if_umr3_failed(st, umr3_check_clocks(device, config[UMR3_USB_STATUS_CLKCFG])) {
    umr3_log_device_failed("umr3_check_clocks", st);
    goto exit;
  }

  sparam = config[UMR3_USB_STATUS_SPARAM];
  umr3_log_device_info("UMRBus 3.0 has a bus vector of %d bits.", 1 << (UMR3_GET_BITS(sparam, 0x3, 0) + 5));

  // start the HIM
  if_umr3_failed(st, umr3_reg_read_retries(device, UMR3_USB_STATUS_HCTL, &config[0], device->timeout * UMR3_RETRY_COUNT)) {
    umr3_log_device_error("umr3_reg_read_retries timedout.");
    goto exit;
  }

  hctl.value = config[0];
  hctl.IPEN = 1;
  hctl.MPEN = 1;
  hctl.HPEN = 1;
  hctl.HIMPRC = 1;
  if_umr3_failed(st, umr3_reg_write(device, UMR3_USB_STATUS_HCTL, hctl.value)) {
    umr3_log_device_failed("umr3_reg_write", st);
  }

  if_umr3_failed(st, umr3_reg_read_retries(device, UMR3_USB_STATUS_HCTL, &config[0], device->timeout * UMR3_RETRY_COUNT)) {
    umr3_log_device_error("umr3_reg_read_retries timedout.");
    goto exit;
  }
  device->hctl = hctl.value = config[0];

  if(hctl.HPEN && hctl.MPEN && hctl.IPEN) {
    umr3_log_device_info("%s device enabled. HIM: %d.%d Driver: " UMR3_DRIVER_VERSION " Interface: " UMR3_DRIVER_INTERFACE_VERSION " Bus: %04d Device: %04d.", umr3_string_usb_speed(device->ep->speed), device->him_major, device->him_minor, device->ep->bus->busnum, device->ep->devnum);
  } else {
    umr3_log_device_error("Interrupt/Master/Host processes not enabled on the HIM.");
    st = UMR3_STATUS_FAILED;
    goto exit;
  }

  return st;

exit:
  if(st != UMR3_STATUS_SUCCESS) {
    atomic_set(&device->registered, 0);
  }
  umr3_log_device_error("Device could not be initialized.");
  return st;
}

int umr3_device_disable(umr3_device *device) {
  int st = UMR3_STATUS_SUCCESS;
  int st_tmp;

  st_tmp = umr3_reg_write(device, UMR3_USB_STATUS_HCTL, 0x0);

  return st;
}

int umr3_device_cleanup(umr3_device *device) {
  int st = UMR3_STATUS_SUCCESS;

  //umr3_disable_watchdog(device);
  umr3usb_stop_threads(device);
  umr3_free_buffers(device);
  umr3_clear_rw_all_queues(device);
  umr3_free_scan_payload_list(device);

  //usb_put_intf(device->interface);
  usb_put_dev(device->ep);

  return st;
}
