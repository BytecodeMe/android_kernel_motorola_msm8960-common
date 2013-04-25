/* hexagon/coresys/ram_console.c
 *
 * Copyright (C) 2013 Cotulla
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/console.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/ioport.h>

/*

Cotulla:
This file implements Android compatible ram console
without any system dependences. can be used very early during boot process.

*/

extern void flush_dcache_range(unsigned long start, unsigned long end);
void scr_fb_console_write(void *console, const char *s, unsigned int count);




struct ram_console_buffer 
{
	uint32_t    sig;
	uint32_t    start;
	uint32_t    size;
	uint8_t     data[0];
};

#define RAM_CONSOLE_SIG (0x43474244) /* DBGC */


static struct ram_console_buffer *ram_console_buffer;
static size_t ram_console_buffer_size;


static void ram_console_update(const char *s, unsigned int count)
{
	struct ram_console_buffer *buffer = ram_console_buffer;
	char *ptr = (char*)(buffer->data + buffer->start);

	while (count-- > 0)
	{
		iowrite8(*s, ptr);
		s++;
		ptr++;
	}
	
}


void ram_console_write_size(const char *s, unsigned count)
{
	int rem;
	struct ram_console_buffer *buffer = ram_console_buffer;

	if (count > ram_console_buffer_size) {
		s += count - ram_console_buffer_size;
		count = ram_console_buffer_size;
	}
	rem = ram_console_buffer_size - buffer->start;
	if (rem < count) {
		ram_console_update(s, rem);
		s += rem;
		count -= rem;
		iowrite32(0, &buffer->start);
		iowrite32(ram_console_buffer_size, &buffer->size);
	}
	ram_console_update(s, count);

	iowrite32(ioread32(&buffer->start) + count, &buffer->start);

	if (ioread32(&buffer->size) < ram_console_buffer_size) {
		iowrite32(ioread32(&buffer->size) + count, &buffer->size);
	}
}

static void ram_console_write(const char *s){
	unsigned int count = strlen(s);
	ram_console_write_size(s,count);
}	


static int ram_console_init(struct ram_console_buffer *buffer,  size_t buffer_size)
{
	char *ptr;
	int count;

	ram_console_buffer = buffer;
	ram_console_buffer_size = buffer_size - sizeof(struct ram_console_buffer);

	buffer->sig = RAM_CONSOLE_SIG;
	buffer->start = 0;
	buffer->size = 0;

#if 1
	ptr = (char*)buffer->data;
	count = ram_console_buffer_size;
	while (count-- > 0)
	{
		iowrite8(0, ptr);
		ptr++;
	}
#endif

	return 0;
}


static int ram_console_inited = 0;
static char strbuffer[512];

void my_out_va(const char* str, va_list va)
{
	if (ram_console_inited == 0)
	{
		ram_console_inited = 1;
		ram_console_init((struct ram_console_buffer *)0xbff00000, 0x00100000);
	}

	vsprintf(strbuffer, str, va);

	ram_console_write(strbuffer);
}

void my_out(const char *str, ...){
	va_list va;

	va_start(va, str);
	my_out_va(str, va);
	va_end(va);
}



