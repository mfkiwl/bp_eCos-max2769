#ifndef _LINUX_FS_H
#define _LINUX_FS_H

/*
 * Request flags.  For use in the cmd_flags field of struct request, and in
 * bi_rw of struct bio.  Note that some flags are only valid in either one.
 */
enum rq_flag_bits {
	/* common flags */
	__REQ_WRITE,		/* not set, read. set, write */
	__REQ_FAILFAST_DEV,	/* no driver retries of device errors */
	__REQ_FAILFAST_TRANSPORT, /* no driver retries of transport errors */
	__REQ_FAILFAST_DRIVER,	/* no driver retries of driver errors */

	__REQ_SYNC,		/* request is sync (sync write or read) */
	__REQ_META,		/* metadata io request */
	__REQ_DISCARD,		/* request to discard sectors */
	__REQ_NOIDLE,		/* don't anticipate more IO after this one */

	/* bio only flags */
	__REQ_RAHEAD,		/* read ahead, can fail anytime */
	__REQ_THROTTLED,	/* This bio has already been subjected to
				 * throttling rules. Don't do it again. */

	/* request only flags */
	__REQ_SORTED,		/* elevator knows about this request */
	__REQ_SOFTBARRIER,	/* may not be passed by ioscheduler */
	__REQ_FUA,		/* forced unit access */
	__REQ_NOMERGE,		/* don't touch this for merging */
	__REQ_STARTED,		/* drive already may have started this one */
	__REQ_DONTPREP,		/* don't call prep for this one */
	__REQ_QUEUED,		/* uses queueing */
	__REQ_ELVPRIV,		/* elevator private data attached */
	__REQ_FAILED,		/* set if the request failed */
	__REQ_QUIET,		/* don't worry about errors */
	__REQ_PREEMPT,		/* set for "ide_preempt" requests */
	__REQ_ALLOCED,		/* request came from our alloc pool */
	__REQ_COPY_USER,	/* contains copies of user pages */
	__REQ_FLUSH,		/* request for cache flush */
	__REQ_FLUSH_SEQ,	/* request for flush sequence */
	__REQ_IO_STAT,		/* account I/O stat */
	__REQ_MIXED_MERGE,	/* merge of different types, fail separately */
	__REQ_SECURE,		/* secure discard (used with __REQ_DISCARD) */
	__REQ_NR_BITS,		/* stops here */
};

#define REQ_WRITE		(1 << __REQ_WRITE)
#define REQ_FAILFAST_DEV	(1 << __REQ_FAILFAST_DEV)
#define REQ_FAILFAST_TRANSPORT	(1 << __REQ_FAILFAST_TRANSPORT)
#define REQ_FAILFAST_DRIVER	(1 << __REQ_FAILFAST_DRIVER)
#define REQ_SYNC		(1 << __REQ_SYNC)
#define REQ_META		(1 << __REQ_META)
#define REQ_DISCARD		(1 << __REQ_DISCARD)
#define REQ_NOIDLE		(1 << __REQ_NOIDLE)
#define REQ_SECURE      (1 << __REQ_SECURE)

#define RW_MASK			REQ_WRITE
#define RWA_MASK		REQ_RAHEAD

#define READ			0
#define WRITE			RW_MASK
#define READA			RWA_MASK

#define READ_SYNC		(READ | REQ_SYNC)
#define READ_META		(READ | REQ_META)
#define WRITE_SYNC		(WRITE | REQ_SYNC | REQ_NOIDLE)
#define WRITE_ODIRECT		(WRITE | REQ_SYNC)
#define WRITE_META		(WRITE | REQ_META)
#define WRITE_FLUSH		(WRITE | REQ_SYNC | REQ_NOIDLE | REQ_FLUSH)
#define WRITE_FUA		(WRITE | REQ_SYNC | REQ_NOIDLE | REQ_FUA)
#define WRITE_FLUSH_FUA		(WRITE | REQ_SYNC | REQ_NOIDLE | REQ_FLUSH | REQ_FUA)

#define SEL_IN		1
#define SEL_OUT		2
#define SEL_EX		4

#endif //_LINUX_FS_H
