/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */
/* $FreeBSD$ */

#ifndef _NLM_PROT_H_RPCGEN
#define	_NLM_PROT_H_RPCGEN

#include <rpc/rpc.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LM_MAXSTRLEN 1024
#define MAXNAMELEN LM_MAXSTRLEN+1

enum nlm_stats {
	nlm_granted = 0,
	nlm_denied = 1,
	nlm_denied_nolocks = 2,
	nlm_blocked = 3,
	nlm_denied_grace_period = 4,
	nlm_deadlck = 5
};
typedef enum nlm_stats nlm_stats;

struct nlm_holder {
	bool_t exclusive;
	int svid;
	netobj oh;
	u_int l_offset;
	u_int l_len;
};
typedef struct nlm_holder nlm_holder;

struct nlm_testrply {
	nlm_stats stat;
	union {
		struct nlm_holder holder;
	} nlm_testrply_u;
};
typedef struct nlm_testrply nlm_testrply;

struct nlm_stat {
	nlm_stats stat;
};
typedef struct nlm_stat nlm_stat;

struct nlm_res {
	netobj cookie;
	nlm_stat stat;
};
typedef struct nlm_res nlm_res;

struct nlm_testres {
	netobj cookie;
	nlm_testrply stat;
};
typedef struct nlm_testres nlm_testres;

struct nlm_lock {
	char *caller_name;
	netobj fh;
	netobj oh;
	int svid;
	u_int l_offset;
	u_int l_len;
};
typedef struct nlm_lock nlm_lock;

struct nlm_lockargs {
	netobj cookie;
	bool_t block;
	bool_t exclusive;
	struct nlm_lock alock;
	bool_t reclaim;
	int state;
};
typedef struct nlm_lockargs nlm_lockargs;

struct nlm_cancargs {
	netobj cookie;
	bool_t block;
	bool_t exclusive;
	struct nlm_lock alock;
};
typedef struct nlm_cancargs nlm_cancargs;

struct nlm_testargs {
	netobj cookie;
	bool_t exclusive;
	struct nlm_lock alock;
};
typedef struct nlm_testargs nlm_testargs;

struct nlm_unlockargs {
	netobj cookie;
	struct nlm_lock alock;
};
typedef struct nlm_unlockargs nlm_unlockargs;
/*
 * The following enums are actually bit encoded for efficient
 * boolean algebra.... DON'T change them.....
 */

enum fsh_mode {
	fsm_DN = 0,
	fsm_DR = 1,
	fsm_DW = 2,
	fsm_DRW = 3
};
typedef enum fsh_mode fsh_mode;

enum fsh_access {
	fsa_NONE = 0,
	fsa_R = 1,
	fsa_W = 2,
	fsa_RW = 3
};
typedef enum fsh_access fsh_access;

struct nlm_share {
	char *caller_name;
	netobj fh;
	netobj oh;
	fsh_mode mode;
	fsh_access access;
};
typedef struct nlm_share nlm_share;

struct nlm_shareargs {
	netobj cookie;
	nlm_share share;
	bool_t reclaim;
};
typedef struct nlm_shareargs nlm_shareargs;

struct nlm_shareres {
	netobj cookie;
	nlm_stats stat;
	int sequence;
};
typedef struct nlm_shareres nlm_shareres;

struct nlm_notify {
	char *name;
	long state;
};
typedef struct nlm_notify nlm_notify;
/* definitions for NLM version 4 */

enum nlm4_stats {
	nlm4_granted = 0,
	nlm4_denied = 1,
	nlm4_denied_nolocks = 2,
	nlm4_blocked = 3,
	nlm4_denied_grace_period = 4,
	nlm4_deadlck = 5,
	nlm4_rofs = 6,
	nlm4_stale_fh = 7,
	nlm4_fbig = 8,
	nlm4_failed = 9
};
typedef enum nlm4_stats nlm4_stats;

struct nlm4_stat {
	nlm4_stats stat;
};
typedef struct nlm4_stat nlm4_stat;

struct nlm4_holder {
	bool_t exclusive;
	u_int32_t svid;
	netobj oh;
	u_int64_t l_offset;
	u_int64_t l_len;
};
typedef struct nlm4_holder nlm4_holder;

