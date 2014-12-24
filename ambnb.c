/*-
 * Copyright (c) 2009 Patrick Lamaiziere <patfbsd@davenulle.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Backlight driver for Apple MacbookPro based on a Nvidia or AMD Radeon graphic 
 * chipset
 * This should work on MacbookPro 1,2 MacBookPro 3,1 MacBookPro 3,2 and MacbookPro 4,2
 *
 * dmidecode:
 * Handle 0x0011, DMI type 1, 27 bytes
 * System Information
 *        Manufacturer: Apple Inc.
 *        Product Name: MacBookPro3,1
 *
 * Use the sysctl dev.ambnb.level to set the backlight level (0 <= level <= 15)
 *
 * Magic values (not the code) come from the Linux driver (mbp_nvidia_bl.c), thanks!
 *
 * ----------------------------------------------------------------------------
 * History:
 * - 12/23/14 : adding possibility to change the backlight for non priviledges user
 *              and changing branch in sysctl tree: hw -> dev (like asmc module)
 *              (dervishe@yahoo.fr)
 * - 07/17/09 : fix incorrect use of device_t in ambnb_identify() 
 *              (fix build on FreeBSD 8.0)
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <machine/pc/bios.h>

#define AMBNB_SMI_CTRL	0xb2
#define AMBNB_SMI_DATA	0xb3

/*
 * Device interface.
 */
static int ambnb_probe(device_t);
static int ambnb_attach(device_t);
static int ambnb_detach(device_t);

/*
 * AMBNB functions.
 */
static void ambnb_identify(driver_t *, device_t);
static void ambnb_set_backlight(int);
static int ambnb_get_backlight(void);
static int ambnb_sysctl_dev_ambnb_level(SYSCTL_HANDLER_ARGS);

static device_method_t ambnb_methods[] = {
	DEVMETHOD(device_identify,	ambnb_identify),
	DEVMETHOD(device_probe,		ambnb_probe),
	DEVMETHOD(device_attach,	ambnb_attach),
	DEVMETHOD(device_detach,	ambnb_detach),

	{0, 0},
};

static driver_t ambnb_driver = {
	"ambnb",
	ambnb_methods,
	0,		/* NB: no softc */
};

static devclass_t ambnb_devclass;

DRIVER_MODULE(ambnb, nexus, ambnb_driver, ambnb_devclass, 0, 0);
MODULE_VERSION(ambnb, 0);

SYSCTL_NODE(_dev, OID_AUTO, ambnb, CTLFLAG_RD, 0,
	"Apple MacBook Nvidia Backlight");
SYSCTL_PROC(_dev_ambnb, OID_AUTO, level, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_ANYBODY,
	0, sizeof(int), ambnb_sysctl_dev_ambnb_level, "I",
	"backlight level");

/* MacbookPro models */
static struct bios_oem bios_apple = {
	{ 0xe0000, 0xfffff },
	{
		{ "MacBookPro1,2", 0, 13 },
		{ "MacBookPro3,1", 0, 13 },
		{ "MacBookPro3,2", 0, 13 },
		{ "MacBookPro4,2", 0, 13 },
		{ NULL, 0, 0 },
	}
};

static void
ambnb_identify(driver_t *drv, device_t parent)
{
	/* NB: order 10 is so we get attached after h/w devices */
	if (device_find_child(parent, "ambnb", -1) == NULL &&
	    BUS_ADD_CHILD(parent, 10, "ambnb", -1) == 0)
		panic("ambnb: could not attach");
}

static int
ambnb_probe(device_t dev)
{
#define BIOS_OEM_MAXLEN 80
	static u_char bios_oem[BIOS_OEM_MAXLEN] = "\0";
	int r;

	/* Search the bios for suitable Apple MacBookPro */
	r = bios_oem_strings(&bios_apple, bios_oem, BIOS_OEM_MAXLEN);
	if (r > 0) {
		device_set_desc(dev, "Apple MacBook Nvidia/Radeon Backlight");
		return (BUS_PROBE_DEFAULT);
        }
	return (ENXIO);
}

static int
ambnb_attach(device_t dev)
{
	return (0);
}

static int
ambnb_detach(device_t dev)
{
	return (0);
}

static void
ambnb_set_backlight(int level)
{
	outb(AMBNB_SMI_DATA, 0x04 | (level << 4));
	outb(AMBNB_SMI_CTRL, 0xbf); 
}

static int
ambnb_get_backlight(void)
{
	int level;

	outb(AMBNB_SMI_DATA, 0x03);
	outb(AMBNB_SMI_CTRL, 0xbf);
	level = inb(AMBNB_SMI_DATA) >> 4;
	return (level);
}

static int
ambnb_sysctl_dev_ambnb_level(SYSCTL_HANDLER_ARGS)
{
	int error, level;

	level = ambnb_get_backlight();
	error = sysctl_handle_int(oidp, &level, 0, req);
	if (error == 0 && req->newptr != NULL) {
		if (level < 0 || level > 15)
			return (EINVAL);
		ambnb_set_backlight(level);
	}
	return (error);
}
