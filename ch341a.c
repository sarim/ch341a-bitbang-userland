/*
 * This file is part of the ch341prog project.
 *
 * Copyright (C) 2014 Pluto Yang (yangyj.ee@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#include "ch341a.h"

struct libusb_device_handle *dev_handle = NULL;

/* Configure CH341A, find the device and set the default interface. */
int32_t ch341a_configure(uint16_t vid, uint16_t pid)
{
    struct libusb_device *dev;
    int32_t ret;

    uint8_t  desc[0x12];

    if (dev_handle != NULL) {
        fprintf(stderr, "Call ch341Release before re-configure\n");
        return -1;
    }
    ret = libusb_init(NULL);
    if(ret < 0) {
        fprintf(stderr, "Couldn't initialise libusb\n");
        return -1;
    }

    #if LIBUSB_API_VERSION < 0x01000106
        libusb_set_debug(NULL, 3);
    #else
        libusb_set_option(NULL, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
    #endif

    if(!(dev_handle = libusb_open_device_with_vid_pid(NULL, vid, pid))) {
        fprintf(stderr, "Couldn't open device [%04x:%04x].\n", vid, pid);
        return -1;
    }

    if(!(dev = libusb_get_device(dev_handle))) {
        fprintf(stderr, "Couldn't get bus number and address.\n");
        goto close_handle;
    }

    // These functions aren't available in windows
    #ifndef _WIN32

    if(libusb_kernel_driver_active(dev_handle, 0)) {
        ret = libusb_detach_kernel_driver(dev_handle, 0);
        if(ret) {
            fprintf(stderr, "Failed to detach kernel driver: '%s'\n", strerror(-ret));
            goto close_handle;
        }
    }

    #endif

    ret = libusb_claim_interface(dev_handle, 0);

    if(ret) {
        fprintf(stderr, "Failed to claim interface 0: '%s'\n", strerror(-ret));
        goto close_handle;
    }

    ret = libusb_get_descriptor(dev_handle, LIBUSB_DT_DEVICE, 0x00, desc, 0x12);

    if(ret < 0) {
        fprintf(stderr, "Failed to get device descriptor: '%s'\n", strerror(-ret));
        goto release_interface;
    }

    printf("Device reported its revision [%d.%02d]\n", desc[12], desc[13]);
    return 0;
release_interface:
    libusb_release_interface(dev_handle, 0);
close_handle:
    libusb_close(dev_handle);
    dev_handle = NULL;
    return -1;
}

/* release libusb structure and ready to exit */
int32_t ch341a_release(void)
{
    if (dev_handle == NULL) return -1;
    libusb_release_interface(dev_handle, 0);
    libusb_close(dev_handle);
    libusb_exit(NULL);
    dev_handle = NULL;
    return 0;
}

/* Helper function for libusb_bulk_transfer, display error message with the caller name */
int32_t usb_transfer(const char * func, uint8_t type, uint8_t* buf, int len)
{
    int32_t ret;
    int transfered;
    if (dev_handle == NULL) return -1;
    ret = libusb_bulk_transfer(dev_handle, type, buf, len, &transfered, DEFAULT_TIMEOUT);
    if (ret < 0) {
        fprintf(stderr, "%s: Failed to %s %d bytes '%s'\n", func,
                (type == BULK_WRITE_ENDPOINT) ? "write" : "read", len, strerror(-ret));
        return -1;
    }
    return transfered;
}

/* swap LSB MSB if needed */
uint8_t swap_byte(uint8_t c)
{
    uint8_t result=0;
    for (int i = 0; i < 8; ++i)
    {
        result = result << 1;
        result |= (c & 1);
        c = c >> 1;
    }
    return result;
}

int32_t ch341a_gpio_configure(uint8_t* dir_mask)
{
    uint8_t buf[3];

    *dir_mask &= 0b00111111; // make sure last two bits are INPUT

    buf[0] = CH341A_CMD_UIO_STREAM;
    buf[1] = CH341A_CMD_UIO_STM_DIR | *dir_mask;
    buf[2] = CH341A_CMD_UIO_STM_END;

    return usb_transfer(__func__, BULK_WRITE_ENDPOINT, buf, 3);
}

int32_t ch341a_gpio_instruct(uint8_t* dir_mask, uint8_t* data)
{
    int32_t ret;
    uint8_t buf[5];

    buf[0] = CH341A_CMD_UIO_STREAM;
    buf[1] = CH341A_CMD_UIO_STM_OUT | (*data & *dir_mask);
    buf[2] = CH341A_CMD_UIO_STM_IN;
    buf[3] = CH341A_CMD_UIO_STM_END;

    ret = usb_transfer(__func__, BULK_WRITE_ENDPOINT, buf, 4);
    if (ret < 0) return -1;

    ret = usb_transfer(__func__, BULK_READ_ENDPOINT, buf, 1);
    if (ret < 0) return -1;
    *data &= *dir_mask;
    *data |= buf[0] & ~*dir_mask;
    return ret;
}