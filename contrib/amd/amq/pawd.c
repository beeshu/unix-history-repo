/*
 * Copyright (c) 1997-1998 Erez Zadok
 * Copyright (c) 1990 Jan-Simon Pendry
 * Copyright (c) 1990 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry at Imperial College, London.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *      %W% (Berkeley) %G%
 *
 * $Id: pawd.c,v 5.2.2.1 1992/02/09 15:09:16 jsp beta $
 *
 */

/*
 * pawd is similar to pwd, except that it returns more "natural" versions of
 * pathnames for directories automounted with the amd automounter.  If any
 * arguments are given, the "more natural" form of the given pathnames are
 * printed.
 *
 * Paul Anderson (paul@ed.lfcs)
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amq.h>

/* dummy variables */
char *progname;
char hostname[MAXHOSTNAMELEN];
int orig_umask, foreground, debug_flags;
pid_t mypid;
serv_state amd_state;

/* statics */
static char *localhost="localhost";
static char newdir[MAXPATHLEN];
static char transform[MAXPATHLEN];

static int
find_mt(amq_mount_tree *mt, char *dir)
{
  while (mt) {
    if (STREQ(mt->mt_type, "link") || STREQ(mt->mt_type, "nfs")) {
      int len = strlen(mt->mt_mountpoint);
      if (NSTREQ(mt->mt_mountpoint, dir, len) &&
	  ((dir[len] == '\0') || (dir[len] == '/'))) {
	char tmp_buf[MAXPATHLEN];
	strcpy(tmp_buf, mt->mt_directory);
	strcat(tmp_buf, &dir[len]);
	strcpy(newdir, tmp_buf);
	return 1;
      }
    }
    if (find_mt(mt->mt_next,dir))
      return 1;
    mt = mt->mt_child;
  }
  return 0;
}


static int
find_mlp(amq_mount_tree_list *mlp, char *dir)
{
  int i;

  for (i = 0; i < mlp->amq_mount_tree_list_len; i++) {
    if (find_mt(mlp->amq_mount_tree_list_val[i], dir))
      return 1;
  }
  return 0;
}


#ifdef HAVE_CNODEID
static char *
cluster_server(void)
{
# ifdef HAVE_EXTERN_GETCCENT
  struct cct_entry *cp;
# endif /* HAVE_EXTERN_GETCCENT */

  if (cnodeid() == 0)
    return localhost;

# ifdef HAVE_EXTERN_GETCCENT
  while ((cp = getccent()))
    if (cp->cnode_type == 'r')
      return cp->cnode_name;
# endif /* HAVE_EXTERN_GETCCENT */

  return localhost;
}
#endif /* HAVE_CNODEID */


