/* $FreeBSD$ */

/*	$NetBSD: ccd.c,v 1.22 1995/12/08 19:13:26 thorpej Exp $	*/

/*
 * Copyright (c) 1995 Jason R. Thorpe.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project
 *	by Jason R. Thorpe.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: cd.c 1.6 90/11/28$
 *
 *	@(#)cd.c	8.2 (Berkeley) 11/16/93
 */

/*
 * "Concatenated" disk driver.
 *
 * Dynamic configuration and disklabel support by:
 *	Jason R. Thorpe <thorpej@nas.nasa.gov>
 *	Numerical Aerodynamic Simulation Facility
 *	Mail Stop 258-6
 *	NASA Ames Research Center
 *	Moffett Field, CA 94035
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/bio.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/disklabel.h>
#include <ufs/ffs/fs.h> 
#include <sys/devicestat.h>
#include <sys/fcntl.h>
#include <sys/vnode.h>

#include <sys/ccdvar.h>

MALLOC_DEFINE(M_CCD, "CCD driver", "Concatenated Disk driver");

#if defined(CCDDEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#ifdef DEBUG
#define CCDB_FOLLOW	0x01
#define CCDB_INIT	0x02
#define CCDB_IO		0x04
#define CCDB_LABEL	0x08
#define CCDB_VNODE	0x10
static int ccddebug = CCDB_FOLLOW | CCDB_INIT | CCDB_IO | CCDB_LABEL |
    CCDB_VNODE;
SYSCTL_INT(_debug, OID_AUTO, ccddebug, CTLFLAG_RW, &ccddebug, 0, "");
#endif

#define	ccdunit(x)	dkunit(x)
#define ccdpart(x)	dkpart(x)

/*
   This is how mirroring works (only writes are special):

   When initiating a write, ccdbuffer() returns two "struct ccdbuf *"s
   linked together by the cb_mirror field.  "cb_pflags &
   CCDPF_MIRROR_DONE" is set to 0 on both of them.

   When a component returns to ccdiodone(), it checks if "cb_pflags &
   CCDPF_MIRROR_DONE" is set or not.  If not, it sets the partner's
   flag and returns.  If it is, it means its partner has already
   returned, so it will go to the regular cleanup.

 */

struct ccdbuf {
	struct bio	cb_buf;		/* new I/O buf */
	struct bio	*cb_obp;	/* ptr. to original I/O buf */
	struct ccdbuf	*cb_freenext;	/* free list link */
	int		cb_unit;	/* target unit */
	int		cb_comp;	/* target component */
	int		cb_pflags;	/* mirror/parity status flag */
	struct ccdbuf	*cb_mirror;	/* mirror counterpart */
};

/* bits in cb_pflags */
#define CCDPF_MIRROR_DONE 1	/* if set, mirror counterpart is done */

#define CCDLABELDEV(dev)	\
	(makedev(major((dev)), dkmakeminor(ccdunit((dev)), 0, RAW_PART)))

/* convinient macros for often-used statements */
#define IS_ALLOCATED(unit)	(ccdfind(unit) != NULL)
#define IS_INITED(cs)		(((cs)->sc_flags & CCDF_INITED) != 0)

static d_open_t ccdopen;
static d_close_t ccdclose;
static d_strategy_t ccdstrategy;
static d_ioctl_t ccdioctl;
static d_dump_t ccddump;
static d_psize_t ccdsize;

#define NCCDFREEHIWAT	16

#define CDEV_MAJOR 74

static struct cdevsw ccd_cdevsw = {
	/* open */	ccdopen,
	/* close */	ccdclose,
	/* read */	physread,
	/* write */	physwrite,
	/* ioctl */	ccdioctl,
	/* poll */	nopoll,
	/* mmap */	nommap,
	/* strategy */	ccdstrategy,
	/* name */	"ccd",
	/* maj */	CDEV_MAJOR,
	/* dump */	ccddump,
	/* psize */	ccdsize,
	/* flags */	D_DISK,
};
static LIST_HEAD(, ccd_s) ccd_softc_list = LIST_HEAD_INITIALIZER(&ccd_softc_list);

static struct ccd_s *ccdfind(int);
static struct ccd_s *ccdnew(int);
static int ccddestroy(struct ccd_s *, struct proc *);

/* called during module initialization */
static void ccdattach(void);
static int ccd_modevent(module_t, int, void *);

/* called by biodone() at interrupt time */
static void ccdiodone(struct bio *bp);

static void ccdstart(struct ccd_s *, struct bio *);
static void ccdinterleave(struct ccd_s *, int);
static void ccdintr(struct ccd_s *, struct bio *);
static int ccdinit(struct ccd_s *, char **, struct thread *);
static int ccdlookup(char *, struct thread *p, struct vnode **);
static void ccdbuffer(struct ccdbuf **ret, struct ccd_s *,
		      struct bio *, daddr_t, caddr_t, long);
static void ccdgetdisklabel(dev_t);
static void ccdmakedisklabel(struct ccd_s *);
static int ccdlock(struct ccd_s *);
static void ccdunlock(struct ccd_s *);

#ifdef DEBUG
static void printiinfo(struct ccdiinfo *);
#endif

/* Non-private for the benefit of libkvm. */
struct ccdbuf *ccdfreebufs;
static int numccdfreebufs;

/*
 * getccdbuf() -	Allocate and zero a ccd buffer.
 *
 *	This routine is called at splbio().
 */

static __inline
struct ccdbuf *
getccdbuf(struct ccdbuf *cpy)
{
	struct ccdbuf *cbp;

	/*
	 * Allocate from freelist or malloc as necessary
	 */
	if ((cbp = ccdfreebufs) != NULL) {
		ccdfreebufs = cbp->cb_freenext;
		--numccdfreebufs;
	} else {
		cbp = malloc(sizeof(struct ccdbuf), M_DEVBUF, M_WAITOK);
	}

	/*
	 * Used by mirroring code
	 */
	if (cpy)
		bcopy(cpy, cbp, sizeof(struct ccdbuf));
	else
		bzero(cbp, sizeof(struct ccdbuf));

	/*
	 * independant struct bio initialization
	 */

	return(cbp);
}

/*
 * putccdbuf() -	Free a ccd buffer.
 *
 *	This routine is called at splbio().
 */

static __inline
void
putccdbuf(struct ccdbuf *cbp)
{

	if (numccdfreebufs < NCCDFREEHIWAT) {
		cbp->cb_freenext = ccdfreebufs;
		ccdfreebufs = cbp;
		++numccdfreebufs;
	} else {
		free((caddr_t)cbp, M_DEVBUF);
	}
}


/*
 * Number of blocks to untouched in front of a component partition.
 * This is to avoid violating its disklabel area when it starts at the
 * beginning of the slice.
 */
