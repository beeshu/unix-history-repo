/*-
 * Copyright (c) 1998 Alex Nash
 * Copyright (c) 2004 Michael Telahun Makonnen
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
 * $FreeBSD$
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include <pthread.h>
#include "thr_private.h"

/* maximum number of times a read lock may be obtained */
#define	MAX_READ_LOCKS		(INT_MAX - 1)

__weak_reference(_pthread_rwlock_destroy, pthread_rwlock_destroy);
__weak_reference(_pthread_rwlock_init, pthread_rwlock_init);
__weak_reference(_pthread_rwlock_rdlock, pthread_rwlock_rdlock);
__weak_reference(_pthread_rwlock_tryrdlock, pthread_rwlock_tryrdlock);
__weak_reference(_pthread_rwlock_trywrlock, pthread_rwlock_trywrlock);
__weak_reference(_pthread_rwlock_unlock, pthread_rwlock_unlock);
__weak_reference(_pthread_rwlock_wrlock, pthread_rwlock_wrlock);

static int	rwlock_rdlock_common(pthread_rwlock_t *, int);
static int	rwlock_wrlock_common(pthread_rwlock_t *, int);

int
_pthread_rwlock_destroy (pthread_rwlock_t *rwlock)
{
	pthread_rwlock_t prwlock;

	if (rwlock == NULL || *rwlock == NULL)
		return (EINVAL);

	prwlock = *rwlock;

	if (prwlock->state != 0)
		return (EBUSY);

	pthread_mutex_destroy(&prwlock->lock);
	pthread_cond_destroy(&prwlock->read_signal);
	pthread_cond_destroy(&prwlock->write_signal);
	free(prwlock);

	*rwlock = NULL;

	return (0);
}

int
_pthread_rwlock_init (pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr)
{
	pthread_rwlock_t	prwlock;
	int			ret;

	/* allocate rwlock object */
	prwlock = (pthread_rwlock_t)malloc(sizeof(struct pthread_rwlock));

	if (prwlock == NULL)
		return(ENOMEM);

	/* initialize the lock */
	if ((ret = pthread_mutex_init(&prwlock->lock, NULL)) != 0)
		goto out_mutex;

	/* initialize the read condition signal */
	if ((ret = pthread_cond_init(&prwlock->read_signal, NULL)) != 0)
		goto out_readcond;

	/* initialize the write condition signal */
	if ((ret = pthread_cond_init(&prwlock->write_signal, NULL)) != 0)
		goto out_writecond;

	/* success */
	prwlock->state		 = 0;
	prwlock->blocked_writers = 0;

	*rwlock = prwlock;
	return (0);

out_writecond:
	pthread_cond_destroy(&prwlock->read_signal);
out_readcond:
	pthread_mutex_destroy(&prwlock->lock);
out_mutex:
	free(prwlock);
	return(ret);
}

/*
 * If nonblocking is 0 this function will wait on the lock. If
 * it is greater than 0 it will return immediately with EBUSY.
 */
static int
rwlock_rdlock_common(pthread_rwlock_t *rwlock, int nonblocking)
{
	pthread_rwlock_t 	prwlock;
	int			ret;

	if (rwlock == NULL || *rwlock == NULL)
		return(EINVAL);

	prwlock = *rwlock;

	/* grab the monitor lock */
	if ((ret = pthread_mutex_lock(&prwlock->lock)) != 0)
		return(ret);

	/* check lock count */
	if (prwlock->state == MAX_READ_LOCKS) {
		pthread_mutex_unlock(&prwlock->lock);
		return (EAGAIN);
	}

	/* give writers priority over readers */
	while (prwlock->blocked_writers || prwlock->state < 0) {
		if (nonblocking) {
			pthread_mutex_unlock(&prwlock->lock);
			return (EBUSY);
		}

		ret = pthread_cond_wait(&prwlock->read_signal, &prwlock->lock);

		if (ret != 0 && ret != EINTR) {
			/* can't do a whole lot if this fails */
			pthread_mutex_unlock(&prwlock->lock);
			return(ret);
		}
	}

	++prwlock->state; /* indicate we are locked for reading */

	/*
	 * Something is really wrong if this call fails.  Returning
	 * error won't do because we've already obtained the read
	 * lock.  Decrementing 'state' is no good because we probably
	 * don't have the monitor lock.
	 */
	pthread_mutex_unlock(&prwlock->lock);

	return(0);
}

int
_pthread_rwlock_rdlock (pthread_rwlock_t *rwlock)
{
	return (rwlock_rdlock_common(rwlock, 0));
}

int
_pthread_rwlock_tryrdlock (pthread_rwlock_t *rwlock)
{
	return (rwlock_rdlock_common(rwlock, 1));
}

int
_pthread_rwlock_unlock (pthread_rwlock_t *rwlock)
{
	pthread_rwlock_t 	prwlock;
	int			ret;

	if (rwlock == NULL || *rwlock == NULL)
		return(EINVAL);

	prwlock = *rwlock;

	/* grab the monitor lock */
	if ((ret = pthread_mutex_lock(&prwlock->lock)) != 0)
		return(ret);

	/* XXX - Make sure we hold a lock on this rwlock */

	if (prwlock->state > 0) {
		if (--prwlock->state == 0 && prwlock->blocked_writers)
			ret = pthread_cond_signal(&prwlock->write_signal);
	} else if (prwlock->state < 0) {
		prwlock->state = 0;

		if (prwlock->blocked_writers)
			ret = pthread_cond_signal(&prwlock->write_signal);
		else
			ret = pthread_cond_broadcast(&prwlock->read_signal);
	}

	/* see the comment on this in rwlock_rdlock_common */
	pthread_mutex_unlock(&prwlock->lock);

	return(ret);
}

int
_pthread_rwlock_wrlock (pthread_rwlock_t *rwlock)
{
	return (rwlock_wrlock_common(rwlock, 0));
}

int
_pthread_rwlock_trywrlock (pthread_rwlock_t *rwlock)
{
	return (rwlock_wrlock_common(rwlock, 1));
}

/*
 * If nonblocking is 0 this function will wait on the lock. If
 * it is greater than 0 it will return immediately with EBUSY.
 */
static int
rwlock_wrlock_common(pthread_rwlock_t *rwlock, int nonblocking)
{
	pthread_rwlock_t 	prwlock;
	int			ret;

	if (rwlock == NULL || *rwlock == NULL)
		return(EINVAL);

	prwlock = *rwlock;

	/* grab the monitor lock */
	if ((ret = pthread_mutex_lock(&prwlock->lock)) != 0)
		return(ret);

	while (prwlock->state != 0) {
		if (nonblocking) {
			pthread_mutex_unlock(&prwlock->lock);
			return (EBUSY);
		}

		++prwlock->blocked_writers;

		ret = pthread_cond_wait(&prwlock->write_signal,
		    &prwlock->lock);

		if (ret != 0 && ret != EINTR) {
			--prwlock->blocked_writers;
			pthread_mutex_unlock(&prwlock->lock);
			return(ret);
		}

		--prwlock->blocked_writers;
	}

	/* indicate we are locked for writing */
	prwlock->state = -1;

	/* see the comment on this in pthread_rwlock_rdlock */
	pthread_mutex_unlock(&prwlock->lock);

	return(0);
}