/* DISK_HOME_HACK added by gdmr */
#ifdef DISK_HOME_HACK
static char *
hack_name(char *dir)
{
  char partition[MAXPATHLEN];
  char username[MAXPATHLEN];
  char hesiod_lookup[MAXPATHLEN];
  char *to, *ch, *hes_name, *dot;
  char **hes;

#ifdef DEBUG
  fprintf(stderr, "hack_name(%s)\n", dir);
#endif /* DEBUG */

  if (dir[0] == '/' && dir[1] == 'a' && dir[2] == '/') {
    /* Could be /a/server/disk/home/partition/user... */
    ch = dir + 3;
    while (*ch && *ch != '/') ch++;  /* Skip server */
    if (!NSTREQ(ch, "/disk/home/", 11))
      return NULL;		/* Nope */
    /* Looking promising, next should be the partition name */
    ch += 11;
    to = partition;
    while (*ch && *ch != '/') *to++ = *ch++;
    to = '\0';
    if (!(*ch))
      return NULL;		/* Off the end */
    /* Now the username */
    ch++;
    to = username;
    while (*ch && *ch != '/') *to++ = *ch++;
    to = '\0';
#ifdef DEBUG
    fprintf(stderr, "partition %s, username %s\n", partition, username);
#endif /* DEBUG */

    sprintf(hesiod_lookup, "%s.homes-remote", username);
    hes = hes_resolve(hesiod_lookup, "amd");
    if (!hes)
      return NULL;
#ifdef DEBUG
    fprintf(stderr, "hesiod -> <%s>\n", *hes);
#endif /* DEBUG */
    hes_name = strstr(*hes, "/homes/remote/");
    if (!hes_name) return NULL;
    hes_name += 14;
#ifdef DEBUG
    fprintf(stderr, "hesiod -> <%s>\n", hes_name);
#endif /* DEBUG */
    dot = hes_name;
    while (*dot && *dot != '.') dot++;
    *dot = '\0';
#ifdef DEBUG
    fprintf(stderr, "hesiod -> <%s>\n", hes_name);
#endif /* DEBUG */

    if (strcmp(partition, hes_name)) return NULL;
#ifdef DEBUG
    fprintf(stderr, "A match, munging....\n");
#endif /* DEBUG */
    strcpy(transform, "/home/");
    strcat(transform, username);
    if (*ch) strcat(transform, ch);
#ifdef DEBUG
    fprintf(stderr, "Munged to <%s>\n", transform);
#endif /* DEBUG */
    return transform;
  }
  return NULL;
}
#endif /* DISK_HOME_HACK */


/*
 * The routine transform_dir(path) transforms pathnames of directories
 * mounted with the amd automounter to produce a more "natural" version.
 * The automount table is obtained from the local amd via the rpc interface
 * and reverse lookups are repeatedly performed on the directory name
 * substituting the name of the automount link for the value of the link
 * whenever it occurs as a prefix of the directory name.
 */
static char *
transform_dir(char *dir)
{
#ifdef DISK_HOME_HACK
  char *ch;
#endif /* DISK_HOME_HACK */
  char *server;
  struct sockaddr_in server_addr;
  int s = RPC_ANYSOCK;
  CLIENT *clnt;
  struct hostent *hp;
  amq_mount_tree_list *mlp;
  struct timeval tmo = {10, 0};

#ifdef DISK_HOME_HACK
  if (ch = hack_name(dir))
    return ch;
#endif /* DISK_HOME_HACK */

#ifdef HAVE_CNODEID
  server = cluster_server();
#else /* not HAVE_CNODEID */
  server = localhost;
#endif /* not HAVE_CNODEID */

  if ((hp = gethostbyname(server)) == 0)
    return dir;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr = *(struct in_addr *) hp->h_addr;

  clnt = clntudp_create(&server_addr, AMQ_PROGRAM, AMQ_VERSION, tmo, &s);
  if (clnt == 0)
    return dir;

  strcpy(transform,dir);
  while ( (mlp = amqproc_export_1((voidp)0, clnt)) &&
	  find_mlp(mlp,transform) ) {
    strcpy(transform,newdir);
  }
  return transform;
}


/* getawd() is a substitute for getwd() which transforms the path */
static char *
getawd(char *path)
{
#ifdef HAVE_GETCWD
  char *wd = getcwd(path, MAXPATHLEN+1);
#else /* not HAVE_GETCWD */
  char *wd = getwd(path);
#endif /* not HAVE_GETCWD */

  if (wd == NULL) {
    return NULL;
  }
  strcpy(path, transform_dir(wd));
  return path;
}


int
main(int argc, char *argv[])
{
  char tmp_buf[MAXPATHLEN], *wd;

  if (argc == 1) {
    wd = getawd(tmp_buf);
    if (wd == NULL) {
      fprintf(stderr, "pawd: %s\n", tmp_buf);
      exit(1);
    } else {
      fprintf(stdout, "%s\n", wd);
    }
  } else {
    while (--argc) {
      wd = transform_dir(*++argv);
      fprintf(stdout, "%s\n", wd);
    }
  }
  exit(0);
}