struct nlm4_lock {
	char *caller_name;
	netobj fh;
	netobj oh;
	u_int32_t svid;
	u_int64_t l_offset;
	u_int64_t l_len;
};
typedef struct nlm4_lock nlm4_lock;

struct nlm4_share {
	char *caller_name;
	netobj fh;
	netobj oh;
	fsh_mode mode;
	fsh_access access;
};
typedef struct nlm4_share nlm4_share;

struct nlm4_testrply {
	nlm4_stats stat;
	union {
		struct nlm4_holder holder;
	} nlm4_testrply_u;
};
typedef struct nlm4_testrply nlm4_testrply;

struct nlm4_testres {
	netobj cookie;
	nlm4_testrply stat;
};
typedef struct nlm4_testres nlm4_testres;

struct nlm4_testargs {
	netobj cookie;
	bool_t exclusive;
	struct nlm4_lock alock;
};
typedef struct nlm4_testargs nlm4_testargs;

struct nlm4_res {
	netobj cookie;
	nlm4_stat stat;
};
typedef struct nlm4_res nlm4_res;

struct nlm4_lockargs {
	netobj cookie;
	bool_t block;
	bool_t exclusive;
	struct nlm4_lock alock;
	bool_t reclaim;
	int state;
};
typedef struct nlm4_lockargs nlm4_lockargs;

struct nlm4_cancargs {
	netobj cookie;
	bool_t block;
	bool_t exclusive;
	struct nlm4_lock alock;
};
typedef struct nlm4_cancargs nlm4_cancargs;

struct nlm4_unlockargs {
	netobj cookie;
	struct nlm4_lock alock;
};
typedef struct nlm4_unlockargs nlm4_unlockargs;

struct nlm4_shareargs {
	netobj cookie;
	nlm4_share share;
	bool_t reclaim;
};
typedef struct nlm4_shareargs nlm4_shareargs;

struct nlm4_shareres {
	netobj cookie;
	nlm4_stats stat;
	int sequence;
};
typedef struct nlm4_shareres nlm4_shareres;

struct nlm_sm_status {
	char *mon_name;
	int state;
	char priv[16];
};
typedef struct nlm_sm_status nlm_sm_status;

struct nlm4_notify {
	char *name;
	int32_t state;
};
typedef struct nlm4_notify nlm4_notify;

#define	NLM_PROG ((unsigned long)(100021))
#define	NLM_SM ((unsigned long)(0))

#define	NLM_SM_NOTIFY ((unsigned long)(1))
extern  enum clnt_stat nlm_sm_notify_0(struct nlm_sm_status *, void *, CLIENT *);
extern  bool_t nlm_sm_notify_0_svc(struct nlm_sm_status *, void *, struct svc_req *);
#define	NLM_VERS ((unsigned long)(1))

