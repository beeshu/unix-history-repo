#!/bin/sh
# This writes a skeleton driver and puts it into the kernel tree for you
#arg1 is lowercase "foo"
#
# Trust me, RUN THIS SCRIPT :)
#
# $FreeBSD$
#
#-------cut here------------------
cd /sys/i386/conf

if [ "${1}X" = "X" ]
then
	echo "Hey , how about some help here.. give me a device name!"
	exit 1
fi

UPPER=`echo ${1} |tr "[:lower:]" "[:upper:]"`
cat >files.${UPPER} <<DONE
dev/${1}.c      optional ${1} device-driver
DONE

cat >${UPPER} <<DONE
# Configuration file for kernel type: ${UPPER}
ident	${UPPER}
# \$FreeBSD\$
DONE

grep -v GENERIC < GENERIC >>${UPPER}

cat >>${UPPER} <<DONE
# trust me, you'll need this
options	DDB
device	${1}
DONE

cat >../../dev/${1}.c <<DONE
/*
 * Copyright (c) [year] [your name]
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
 * ${1} driver
 */

#include <sys/cdefs.h>
__FBSDID("\$FreeBSD\$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>		/* SYSINIT stuff */
#include <sys/uio.h>		/* SYSINIT stuff */
#include <sys/conf.h>		/* cdevsw stuff */
#include <sys/malloc.h>		/* malloc region definitions */
#include <sys/proc.h>
#include <sys/${1}io.h>		/* ${1} IOCTL definitions */

#include <machine/clock.h>	/* DELAY() */

#define N${UPPER}	3	/* defines number of instances */

/* XXX These should be defined in terms of bus-space ops. */
#define ${UPPER}_INB(port) inb(port)
#define ${UPPER}_OUTB(port, val) (port, (val))

/* Function prototypes (these should all be static) */
static  d_open_t	${1}open;
static  d_close_t	${1}close;
static  d_read_t	${1}read;
static  d_write_t	${1}write;
static  d_ioctl_t	${1}ioctl;
static  d_mmap_t	${1}mmap;
static  d_poll_t	${1}poll;

#define CDEV_MAJOR 20
static struct cdevsw ${1}_cdevsw = {
	.d_version =	D_VERSION,
	.d_open =	${1}open,
	.d_close =	${1}close,
	.d_read =	${1}read,
	.d_write =	${1}write,
	.d_ioctl =	${1}ioctl,
	.d_poll =	${1}poll,
	.d_mmap =	${1}mmap,
	.d_name =	"${1}",
};

/*
 * device  specific Misc defines
 */
#define BUFFERSIZE 1024
#define UNIT(dev) minor(dev)	/* assume one minor number per unit */

/*
 * One of these per allocated device
 */
struct ${1}_softc {
	u_long	iobase;
	char	buffer[BUFFERSIZE];
  	struct cdev *dev;
};

typedef	struct ${1}_softc *sc_p;

static sc_p sca[N${UPPER}];

/*
 * Macro to check that the unit number is valid
 * Often this isn't needed as once the open() is performed,
 * the unit number is pretty much safe.. The exception would be if we
 * implemented devices that could "go away". in which case all these routines
 * would be wise to check the number, DIAGNOSTIC or not.
 */
#define CHECKUNIT(RETVAL)						\
do { /* the do-while is a safe way to do this grouping */		\
	if (unit > N${UPPER}) {						\
		printf("%s: bad unit %d\n", __func__, unit);		\
		return (RETVAL);					\
	}								\
	if (scp == NULL) { 						\
		printf("%s: unit %d not attached\n", __func__, unit);	\
		return (RETVAL);					\
	}								\
} while (0)

#ifdef	DIAGNOSTIC
#define	CHECKUNIT_DIAG(RETVAL) CHECKUNIT(RETVAL)
#else	/* DIAGNOSTIC */
#define	CHECKUNIT_DIAG(RETVAL)
#endif 	/* DIAGNOSTIC */

static int
${1}ioctl (struct cdev *dev, u_long cmd, caddr_t data, int flag, struct thread *td)
{
	int unit = UNIT(dev);
	sc_p scp  = sca[unit];

	CHECKUNIT_DIAG(ENXIO);

	switch (cmd) {
	    case DHIOCRESET:
		/*  whatever resets it */
		(void)scp; /* Delete this line after using scp. */
#if 0
		${UPPER}_OUTB(scp->iobase, 0xff);
#endif
		break;
	    default:
		return ENXIO;
	}
	return (0);
}

/*
 * You also need read, write, open, close routines.
 * This should get you started
 */
static int
${1}open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
	int unit = UNIT(dev);
	sc_p scp  = sca[unit];

	CHECKUNIT(ENXIO);

	(void)scp; /* Delete this line after using scp. */
	/*
	 * Do processing
	 */
	return (0);
}