#if !defined(CCD_OFFSET)
#define CCD_OFFSET 16
#endif

static struct ccd_s *
ccdfind(int unit)
{
	struct ccd_s *sc = NULL;

	/* XXX: LOCK(unique unit numbers) */
	LIST_FOREACH(sc, &ccd_softc_list, list) {
		if (sc->sc_unit == unit)
			break;
	}
	/* XXX: UNLOCK(unique unit numbers) */
	return ((sc == NULL) || (sc->sc_unit != unit) ? NULL : sc);
}

static struct ccd_s *
ccdnew(int unit)
{
	struct ccd_s *sc;

	/* XXX: LOCK(unique unit numbers) */
	if (IS_ALLOCATED(unit) || unit > DKMAXUNIT)
		return (NULL);

	MALLOC(sc, struct ccd_s *, sizeof(*sc), M_CCD, M_WAITOK | M_ZERO);
	sc->sc_unit = unit;
	LIST_INSERT_HEAD(&ccd_softc_list, sc, list);
	/* XXX: UNLOCK(unique unit numbers) */
	return (sc);
}

static int
ccddestroy(struct ccd_s *sc, struct proc *p)
{

	/* XXX: LOCK(unique unit numbers) */
	LIST_REMOVE(sc, list);
	/* XXX: UNLOCK(unique unit numbers) */
	FREE(sc, M_CCD);
	return (0);
}

static void
ccd_clone(void *arg, char *name, int namelen, dev_t *dev)
{
	int i, u;
	char *s;

	if (*dev != NODEV)
		return;
	i = dev_stdclone(name, &s, "ccd", &u);
	if (i != 2)
		return;
	if (*s < 'a' || *s > 'h')
		return;
	if (s[1] != '\0')
		return;
	*dev = make_dev(&ccd_cdevsw, u * 8 + *s - 'a',
		UID_ROOT, GID_OPERATOR, 0640, name);
}

/*
 * Called by main() during pseudo-device attachment.  All we need
 * to do is to add devsw entries.
 */
static void
ccdattach()
{

	EVENTHANDLER_REGISTER(dev_clone, ccd_clone, 0, 1000);
}

static int
ccd_modevent(module_t mod, int type, void *data)
{
	int error = 0;

	switch (type) {
	case MOD_LOAD:
		ccdattach();
		break;

	case MOD_UNLOAD:
		printf("ccd0: Unload not supported!\n");
		error = EOPNOTSUPP;
		break;

	case MOD_SHUTDOWN:
		break;

	default:
		error = EOPNOTSUPP;
	}
	return (error);
}

DEV_MODULE(ccd, ccd_modevent, NULL);

static int
ccdinit(struct ccd_s *cs, char **cpaths, struct thread *td)
{
	struct ccdcinfo *ci = NULL;	/* XXX */
	size_t size;
	int ix;
	struct vnode *vp;
	size_t minsize;
	int maxsecsize;
	struct partinfo dpart;
	struct ccdgeom *ccg = &cs->sc_geom;
	char *tmppath = NULL;
	int error = 0;

#ifdef DEBUG
	if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
		printf("ccdinit: unit %d\n", cs->sc_unit);
#endif

	cs->sc_size = 0;

	/* Allocate space for the component info. */
	cs->sc_cinfo = malloc(cs->sc_nccdisks * sizeof(struct ccdcinfo),
	    M_DEVBUF, M_WAITOK);

	/*
	 * Verify that each component piece exists and record
	 * relevant information about it.
	 */
	maxsecsize = 0;
	minsize = 0;
	tmppath = malloc(MAXPATHLEN, M_DEVBUF, M_WAITOK);
	for (ix = 0; ix < cs->sc_nccdisks; ix++) {
		vp = cs->sc_vpp[ix];
		ci = &cs->sc_cinfo[ix];
		ci->ci_vp = vp;

		/*
		 * Copy in the pathname of the component.
		 */
		if ((error = copyinstr(cpaths[ix], tmppath,
		    MAXPATHLEN, &ci->ci_pathlen)) != 0) {
#ifdef DEBUG
			if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
				printf("ccd%d: can't copy path, error = %d\n",
				    cs->sc_unit, error);
#endif
			goto fail;
		}
		ci->ci_path = malloc(ci->ci_pathlen, M_DEVBUF, M_WAITOK);
		bcopy(tmppath, ci->ci_path, ci->ci_pathlen);

		ci->ci_dev = vn_todev(vp);

		/*
		 * Get partition information for the component.
		 */
		if ((error = VOP_IOCTL(vp, DIOCGPART, (caddr_t)&dpart,
		    FREAD, td->td_ucred, td)) != 0) {
#ifdef DEBUG
			if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
				 printf("ccd%d: %s: ioctl failed, error = %d\n",
				     cs->sc_unit, ci->ci_path, error);
#endif
			goto fail;
		}
		if (dpart.part->p_fstype == FS_BSDFFS) {
			maxsecsize =
			    ((dpart.disklab->d_secsize > maxsecsize) ?
			    dpart.disklab->d_secsize : maxsecsize);
			size = dpart.part->p_size - CCD_OFFSET;
		} else {
#ifdef DEBUG
			if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
				printf("ccd%d: %s: incorrect partition type\n",
				    cs->sc_unit, ci->ci_path);
#endif
			error = EFTYPE;
			goto fail;
		}

		/*
		 * Calculate the size, truncating to an interleave
		 * boundary if necessary.
		 */

		if (cs->sc_ileave > 1)
			size -= size % cs->sc_ileave;

		if (size == 0) {
#ifdef DEBUG
			if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
				printf("ccd%d: %s: size == 0\n",
				    cs->sc_unit, ci->ci_path);
#endif
			error = ENODEV;
			goto fail;
		}

		if (minsize == 0 || size < minsize)
			minsize = size;
		ci->ci_size = size;
		cs->sc_size += size;
	}

	free(tmppath, M_DEVBUF);
	tmppath = NULL;

	/*
	 * Don't allow the interleave to be smaller than
	 * the biggest component sector.
	 */
	if ((cs->sc_ileave > 0) &&
	    (cs->sc_ileave < (maxsecsize / DEV_BSIZE))) {
#ifdef DEBUG
		if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
			printf("ccd%d: interleave must be at least %d\n",
			    cs->sc_unit, (maxsecsize / DEV_BSIZE));
#endif
		error = EINVAL;
		goto fail;
	}

	/*
	 * If uniform interleave is desired set all sizes to that of
	 * the smallest component.  This will guarentee that a single
	 * interleave table is generated.
	 *
	 * Lost space must be taken into account when calculating the
	 * overall size.  Half the space is lost when CCDF_MIRROR is
	 * specified.  One disk is lost when CCDF_PARITY is specified.
	 */
	if (cs->sc_flags & CCDF_UNIFORM) {
		for (ci = cs->sc_cinfo;
		     ci < &cs->sc_cinfo[cs->sc_nccdisks]; ci++) {
			ci->ci_size = minsize;
		}
		if (cs->sc_flags & CCDF_MIRROR) {
			/*
			 * Check to see if an even number of components
			 * have been specified.  The interleave must also
			 * be non-zero in order for us to be able to 
			 * guarentee the topology.
			 */
			if (cs->sc_nccdisks % 2) {
				printf("ccd%d: mirroring requires an even number of disks\n", cs->sc_unit );
				error = EINVAL;
				goto fail;
			}
			if (cs->sc_ileave == 0) {
				printf("ccd%d: an interleave must be specified when mirroring\n", cs->sc_unit);
				error = EINVAL;
				goto fail;
			}
			cs->sc_size = (cs->sc_nccdisks/2) * minsize;
		} else if (cs->sc_flags & CCDF_PARITY) {
			cs->sc_size = (cs->sc_nccdisks-1) * minsize;
		} else {
			if (cs->sc_ileave == 0) {
				printf("ccd%d: an interleave must be specified when using parity\n", cs->sc_unit);
				error = EINVAL;
				goto fail;
			}
			cs->sc_size = cs->sc_nccdisks * minsize;
		}
	}

	/*
	 * Construct the interleave table.
	 */
	ccdinterleave(cs, cs->sc_unit);

	/*
	 * Create pseudo-geometry based on 1MB cylinders.  It's
	 * pretty close.
	 */
	ccg->ccg_secsize = maxsecsize;
	ccg->ccg_ntracks = 1;
	ccg->ccg_nsectors = 1024 * 1024 / ccg->ccg_secsize;
	ccg->ccg_ncylinders = cs->sc_size / ccg->ccg_nsectors;

	/*
	 * Add an devstat entry for this device.
	 */
	devstat_add_entry(&cs->device_stats, "ccd", cs->sc_unit,
			  ccg->ccg_secsize, DEVSTAT_ALL_SUPPORTED,
			  DEVSTAT_TYPE_STORARRAY |DEVSTAT_TYPE_IF_OTHER,
			  DEVSTAT_PRIORITY_ARRAY);

	cs->sc_flags |= CCDF_INITED;
	cs->sc_cflags = cs->sc_flags;	/* So we can find out later... */
	return (0);
fail:
	while (ci > cs->sc_cinfo) {
		ci--;
		free(ci->ci_path, M_DEVBUF);
	}
	if (tmppath != NULL)
		free(tmppath, M_DEVBUF);
	free(cs->sc_cinfo, M_DEVBUF);
	return (error);
}