#define	NLM_TEST ((unsigned long)(1))
extern  enum clnt_stat nlm_test_1(struct nlm_testargs *, nlm_testres *, CLIENT *);
extern  bool_t nlm_test_1_svc(struct nlm_testargs *, nlm_testres *, struct svc_req *);
#define	NLM_LOCK ((unsigned long)(2))
extern  enum clnt_stat nlm_lock_1(struct nlm_lockargs *, nlm_res *, CLIENT *);
extern  bool_t nlm_lock_1_svc(struct nlm_lockargs *, nlm_res *, struct svc_req *);
#define	NLM_CANCEL ((unsigned long)(3))
extern  enum clnt_stat nlm_cancel_1(struct nlm_cancargs *, nlm_res *, CLIENT *);
extern  bool_t nlm_cancel_1_svc(struct nlm_cancargs *, nlm_res *, struct svc_req *);
#define	NLM_UNLOCK ((unsigned long)(4))
extern  enum clnt_stat nlm_unlock_1(struct nlm_unlockargs *, nlm_res *, CLIENT *);
extern  bool_t nlm_unlock_1_svc(struct nlm_unlockargs *, nlm_res *, struct svc_req *);
#define	NLM_GRANTED ((unsigned long)(5))
extern  enum clnt_stat nlm_granted_1(struct nlm_testargs *, nlm_res *, CLIENT *);
extern  bool_t nlm_granted_1_svc(struct nlm_testargs *, nlm_res *, struct svc_req *);
#define	NLM_TEST_MSG ((unsigned long)(6))
extern  enum clnt_stat nlm_test_msg_1(struct nlm_testargs *, void *, CLIENT *);
extern  bool_t nlm_test_msg_1_svc(struct nlm_testargs *, void *, struct svc_req *);
#define	NLM_LOCK_MSG ((unsigned long)(7))
extern  enum clnt_stat nlm_lock_msg_1(struct nlm_lockargs *, void *, CLIENT *);
extern  bool_t nlm_lock_msg_1_svc(struct nlm_lockargs *, void *, struct svc_req *);
#define	NLM_CANCEL_MSG ((unsigned long)(8))
extern  enum clnt_stat nlm_cancel_msg_1(struct nlm_cancargs *, void *, CLIENT *);
extern  bool_t nlm_cancel_msg_1_svc(struct nlm_cancargs *, void *, struct svc_req *);
#define	NLM_UNLOCK_MSG ((unsigned long)(9))
extern  enum clnt_stat nlm_unlock_msg_1(struct nlm_unlockargs *, void *, CLIENT *);
extern  bool_t nlm_unlock_msg_1_svc(struct nlm_unlockargs *, void *, struct svc_req *);
#define	NLM_GRANTED_MSG ((unsigned long)(10))
extern  enum clnt_stat nlm_granted_msg_1(struct nlm_testargs *, void *, CLIENT *);
extern  bool_t nlm_granted_msg_1_svc(struct nlm_testargs *, void *, struct svc_req *);
#define	NLM_TEST_RES ((unsigned long)(11))
extern  enum clnt_stat nlm_test_res_1(nlm_testres *, void *, CLIENT *);
extern  bool_t nlm_test_res_1_svc(nlm_testres *, void *, struct svc_req *);
#define	NLM_LOCK_RES ((unsigned long)(12))
extern  enum clnt_stat nlm_lock_res_1(nlm_res *, void *, CLIENT *);
extern  bool_t nlm_lock_res_1_svc(nlm_res *, void *, struct svc_req *);
#define	NLM_CANCEL_RES ((unsigned long)(13))
extern  enum clnt_stat nlm_cancel_res_1(nlm_res *, void *, CLIENT *);
extern  bool_t nlm_cancel_res_1_svc(nlm_res *, void *, struct svc_req *);
#define	NLM_UNLOCK_RES ((unsigned long)(14))
extern  enum clnt_stat nlm_unlock_res_1(nlm_res *, void *, CLIENT *);
extern  bool_t nlm_unlock_res_1_svc(nlm_res *, void *, struct svc_req *);
#define	NLM_GRANTED_RES ((unsigned long)(15))
extern  enum clnt_stat nlm_granted_res_1(nlm_res *, void *, CLIENT *);
extern  bool_t nlm_granted_res_1_svc(nlm_res *, void *, struct svc_req *);
extern int nlm_prog_1_freeresult(SVCXPRT *, xdrproc_t, caddr_t);
#define	NLM_VERSX ((unsigned long)(3))

#define	NLM_SHARE ((unsigned long)(20))
extern  enum clnt_stat nlm_share_3(nlm_shareargs *, nlm_shareres *, CLIENT *);
extern  bool_t nlm_share_3_svc(nlm_shareargs *, nlm_shareres *, struct svc_req *);
#define	NLM_UNSHARE ((unsigned long)(21))
extern  enum clnt_stat nlm_unshare_3(nlm_shareargs *, nlm_shareres *, CLIENT *);
extern  bool_t nlm_unshare_3_svc(nlm_shareargs *, nlm_shareres *, struct svc_req *);
#define	NLM_NM_LOCK ((unsigned long)(22))
extern  enum clnt_stat nlm_nm_lock_3(nlm_lockargs *, nlm_res *, CLIENT *);
extern  bool_t nlm_nm_lock_3_svc(nlm_lockargs *, nlm_res *, struct svc_req *);
#define	NLM_FREE_ALL ((unsigned long)(23))
extern  enum clnt_stat nlm_free_all_3(nlm_notify *, void *, CLIENT *);
extern  bool_t nlm_free_all_3_svc(nlm_notify *, void *, struct svc_req *);
extern int nlm_prog_3_freeresult(SVCXPRT *, xdrproc_t, caddr_t);
#define	NLM_VERS4 ((unsigned long)(4))