static int
${1}close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
	int unit = UNIT(dev);
	sc_p scp  = sca[unit];

	CHECKUNIT_DIAG(ENXIO);

	(void)scp; /* Delete this line after using scp. */
	/*
	 * Do processing
	 */
	return (0);
}

static int
${1}read(struct cdev *dev, struct uio *uio, int ioflag)
{
	int unit = UNIT(dev);
	sc_p scp  = sca[unit];
	int     toread;


	CHECKUNIT_DIAG(ENXIO);

	/*
	 * Do processing
	 * read from buffer
	 */
	toread = (min(uio->uio_resid, sizeof(scp->buffer)));
	return(uiomove(scp->buffer, toread, uio));
}

static int
${1}write(struct cdev *dev, struct uio *uio, int ioflag)
{
	int unit = UNIT(dev);
	sc_p scp  = sca[unit];
	int	towrite;

	CHECKUNIT_DIAG(ENXIO);

	/*
	 * Do processing
	 * write to buffer
	 */
	towrite = (min(uio->uio_resid, sizeof(scp->buffer)));
	return(uiomove(scp->buffer, towrite, uio));
}

static int
${1}mmap(struct cdev *dev, vm_offset_t offset, vm_paddr_t *paddr, int nprot)
{
	int unit = UNIT(dev);
	sc_p scp  = sca[unit];

	CHECKUNIT_DIAG(-1);

	(void)scp; /* Delete this line after using scp. */
	/*
	 * Do processing
	 */
#if 0	/* if we had a frame buffer or whatever.. do this */
	if (offset > FRAMEBUFFERSIZE - PAGE_SIZE) {
		return (-1);
	}
	return i386_btop((FRAMEBASE + offset));
#else
	return (-1);
#endif
}

static int
${1}poll(struct cdev *dev, int which, struct thread *td)
{
	int unit = UNIT(dev);
	sc_p scp  = sca[unit];

	CHECKUNIT_DIAG(ENXIO);

	(void)scp; /* Delete this line after using scp. */
	/*
	 * Do processing
	 */
	return (0); /* this is the wrong value I'm sure */
}

/*
 * Now  for some driver initialisation.
 * Occurs ONCE during boot (very early).
 */
static void
${1}_drvinit(void *unused)
{
	int	unit;
	sc_p scp  = sca[unit];

	for (unit = 0; unit < N${UPPER}; unit++) {
		/*
		 * Allocate storage for this instance .
		 */
		scp = malloc(sizeof(*scp), M_DEVBUF, M_NOWAIT | M_ZERO);
		if( scp == NULL) {
			printf("${1}%d failed to allocate strorage\n", unit);
			return;
		}
		sca[unit] = scp;
    		scp->dev = make_dev(&${1}_cdevsw, unit,
			UID_ROOT, GID_KMEM, 0640, "${1}%d", unit);
	}
}

SYSINIT(${1}dev, SI_SUB_DRIVERS, SI_ORDER_MIDDLE+CDEV_MAJOR,
		${1}_drvinit, NULL)
DONE

cat >../../sys/${1}io.h <<DONE
/*
 * Definitions needed to access the ${1} device (ioctls etc)
 * see mtio.h , ioctl.h as examples
 */
#ifndef SYS_DHIO_H
#define SYS_DHIO_H

#ifndef KERNEL
#include <sys/types.h>
#endif
#include <sys/ioccom.h>

/*
 * define an ioctl here
 */
#define DHIOCRESET _IO('D', 0)   /* reset the ${1} device */
#endif
DONE

config ${UPPER}
cd ../../compile/${UPPER}
make depend
make ${1}.o
make
exit

#--------------end of script---------------
#
#you also need to add an entry into the cdevsw[]
#array in conf.c, but it's too hard to do in a script..
#
#edit to your taste..
#
#