static void
ccdinterleave(struct ccd_s *cs, int unit)
{
	struct ccdcinfo *ci, *smallci;
	struct ccdiinfo *ii;
	daddr_t bn, lbn;
	int ix;
	u_long size;

#ifdef DEBUG
	if (ccddebug & CCDB_INIT)
		printf("ccdinterleave(%p): ileave %d\n", cs, cs->sc_ileave);
#endif

	/*
	 * Allocate an interleave table.  The worst case occurs when each
	 * of N disks is of a different size, resulting in N interleave
	 * tables.
	 *
	 * Chances are this is too big, but we don't care.
	 */
	size = (cs->sc_nccdisks + 1) * sizeof(struct ccdiinfo);
	cs->sc_itable = (struct ccdiinfo *)malloc(size, M_DEVBUF,
	    M_WAITOK | M_ZERO);

	/*
	 * Trivial case: no interleave (actually interleave of disk size).
	 * Each table entry represents a single component in its entirety.
	 *
	 * An interleave of 0 may not be used with a mirror or parity setup.
	 */
	if (cs->sc_ileave == 0) {
		bn = 0;
		ii = cs->sc_itable;

		for (ix = 0; ix < cs->sc_nccdisks; ix++) {
			/* Allocate space for ii_index. */
			ii->ii_index = malloc(sizeof(int), M_DEVBUF, M_WAITOK);
			ii->ii_ndisk = 1;
			ii->ii_startblk = bn;
			ii->ii_startoff = 0;
			ii->ii_index[0] = ix;
			bn += cs->sc_cinfo[ix].ci_size;
			ii++;
		}
		ii->ii_ndisk = 0;
#ifdef DEBUG
		if (ccddebug & CCDB_INIT)
			printiinfo(cs->sc_itable);
#endif
		return;
	}

	/*
	 * The following isn't fast or pretty; it doesn't have to be.
	 */
	size = 0;
	bn = lbn = 0;
	for (ii = cs->sc_itable; ; ii++) {
		/*
		 * Allocate space for ii_index.  We might allocate more then
		 * we use.
		 */
		ii->ii_index = malloc((sizeof(int) * cs->sc_nccdisks),
		    M_DEVBUF, M_WAITOK);

		/*
		 * Locate the smallest of the remaining components
		 */
		smallci = NULL;
		for (ci = cs->sc_cinfo; ci < &cs->sc_cinfo[cs->sc_nccdisks]; 
		    ci++) {
			if (ci->ci_size > size &&
			    (smallci == NULL ||
			     ci->ci_size < smallci->ci_size)) {
				smallci = ci;
			}
		}

		/*
		 * Nobody left, all done
		 */
		if (smallci == NULL) {
			ii->ii_ndisk = 0;
			break;
		}

		/*
		 * Record starting logical block using an sc_ileave blocksize.
		 */
		ii->ii_startblk = bn / cs->sc_ileave;

		/*
		 * Record starting comopnent block using an sc_ileave 
		 * blocksize.  This value is relative to the beginning of
		 * a component disk.
		 */
		ii->ii_startoff = lbn;

		/*
		 * Determine how many disks take part in this interleave
		 * and record their indices.
		 */
		ix = 0;
		for (ci = cs->sc_cinfo; 
		    ci < &cs->sc_cinfo[cs->sc_nccdisks]; ci++) {
			if (ci->ci_size >= smallci->ci_size) {
				ii->ii_index[ix++] = ci - cs->sc_cinfo;
			}
		}
		ii->ii_ndisk = ix;
		bn += ix * (smallci->ci_size - size);
		lbn = smallci->ci_size / cs->sc_ileave;
		size = smallci->ci_size;
	}
#ifdef DEBUG
	if (ccddebug & CCDB_INIT)
		printiinfo(cs->sc_itable);
#endif
}