#define	NLM4_TEST ((unsigned long)(1))
extern  enum clnt_stat nlm4_test_4(nlm4_testargs *, nlm4_testres *, CLIENT *);
extern  bool_t nlm4_test_4_svc(nlm4_testargs *, nlm4_testres *, struct svc_req *);
#define	NLM4_LOCK ((unsigned long)(2))
extern  enum clnt_stat nlm4_lock_4(nlm4_lockargs *, nlm4_res *, CLIENT *);
extern  bool_t nlm4_lock_4_svc(nlm4_lockargs *, nlm4_res *, struct svc_req *);
#define	NLM4_CANCEL ((unsigned long)(3))
extern  enum clnt_stat nlm4_cancel_4(nlm4_cancargs *, nlm4_res *, CLIENT *);
extern  bool_t nlm4_cancel_4_svc(nlm4_cancargs *, nlm4_res *, struct svc_req *);
#define	NLM4_UNLOCK ((unsigned long)(4))
extern  enum clnt_stat nlm4_unlock_4(nlm4_unlockargs *, nlm4_res *, CLIENT *);
extern  bool_t nlm4_unlock_4_svc(nlm4_unlockargs *, nlm4_res *, struct svc_req *);
#define	NLM4_GRANTED ((unsigned long)(5))
extern  enum clnt_stat nlm4_granted_4(nlm4_testargs *, nlm4_res *, CLIENT *);
extern  bool_t nlm4_granted_4_svc(nlm4_testargs *, nlm4_res *, struct svc_req *);
#define	NLM4_TEST_MSG ((unsigned long)(6))
extern  enum clnt_stat nlm4_test_msg_4(nlm4_testargs *, void *, CLIENT *);
extern  bool_t nlm4_test_msg_4_svc(nlm4_testargs *, void *, struct svc_req *);
#define	NLM4_LOCK_MSG ((unsigned long)(7))
extern  enum clnt_stat nlm4_lock_msg_4(nlm4_lockargs *, void *, CLIENT *);
extern  bool_t nlm4_lock_msg_4_svc(nlm4_lockargs *, void *, struct svc_req *);
#define	NLM4_CANCEL_MSG ((unsigned long)(8))
extern  enum clnt_stat nlm4_cancel_msg_4(nlm4_cancargs *, void *, CLIENT *);
extern  bool_t nlm4_cancel_msg_4_svc(nlm4_cancargs *, void *, struct svc_req *);
#define	NLM4_UNLOCK_MSG ((unsigned long)(9))
extern  enum clnt_stat nlm4_unlock_msg_4(nlm4_unlockargs *, void *, CLIENT *);
extern  bool_t nlm4_unlock_msg_4_svc(nlm4_unlockargs *, void *, struct svc_req *);
#define	NLM4_GRANTED_MSG ((unsigned long)(10))
extern  enum clnt_stat nlm4_granted_msg_4(nlm4_testargs *, void *, CLIENT *);
extern  bool_t nlm4_granted_msg_4_svc(nlm4_testargs *, void *, struct svc_req *);
#define	NLM4_TEST_RES ((unsigned long)(11))
extern  enum clnt_stat nlm4_test_res_4(nlm4_testres *, void *, CLIENT *);
extern  bool_t nlm4_test_res_4_svc(nlm4_testres *, void *, struct svc_req *);
#define	NLM4_LOCK_RES ((unsigned long)(12))
extern  enum clnt_stat nlm4_lock_res_4(nlm4_res *, void *, CLIENT *);
extern  bool_t nlm4_lock_res_4_svc(nlm4_res *, void *, struct svc_req *);
#define	NLM4_CANCEL_RES ((unsigned long)(13))
extern  enum clnt_stat nlm4_cancel_res_4(nlm4_res *, void *, CLIENT *);
extern  bool_t nlm4_cancel_res_4_svc(nlm4_res *, void *, struct svc_req *);
#define	NLM4_UNLOCK_RES ((unsigned long)(14))
extern  enum clnt_stat nlm4_unlock_res_4(nlm4_res *, void *, CLIENT *);
extern  bool_t nlm4_unlock_res_4_svc(nlm4_res *, void *, struct svc_req *);
#define	NLM4_GRANTED_RES ((unsigned long)(15))
extern  enum clnt_stat nlm4_granted_res_4(nlm4_res *, void *, CLIENT *);
extern  bool_t nlm4_granted_res_4_svc(nlm4_res *, void *, struct svc_req *);
#define	NLM4_SHARE ((unsigned long)(20))
extern  enum clnt_stat nlm4_share_4(nlm4_shareargs *, nlm4_shareres *, CLIENT *);
extern  bool_t nlm4_share_4_svc(nlm4_shareargs *, nlm4_shareres *, struct svc_req *);
#define	NLM4_UNSHARE ((unsigned long)(21))
extern  enum clnt_stat nlm4_unshare_4(nlm4_shareargs *, nlm4_shareres *, CLIENT *);
extern  bool_t nlm4_unshare_4_svc(nlm4_shareargs *, nlm4_shareres *, struct svc_req *);
#define	NLM4_NM_LOCK ((unsigned long)(22))
extern  enum clnt_stat nlm4_nm_lock_4(nlm4_lockargs *, nlm4_res *, CLIENT *);
extern  bool_t nlm4_nm_lock_4_svc(nlm4_lockargs *, nlm4_res *, struct svc_req *);
#define	NLM4_FREE_ALL ((unsigned long)(23))
extern  enum clnt_stat nlm4_free_all_4(nlm4_notify *, void *, CLIENT *);
extern  bool_t nlm4_free_all_4_svc(nlm4_notify *, void *, struct svc_req *);
extern int nlm_prog_4_freeresult(SVCXPRT *, xdrproc_t, caddr_t);

