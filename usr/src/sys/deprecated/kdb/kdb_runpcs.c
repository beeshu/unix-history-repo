/*	kdb_runpcs.c	7.1	86/11/20	*/

#include "../kdb/defs.h"

char	*lp;

/* breakpoints */
BKPTR	bkpthead;

char	lastc;

long	dot;
int	adrflg;
long	loopcnt;
ADDR	userpc = 1;

runpcs(runmode, execsig)
{
	register BKPTR bkpt;

	if (adrflg)
		userpc = dot;
	if (execsig == 0)
		printf("kdb: running\n");
	if (runmode==SINGLE) {
		/*
		 * To single step, delete the
		 * breakpoints and invoke the
		 * hardware single step in the
		 * main loop.
		 */
		delbp();
		reset(SINGLE);
	}
	/*
	 * If we're currently at a breakpoint,
	 * restore the instruction and single
	 * step before continuing.  Otherwise,
	 * we can just set our breakpoints and
	 * continue.
	 */
	if (bkpt = scanbkpt(userpc)) {
		execbkpt(bkpt, execsig);
		/*NOTREACHED*/
	}
	setbp();
	reset(CONTIN);
}

static	int execbkptf;

/*
 * Continue execution after a trap.
 *
 * If tracetrap is nonzero, we've entered here because of a
 * trace trap.  If we're skipping a breakpoint (execbkptf),
 * or this is the next iteration of a breakpoint, continue.
 * If this is the next iteration of a single step, do the
 * next step.  Otherwise return 1 if we stopped because
 * of a breakpoint,
 */
nextpcs(tracetrap, execsig)
	int tracetrap, execsig;
{
	register BKPTR bkpt;
	short rc;

	pcb.pcb_psl &= ~TBIT;
	delbp();
	if (execbkptf) {
		execbkptf = 0;
		runpcs(CONTIN, 1);
		/*NOTREACHED*/
	}
	if (!tracetrap && (bkpt = scanbkpt(userpc))) {
		/*
		 * Stopped at a breakpoint,
		 * execute any command.
		 */
		dot = bkpt->loc;
		if (bkpt->flag == BKPTEXEC ||
		    ((bkpt->flag = BKPTEXEC) && bkpt->comm[0] != EOR &&
		    command(bkpt->comm, ':') && --bkpt->count)) {
			loopcnt++;
			execbkpt(bkpt, execsig);
			execsig = 0;
		} else {
			bkpt->count = bkpt->initcnt;
			rc = 1;
		}
	} else {
		execsig = 0;
		rc = 0;
	}
	if (--loopcnt > 0)
		runpcs(rc ? CONTIN : SINGLE, 1);
	return (rc);
}

#define BPOUT 0
#define BPIN 1
static	int bpstate = BPOUT;

execbkpt(bkptr,execsig)
	BKPTR	bkptr;
{

	delbp();
	bkptr->flag=BKPTSET;
	execbkptf++;
	reset(SINGLE);
}

BKPTR
scanbkpt(addr)
	ADDR addr;
{
	register BKPTR	bkptr;

	for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt)
		if (bkptr->flag && bkptr->loc==addr)
			break;
	return (bkptr);
}

delbp()
{
	register ADDR a;
	register BKPTR bkptr;

	if (bpstate==BPOUT)
		return;
	for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt)
		if (bkptr->flag) {
			a=bkptr->loc;
			put(a, ISP, bkptr->ins);
		}
	bpstate=BPOUT;
}

setbp()
{
	register ADDR a;
	register BKPTR bkptr;

	if (bpstate==BPIN)
		return;
	for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt)
		if (bkptr->flag) {
			a = bkptr->loc;
			bkptr->ins = get(a, ISP);
			put(a, ISP, SETBP(bkptr->ins));
		}
	bpstate=BPIN;
}