/* ARGSUSED */
static int
ccdopen(dev_t dev, int flags, int fmt, struct thread *td)
{
	int unit = ccdunit(dev);
	struct ccd_s *cs;
	struct disklabel *lp;
	int error = 0, part, pmask;

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdopen(%p, %x)\n", dev, flags);
#endif

	cs = IS_ALLOCATED(unit) ? ccdfind(unit) : ccdnew(unit);

	if ((error = ccdlock(cs)) != 0)
		return (error);

	lp = &cs->sc_label;

	part = ccdpart(dev);
	pmask = (1 << part);

	/*
	 * If we're initialized, check to see if there are any other
	 * open partitions.  If not, then it's safe to update
	 * the in-core disklabel.
	 */
	if (IS_INITED(cs) && (cs->sc_openmask == 0))
		ccdgetdisklabel(dev);

	/* Check that the partition exists. */
	if (part != RAW_PART && ((part >= lp->d_npartitions) ||
	    (lp->d_partitions[part].p_fstype == FS_UNUSED))) {
		error = ENXIO;
		goto done;
	}

	cs->sc_openmask |= pmask;
 done:
	ccdunlock(cs);
	return (0);
}

/* ARGSUSED */
static int
ccdclose(dev_t dev, int flags, int fmt, struct thread *td)
{
	int unit = ccdunit(dev);
	struct ccd_s *cs;
	int error = 0, part;

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdclose(%p, %x)\n", dev, flags);
#endif

	if (!IS_ALLOCATED(unit))
		return (ENXIO);
	cs = ccdfind(unit);

	if ((error = ccdlock(cs)) != 0)
		return (error);

	part = ccdpart(dev);

	/* ...that much closer to allowing unconfiguration... */
	cs->sc_openmask &= ~(1 << part);
	/* collect "garbage" if possible */
	if (!IS_INITED(cs) && (cs->sc_flags & CCDF_WANTED) == 0)
		ccddestroy(cs, td->td_proc);
	else
		ccdunlock(cs);
	return (0);
}

static void
ccdstrategy(struct bio *bp)
{
	int unit = ccdunit(bp->bio_dev);
	struct ccd_s *cs = ccdfind(unit);
	int s;
	int wlabel;
	struct disklabel *lp;

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdstrategy(%p): unit %d\n", bp, unit);
#endif
	if (!IS_INITED(cs)) {
		biofinish(bp, NULL, ENXIO);
		return;
	}

	/* If it's a nil transfer, wake up the top half now. */
	if (bp->bio_bcount == 0) {
		biodone(bp);
		return;
	}

	lp = &cs->sc_label;

	/*
	 * Do bounds checking and adjust transfer.  If there's an
	 * error, the bounds check will flag that for us.
	 */
	wlabel = cs->sc_flags & (CCDF_WLABEL|CCDF_LABELLING);
	if (ccdpart(bp->bio_dev) != RAW_PART) {
		if (bounds_check_with_label(bp, lp, wlabel) <= 0) {
			biodone(bp);
			return;
		}
	} else {
		int pbn;        /* in sc_secsize chunks */
		long sz;        /* in sc_secsize chunks */

		pbn = bp->bio_blkno / (cs->sc_geom.ccg_secsize / DEV_BSIZE);
		sz = howmany(bp->bio_bcount, cs->sc_geom.ccg_secsize);

		/*
		 * If out of bounds return an error. If at the EOF point,
		 * simply read or write less.
		 */

		if (pbn < 0 || pbn >= cs->sc_size) {
			bp->bio_resid = bp->bio_bcount;
			if (pbn != cs->sc_size)
				biofinish(bp, NULL, EINVAL);
			else
				biodone(bp);
			return;
		}

		/*
		 * If the request crosses EOF, truncate the request.
		 */
		if (pbn + sz > cs->sc_size) {
			bp->bio_bcount = (cs->sc_size - pbn) * 
			    cs->sc_geom.ccg_secsize;
		}
	}

	bp->bio_resid = bp->bio_bcount;

	/*
	 * "Start" the unit.
	 */
	s = splbio();
	ccdstart(cs, bp);
	splx(s);
	return;
}

static void
ccdstart(struct ccd_s *cs, struct bio *bp)
{
	long bcount, rcount;
	struct ccdbuf *cbp[4];
	/* XXX! : 2 reads and 2 writes for RAID 4/5 */
	caddr_t addr;
	daddr_t bn;
	struct partition *pp;

#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdstart(%p, %p)\n", cs, bp);
#endif

	/* Record the transaction start  */
	devstat_start_transaction(&cs->device_stats);

	/*
	 * Translate the partition-relative block number to an absolute.
	 */
	bn = bp->bio_blkno;
	if (ccdpart(bp->bio_dev) != RAW_PART) {
		pp = &cs->sc_label.d_partitions[ccdpart(bp->bio_dev)];
		bn += pp->p_offset;
	}

	/*
	 * Allocate component buffers and fire off the requests
	 */
	addr = bp->bio_data;
	for (bcount = bp->bio_bcount; bcount > 0; bcount -= rcount) {
		ccdbuffer(cbp, cs, bp, bn, addr, bcount);
		rcount = cbp[0]->cb_buf.bio_bcount;

		if (cs->sc_cflags & CCDF_MIRROR) {
			/*
			 * Mirroring.  Writes go to both disks, reads are
			 * taken from whichever disk seems most appropriate.
			 *
			 * We attempt to localize reads to the disk whos arm
			 * is nearest the read request.  We ignore seeks due
			 * to writes when making this determination and we
			 * also try to avoid hogging.
			 */
			if (cbp[0]->cb_buf.bio_cmd == BIO_WRITE) {
				BIO_STRATEGY(&cbp[0]->cb_buf, 0);
				BIO_STRATEGY(&cbp[1]->cb_buf, 0);
			} else {
				int pick = cs->sc_pick;
				daddr_t range = cs->sc_size / 16;

				if (bn < cs->sc_blk[pick] - range ||
				    bn > cs->sc_blk[pick] + range
				) {
					cs->sc_pick = pick = 1 - pick;
				}
				cs->sc_blk[pick] = bn + btodb(rcount);
				BIO_STRATEGY(&cbp[pick]->cb_buf, 0);
			}
		} else {
			/*
			 * Not mirroring
			 */
			BIO_STRATEGY(&cbp[0]->cb_buf, 0);
		}
		bn += btodb(rcount);
		addr += rcount;
	}
}

/*
 * Build a component buffer header.
 */