/* the xdr functions */
extern  bool_t xdr_nlm_stats(XDR *, nlm_stats*);
extern  bool_t xdr_nlm_holder(XDR *, nlm_holder*);
extern  bool_t xdr_nlm_testrply(XDR *, nlm_testrply*);
extern  bool_t xdr_nlm_stat(XDR *, nlm_stat*);
extern  bool_t xdr_nlm_res(XDR *, nlm_res*);
extern  bool_t xdr_nlm_testres(XDR *, nlm_testres*);
extern  bool_t xdr_nlm_lock(XDR *, nlm_lock*);
extern  bool_t xdr_nlm_lockargs(XDR *, nlm_lockargs*);
extern  bool_t xdr_nlm_cancargs(XDR *, nlm_cancargs*);
extern  bool_t xdr_nlm_testargs(XDR *, nlm_testargs*);
extern  bool_t xdr_nlm_unlockargs(XDR *, nlm_unlockargs*);
extern  bool_t xdr_fsh_mode(XDR *, fsh_mode*);
extern  bool_t xdr_fsh_access(XDR *, fsh_access*);
extern  bool_t xdr_nlm_share(XDR *, nlm_share*);
extern  bool_t xdr_nlm_shareargs(XDR *, nlm_shareargs*);
extern  bool_t xdr_nlm_shareres(XDR *, nlm_shareres*);
extern  bool_t xdr_nlm_notify(XDR *, nlm_notify*);
extern  bool_t xdr_nlm4_stats(XDR *, nlm4_stats*);
extern  bool_t xdr_nlm4_stat(XDR *, nlm4_stat*);
extern  bool_t xdr_nlm4_holder(XDR *, nlm4_holder*);
extern  bool_t xdr_nlm4_lock(XDR *, nlm4_lock*);
extern  bool_t xdr_nlm4_share(XDR *, nlm4_share*);
extern  bool_t xdr_nlm4_testrply(XDR *, nlm4_testrply*);
extern  bool_t xdr_nlm4_testres(XDR *, nlm4_testres*);
extern  bool_t xdr_nlm4_testargs(XDR *, nlm4_testargs*);
extern  bool_t xdr_nlm4_res(XDR *, nlm4_res*);
extern  bool_t xdr_nlm4_lockargs(XDR *, nlm4_lockargs*);
extern  bool_t xdr_nlm4_cancargs(XDR *, nlm4_cancargs*);
extern  bool_t xdr_nlm4_unlockargs(XDR *, nlm4_unlockargs*);
extern  bool_t xdr_nlm4_shareargs(XDR *, nlm4_shareargs*);
extern  bool_t xdr_nlm4_shareres(XDR *, nlm4_shareres*);
extern  bool_t xdr_nlm_sm_status(XDR *, nlm_sm_status*);
extern  bool_t xdr_nlm4_notify(XDR *, nlm4_notify*);

#ifdef __cplusplus
}
#endif

#endif /* !_NLM_PROT_H_RPCGEN */