static void
ccdbuffer(struct ccdbuf **cb, struct ccd_s *cs, struct bio *bp, daddr_t bn, caddr_t addr, long bcount)
{
	struct ccdcinfo *ci, *ci2 = NULL;	/* XXX */
	struct ccdbuf *cbp;
	daddr_t cbn, cboff;
	off_t cbc;

#ifdef DEBUG
	if (ccddebug & CCDB_IO)
		printf("ccdbuffer(%p, %p, %d, %p, %ld)\n",
		       cs, bp, bn, addr, bcount);
#endif
	/*
	 * Determine which component bn falls in.
	 */
	cbn = bn;
	cboff = 0;

	if (cs->sc_ileave == 0) {
		/*
		 * Serially concatenated and neither a mirror nor a parity
		 * config.  This is a special case.
		 */
		daddr_t sblk;

		sblk = 0;
		for (ci = cs->sc_cinfo; cbn >= sblk + ci->ci_size; ci++)
			sblk += ci->ci_size;
		cbn -= sblk;
	} else {
		struct ccdiinfo *ii;
		int ccdisk, off;

		/*
		 * Calculate cbn, the logical superblock (sc_ileave chunks),
		 * and cboff, a normal block offset (DEV_BSIZE chunks) relative
		 * to cbn.
		 */
		cboff = cbn % cs->sc_ileave;	/* DEV_BSIZE gran */
		cbn = cbn / cs->sc_ileave;	/* DEV_BSIZE * ileave gran */

		/*
		 * Figure out which interleave table to use.
		 */
		for (ii = cs->sc_itable; ii->ii_ndisk; ii++) {
			if (ii->ii_startblk > cbn)
				break;
		}
		ii--;

		/*
		 * off is the logical superblock relative to the beginning 
		 * of this interleave block.  
		 */
		off = cbn - ii->ii_startblk;

		/*
		 * We must calculate which disk component to use (ccdisk),
		 * and recalculate cbn to be the superblock relative to
		 * the beginning of the component.  This is typically done by
		 * adding 'off' and ii->ii_startoff together.  However, 'off'
		 * must typically be divided by the number of components in
		 * this interleave array to be properly convert it from a
		 * CCD-relative logical superblock number to a 
		 * component-relative superblock number.
		 */
		if (ii->ii_ndisk == 1) {
			/*
			 * When we have just one disk, it can't be a mirror
			 * or a parity config.
			 */
			ccdisk = ii->ii_index[0];
			cbn = ii->ii_startoff + off;
		} else {
			if (cs->sc_cflags & CCDF_MIRROR) {
				/*
				 * We have forced a uniform mapping, resulting
				 * in a single interleave array.  We double
				 * up on the first half of the available
				 * components and our mirror is in the second
				 * half.  This only works with a single 
				 * interleave array because doubling up
				 * doubles the number of sectors, so there
				 * cannot be another interleave array because
				 * the next interleave array's calculations
				 * would be off.
				 */
				int ndisk2 = ii->ii_ndisk / 2;
				ccdisk = ii->ii_index[off % ndisk2];
				cbn = ii->ii_startoff + off / ndisk2;
				ci2 = &cs->sc_cinfo[ccdisk + ndisk2];
			} else if (cs->sc_cflags & CCDF_PARITY) {
				/* 
				 * XXX not implemented yet
				 */
				int ndisk2 = ii->ii_ndisk - 1;
				ccdisk = ii->ii_index[off % ndisk2];
				cbn = ii->ii_startoff + off / ndisk2;
				if (cbn % ii->ii_ndisk <= ccdisk)
					ccdisk++;
			} else {
				ccdisk = ii->ii_index[off % ii->ii_ndisk];
				cbn = ii->ii_startoff + off / ii->ii_ndisk;
			}
		}

		ci = &cs->sc_cinfo[ccdisk];

		/*
		 * Convert cbn from a superblock to a normal block so it
		 * can be used to calculate (along with cboff) the normal
		 * block index into this particular disk.
		 */
		cbn *= cs->sc_ileave;
	}

	/*
	 * Fill in the component buf structure.
	 */
	cbp = getccdbuf(NULL);
	cbp->cb_buf.bio_cmd = bp->bio_cmd;
	cbp->cb_buf.bio_done = ccdiodone;
	cbp->cb_buf.bio_dev = ci->ci_dev;		/* XXX */
	cbp->cb_buf.bio_blkno = cbn + cboff + CCD_OFFSET;
	cbp->cb_buf.bio_offset = dbtob(cbn + cboff + CCD_OFFSET);
	cbp->cb_buf.bio_data = addr;
	if (cs->sc_ileave == 0)
              cbc = dbtob((off_t)(ci->ci_size - cbn));
	else
              cbc = dbtob((off_t)(cs->sc_ileave - cboff));
	cbp->cb_buf.bio_bcount = (cbc < bcount) ? cbc : bcount;
 	cbp->cb_buf.bio_caller1 = (void*)cbp->cb_buf.bio_bcount;

	/*
	 * context for ccdiodone
	 */
	cbp->cb_obp = bp;
	cbp->cb_unit = cs->sc_unit;
	cbp->cb_comp = ci - cs->sc_cinfo;

#ifdef DEBUG
	if (ccddebug & CCDB_IO)
		printf(" dev %p(u%ld): cbp %p bn %d addr %p bcnt %ld\n",
		       ci->ci_dev, (unsigned long)(ci-cs->sc_cinfo), cbp, 
		       cbp->cb_buf.bio_blkno, cbp->cb_buf.bio_data, 
		       cbp->cb_buf.bio_bcount);
#endif
	cb[0] = cbp;

	/*
	 * Note: both I/O's setup when reading from mirror, but only one
	 * will be executed.
	 */
	if (cs->sc_cflags & CCDF_MIRROR) {
		/* mirror, setup second I/O */
		cbp = getccdbuf(cb[0]);
		cbp->cb_buf.bio_dev = ci2->ci_dev;
		cbp->cb_comp = ci2 - cs->sc_cinfo;
		cb[1] = cbp;
		/* link together the ccdbuf's and clear "mirror done" flag */
		cb[0]->cb_mirror = cb[1];
		cb[1]->cb_mirror = cb[0];
		cb[0]->cb_pflags &= ~CCDPF_MIRROR_DONE;
		cb[1]->cb_pflags &= ~CCDPF_MIRROR_DONE;
	}
}

static void
ccdintr(struct ccd_s *cs, struct bio *bp)
{
#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdintr(%p, %p)\n", cs, bp);
#endif
	/*
	 * Request is done for better or worse, wakeup the top half.
	 */
	if (bp->bio_flags & BIO_ERROR)
		bp->bio_resid = bp->bio_bcount;
	biofinish(bp, &cs->device_stats, 0);
}

/*
 * Called at interrupt time.
 * Mark the component as done and if all components are done,
 * take a ccd interrupt.
 */
static void
ccdiodone(struct bio *ibp)
{
	struct ccdbuf *cbp = (struct ccdbuf *)ibp;
	struct bio *bp = cbp->cb_obp;
	int unit = cbp->cb_unit;
	int count, s;

	s = splbio();
#ifdef DEBUG
	if (ccddebug & CCDB_FOLLOW)
		printf("ccdiodone(%p)\n", cbp);
	if (ccddebug & CCDB_IO) {
		printf("ccdiodone: bp %p bcount %ld resid %ld\n",
		       bp, bp->bio_bcount, bp->bio_resid);
		printf(" dev %p(u%d), cbp %p bn %d addr %p bcnt %ld\n",
		       cbp->cb_buf.bio_dev, cbp->cb_comp, cbp,
		       cbp->cb_buf.bio_blkno, cbp->cb_buf.bio_data,
		       cbp->cb_buf.bio_bcount);
	}
#endif
	/*
	 * If an error occured, report it.  If this is a mirrored 
	 * configuration and the first of two possible reads, do not
	 * set the error in the bp yet because the second read may
	 * succeed.
	 */

	if (cbp->cb_buf.bio_flags & BIO_ERROR) {
		const char *msg = "";

		if ((ccdfind(unit)->sc_cflags & CCDF_MIRROR) &&
		    (cbp->cb_buf.bio_cmd == BIO_READ) &&
		    (cbp->cb_pflags & CCDPF_MIRROR_DONE) == 0) {
			/*
			 * We will try our read on the other disk down
			 * below, also reverse the default pick so if we 
			 * are doing a scan we do not keep hitting the
			 * bad disk first.
			 */
			struct ccd_s *cs = ccdfind(unit);

			msg = ", trying other disk";
			cs->sc_pick = 1 - cs->sc_pick;
			cs->sc_blk[cs->sc_pick] = bp->bio_blkno;
		} else {
			bp->bio_flags |= BIO_ERROR;
			bp->bio_error = cbp->cb_buf.bio_error ? 
			    cbp->cb_buf.bio_error : EIO;
		}
		printf("ccd%d: error %d on component %d block %d (ccd block %d)%s\n",
		       unit, bp->bio_error, cbp->cb_comp, 
		       (int)cbp->cb_buf.bio_blkno, bp->bio_blkno, msg);
	}

	/*
	 * Process mirror.  If we are writing, I/O has been initiated on both
	 * buffers and we fall through only after both are finished.
	 *
	 * If we are reading only one I/O is initiated at a time.  If an
	 * error occurs we initiate the second I/O and return, otherwise 
	 * we free the second I/O without initiating it.
	 */

	if (ccdfind(unit)->sc_cflags & CCDF_MIRROR) {
		if (cbp->cb_buf.bio_cmd == BIO_WRITE) {
			/*
			 * When writing, handshake with the second buffer
			 * to determine when both are done.  If both are not
			 * done, return here.
			 */
			if ((cbp->cb_pflags & CCDPF_MIRROR_DONE) == 0) {
				cbp->cb_mirror->cb_pflags |= CCDPF_MIRROR_DONE;
				putccdbuf(cbp);
				splx(s);
				return;
			}
		} else {
			/*
			 * When reading, either dispose of the second buffer
			 * or initiate I/O on the second buffer if an error 
			 * occured with this one.
			 */
			if ((cbp->cb_pflags & CCDPF_MIRROR_DONE) == 0) {
				if (cbp->cb_buf.bio_flags & BIO_ERROR) {
					cbp->cb_mirror->cb_pflags |= 
					    CCDPF_MIRROR_DONE;
					BIO_STRATEGY(&cbp->cb_mirror->cb_buf, 0);
					putccdbuf(cbp);
					splx(s);
					return;
				} else {
					putccdbuf(cbp->cb_mirror);
					/* fall through */
				}
			}
		}
	}

	/*
	 * use bio_caller1 to determine how big the original request was rather
	 * then bio_bcount, because bio_bcount may have been truncated for EOF.
	 *
	 * XXX We check for an error, but we do not test the resid for an
	 * aligned EOF condition.  This may result in character & block
	 * device access not recognizing EOF properly when read or written 
	 * sequentially, but will not effect filesystems.
	 */
	count = (long)cbp->cb_buf.bio_caller1;
	putccdbuf(cbp);

	/*
	 * If all done, "interrupt".
	 */
	bp->bio_resid -= count;
	if (bp->bio_resid < 0)
		panic("ccdiodone: count");
	if (bp->bio_resid == 0)
		ccdintr(ccdfind(unit), bp);
	splx(s);
}

static int
ccdioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct thread *td)
{
	int unit = ccdunit(dev);
	int i, j, lookedup = 0, error = 0;
	int part, pmask, s;
	struct ccd_s *cs;
	struct ccd_ioctl *ccio = (struct ccd_ioctl *)data;
	char **cpp;
	struct vnode **vpp;

	if (!IS_ALLOCATED(unit))
		return (ENXIO);
	cs = ccdfind(unit);

	switch (cmd) {
	case CCDIOCSET:
		if (IS_INITED(cs))
			return (EBUSY);

		if ((flag & FWRITE) == 0)
			return (EBADF);

		if ((error = ccdlock(cs)) != 0)
			return (error);

		if (ccio->ccio_ndisks > CCD_MAXNDISKS)
			return (EINVAL);
 
		/* Fill in some important bits. */
		cs->sc_ileave = ccio->ccio_ileave;
		if (cs->sc_ileave == 0 &&
		    ((ccio->ccio_flags & CCDF_MIRROR) ||
		     (ccio->ccio_flags & CCDF_PARITY))) {
			printf("ccd%d: disabling mirror/parity, interleave is 0\n", unit);
			ccio->ccio_flags &= ~(CCDF_MIRROR | CCDF_PARITY);
		}
		if ((ccio->ccio_flags & CCDF_MIRROR) &&
		    (ccio->ccio_flags & CCDF_PARITY)) {
			printf("ccd%d: can't specify both mirror and parity, using mirror\n", unit);
			ccio->ccio_flags &= ~CCDF_PARITY;
		}
		if ((ccio->ccio_flags & (CCDF_MIRROR | CCDF_PARITY)) &&
		    !(ccio->ccio_flags & CCDF_UNIFORM)) {
			printf("ccd%d: mirror/parity forces uniform flag\n",
			       unit);
			ccio->ccio_flags |= CCDF_UNIFORM;
		}
		cs->sc_flags = ccio->ccio_flags & CCDF_USERMASK;

		/*
		 * Allocate space for and copy in the array of
		 * componet pathnames and device numbers.
		 */
		cpp = malloc(ccio->ccio_ndisks * sizeof(char *),
		    M_DEVBUF, M_WAITOK);
		vpp = malloc(ccio->ccio_ndisks * sizeof(struct vnode *),
		    M_DEVBUF, M_WAITOK);

		error = copyin((caddr_t)ccio->ccio_disks, (caddr_t)cpp,
		    ccio->ccio_ndisks * sizeof(char **));
		if (error) {
			free(vpp, M_DEVBUF);
			free(cpp, M_DEVBUF);
			ccdunlock(cs);
			return (error);
		}

#ifdef DEBUG
		if (ccddebug & CCDB_INIT)
			for (i = 0; i < ccio->ccio_ndisks; ++i)
				printf("ccdioctl: component %d: %p\n",
				    i, cpp[i]);
#endif

		for (i = 0; i < ccio->ccio_ndisks; ++i) {
#ifdef DEBUG
			if (ccddebug & CCDB_INIT)
				printf("ccdioctl: lookedup = %d\n", lookedup);
#endif
			if ((error = ccdlookup(cpp[i], td, &vpp[i])) != 0) {
				for (j = 0; j < lookedup; ++j)
					(void)vn_close(vpp[j], FREAD|FWRITE,
					    td->td_ucred, td);
				free(vpp, M_DEVBUF);
				free(cpp, M_DEVBUF);
				ccdunlock(cs);
				return (error);
			}
			++lookedup;
		}
		cs->sc_vpp = vpp;
		cs->sc_nccdisks = ccio->ccio_ndisks;

		/*
		 * Initialize the ccd.  Fills in the softc for us.
		 */
		if ((error = ccdinit(cs, cpp, td)) != 0) {
			for (j = 0; j < lookedup; ++j)
				(void)vn_close(vpp[j], FREAD|FWRITE,
				    td->td_ucred, td);
			/*
			 * We can't ccddestroy() cs just yet, because nothing
			 * prevents user-level app to do another ioctl()
			 * without closing the device first, therefore
			 * declare unit null and void and let ccdclose()
			 * destroy it when it is safe to do so.
			 */
			cs->sc_flags &= (CCDF_WANTED | CCDF_LOCKED);
			free(vpp, M_DEVBUF);
			free(cpp, M_DEVBUF);
			ccdunlock(cs);
			return (error);
		}

		/*
		 * The ccd has been successfully initialized, so
		 * we can place it into the array and read the disklabel.
		 */
		ccio->ccio_unit = unit;
		ccio->ccio_size = cs->sc_size;
		ccdgetdisklabel(dev);

		ccdunlock(cs);

		break;

	case CCDIOCCLR:
		if (!IS_INITED(cs))
			return (ENXIO);

		if ((flag & FWRITE) == 0)
			return (EBADF);

		if ((error = ccdlock(cs)) != 0)
			return (error);

		/* Don't unconfigure if any other partitions are open */
		part = ccdpart(dev);
		pmask = (1 << part);
		if ((cs->sc_openmask & ~pmask)) {
			ccdunlock(cs);
			return (EBUSY);
		}

		/* Declare unit null and void (reset all flags) */
		cs->sc_flags &= (CCDF_WANTED | CCDF_LOCKED);

		/* Close the components and free their pathnames. */
		for (i = 0; i < cs->sc_nccdisks; ++i) {
			/*
			 * XXX: this close could potentially fail and
			 * cause Bad Things.  Maybe we need to force
			 * the close to happen?
			 */
#ifdef DEBUG
			if (ccddebug & CCDB_VNODE)
				vprint("CCDIOCCLR: vnode info",
				    cs->sc_cinfo[i].ci_vp);
#endif
			(void)vn_close(cs->sc_cinfo[i].ci_vp, FREAD|FWRITE,
			    td->td_ucred, td);
			free(cs->sc_cinfo[i].ci_path, M_DEVBUF);
		}

		/* Free interleave index. */
		for (i = 0; cs->sc_itable[i].ii_ndisk; ++i)
			free(cs->sc_itable[i].ii_index, M_DEVBUF);

		/* Free component info and interleave table. */
		free(cs->sc_cinfo, M_DEVBUF);
		free(cs->sc_itable, M_DEVBUF);
		free(cs->sc_vpp, M_DEVBUF);

		/* And remove the devstat entry. */
		devstat_remove_entry(&cs->device_stats);

		/* This must be atomic. */
		s = splhigh();
		ccdunlock(cs);
		splx(s);

		break;

	case CCDCONFINFO:
		{
			int ninit = 0;
			struct ccdconf *conf = (struct ccdconf *)data;
			struct ccd_s *tmpcs;
			struct ccd_s *ubuf = conf->buffer;

			/* XXX: LOCK(unique unit numbers) */
			LIST_FOREACH(tmpcs, &ccd_softc_list, list)
				if (IS_INITED(tmpcs))
					ninit++;

			if (conf->size == 0) {
				conf->size = sizeof(struct ccd_s) * ninit;
				break;
			} else if ((conf->size / sizeof(struct ccd_s) != ninit) ||
			    (conf->size % sizeof(struct ccd_s) != 0)) {
				/* XXX: UNLOCK(unique unit numbers) */
				return (EINVAL);
			}

			ubuf += ninit;
			LIST_FOREACH(tmpcs, &ccd_softc_list, list) {
				if (!IS_INITED(tmpcs))
					continue;
				error = copyout(tmpcs, --ubuf,
				    sizeof(struct ccd_s));
				if (error != 0)
					/* XXX: UNLOCK(unique unit numbers) */
					return (error);
			}
			/* XXX: UNLOCK(unique unit numbers) */
		}
		break;

	case CCDCPPINFO:
		if (!IS_INITED(cs))
			return (ENXIO);

		{
			int len = 0;
			struct ccdcpps *cpps = (struct ccdcpps *)data;
			char *ubuf = cpps->buffer;


			for (i = 0; i < cs->sc_nccdisks; ++i)
				len += cs->sc_cinfo[i].ci_pathlen;

			if (cpps->size == 0) {
				cpps->size = len;
				break;
			} else if (cpps->size != len) {
				return (EINVAL);
			}

			for (i = 0; i < cs->sc_nccdisks; ++i) {
				len = cs->sc_cinfo[i].ci_pathlen;
				error = copyout(cs->sc_cinfo[i].ci_path, ubuf,
				    len);
				if (error != 0)
					return (error);
				ubuf += len;
			}
		}
		break;

	case DIOCGDINFO:
		if (!IS_INITED(cs))
			return (ENXIO);

		*(struct disklabel *)data = cs->sc_label;
		break;

	case DIOCGPART:
		if (!IS_INITED(cs))
			return (ENXIO);

		((struct partinfo *)data)->disklab = &cs->sc_label;
		((struct partinfo *)data)->part =
		    &cs->sc_label.d_partitions[ccdpart(dev)];
		break;

	case DIOCWDINFO:
	case DIOCSDINFO:
		if (!IS_INITED(cs))
			return (ENXIO);

		if ((flag & FWRITE) == 0)
			return (EBADF);

		if ((error = ccdlock(cs)) != 0)
			return (error);

		cs->sc_flags |= CCDF_LABELLING;

		error = setdisklabel(&cs->sc_label,
		    (struct disklabel *)data, 0);
		if (error == 0) {
			if (cmd == DIOCWDINFO)
				error = writedisklabel(CCDLABELDEV(dev),
				    &cs->sc_label);
		}

		cs->sc_flags &= ~CCDF_LABELLING;

		ccdunlock(cs);

		if (error)
			return (error);
		break;

	case DIOCWLABEL:
		if (!IS_INITED(cs))
			return (ENXIO);

		if ((flag & FWRITE) == 0)
			return (EBADF);
		if (*(int *)data != 0)
			cs->sc_flags |= CCDF_WLABEL;
		else
			cs->sc_flags &= ~CCDF_WLABEL;
		break;

	default:
		return (ENOTTY);
	}

	return (0);
}

static int
ccdsize(dev_t dev)
{
	struct ccd_s *cs;
	int part, size;

	if (ccdopen(dev, 0, S_IFCHR, curthread))
		return (-1);

	cs = ccdfind(ccdunit(dev));
	part = ccdpart(dev);

	if (!IS_INITED(cs))
		return (-1);

	if (cs->sc_label.d_partitions[part].p_fstype != FS_SWAP)
		size = -1;
	else
		size = cs->sc_label.d_partitions[part].p_size;

	if (ccdclose(dev, 0, S_IFCHR, curthread))
		return (-1);

	return (size);
}

static int
ccddump(dev_t dev)
{

	/* Not implemented. */
	return ENXIO;
}

/*
 * Lookup the provided name in the filesystem.  If the file exists,
 * is a valid block device, and isn't being used by anyone else,
 * set *vpp to the file's vnode.
 */
static int
ccdlookup(char *path, struct thread *td, struct vnode **vpp)
{
	struct nameidata nd;
	struct vnode *vp;
	int error, flags;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, path, td);
	flags = FREAD | FWRITE;
	if ((error = vn_open(&nd, &flags, 0)) != 0) {
#ifdef DEBUG
		if (ccddebug & (CCDB_FOLLOW|CCDB_INIT))
			printf("ccdlookup: vn_open error = %d\n", error);
#endif
		return (error);
	}
	vp = nd.ni_vp;

	if (vp->v_usecount > 1) {
		error = EBUSY;
		goto bad;
	}

	if (!vn_isdisk(vp, &error)) 
		goto bad;

#ifdef DEBUG
	if (ccddebug & CCDB_VNODE)
		vprint("ccdlookup: vnode info", vp);
#endif

	VOP_UNLOCK(vp, 0, td);
	NDFREE(&nd, NDF_ONLY_PNBUF);
	*vpp = vp;
	return (0);
bad:
	VOP_UNLOCK(vp, 0, td);
	NDFREE(&nd, NDF_ONLY_PNBUF);
	/* vn_close does vrele() for vp */
	(void)vn_close(vp, FREAD|FWRITE, td->td_ucred, td);
	return (error);
}

/*
 * Read the disklabel from the ccd.  If one is not present, fake one
 * up.
 */
static void
ccdgetdisklabel(dev_t dev)
{
	int unit = ccdunit(dev);
	struct ccd_s *cs = ccdfind(unit);
	char *errstring;
	struct disklabel *lp = &cs->sc_label;
	struct ccdgeom *ccg = &cs->sc_geom;

	bzero(lp, sizeof(*lp));

	lp->d_secperunit = cs->sc_size;
	lp->d_secsize = ccg->ccg_secsize;
	lp->d_nsectors = ccg->ccg_nsectors;
	lp->d_ntracks = ccg->ccg_ntracks;
	lp->d_ncylinders = ccg->ccg_ncylinders;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;

	strncpy(lp->d_typename, "ccd", sizeof(lp->d_typename));
	lp->d_type = DTYPE_CCD;
	strncpy(lp->d_packname, "fictitious", sizeof(lp->d_packname));
	lp->d_rpm = 3600;
	lp->d_interleave = 1;
	lp->d_flags = 0;

	lp->d_partitions[RAW_PART].p_offset = 0;
	lp->d_partitions[RAW_PART].p_size = cs->sc_size;
	lp->d_partitions[RAW_PART].p_fstype = FS_UNUSED;
	lp->d_npartitions = RAW_PART + 1;

	lp->d_bbsize = BBSIZE;				/* XXX */
	lp->d_sbsize = SBSIZE;				/* XXX */

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(&cs->sc_label);

	/*
	 * Call the generic disklabel extraction routine.
	 */
	errstring = readdisklabel(CCDLABELDEV(dev), &cs->sc_label);
	if (errstring != NULL)
		ccdmakedisklabel(cs);

#ifdef DEBUG
	/* It's actually extremely common to have unlabeled ccds. */
	if (ccddebug & CCDB_LABEL)
		if (errstring != NULL)
			printf("ccd%d: %s\n", unit, errstring);
#endif
}

/*
 * Take care of things one might want to take care of in the event
 * that a disklabel isn't present.
 */
static void
ccdmakedisklabel(struct ccd_s *cs)
{
	struct disklabel *lp = &cs->sc_label;

	/*
	 * For historical reasons, if there's no disklabel present
	 * the raw partition must be marked FS_BSDFFS.
	 */
	lp->d_partitions[RAW_PART].p_fstype = FS_BSDFFS;

	strncpy(lp->d_packname, "default label", sizeof(lp->d_packname));
}

/*
 * Wait interruptibly for an exclusive lock.
 *
 * XXX
 * Several drivers do this; it should be abstracted and made MP-safe.
 */
static int
ccdlock(struct ccd_s *cs)
{
	int error;

	while ((cs->sc_flags & CCDF_LOCKED) != 0) {
		cs->sc_flags |= CCDF_WANTED;
		if ((error = tsleep(cs, PRIBIO | PCATCH, "ccdlck", 0)) != 0)
			return (error);
	}
	cs->sc_flags |= CCDF_LOCKED;
	return (0);
}

/*
 * Unlock and wake up any waiters.
 */
static void
ccdunlock(struct ccd_s *cs)
{

	cs->sc_flags &= ~CCDF_LOCKED;
	if ((cs->sc_flags & CCDF_WANTED) != 0) {
		cs->sc_flags &= ~CCDF_WANTED;
		wakeup(cs);
	}
}

#ifdef DEBUG
static void
printiinfo(struct ccdiinfo *ii)
{
	int ix, i;

	for (ix = 0; ii->ii_ndisk; ix++, ii++) {
		printf(" itab[%d]: #dk %d sblk %d soff %d",
		       ix, ii->ii_ndisk, ii->ii_startblk, ii->ii_startoff);
		for (i = 0; i < ii->ii_ndisk; i++)
			printf(" %d", ii->ii_index[i]);
		printf("\n");
	}
}
#endif
