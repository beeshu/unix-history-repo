#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include <dialog.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include "sysinstall.h"

/* Macros and magic values */
#define MAX_MENU	13

/* Smarter strdup */
inline char *
_strdup(char *ptr)
{
    return ptr ? strdup(ptr) : ptr;
}

static char *descrs[] = {
    "Package Selection",
    "To select a package or category, move to it and press SPACE.\n"
	"To remove a package from consideration, press SPACE again.\n"
	    "To go to a previous menu, select UP item or Cancel. To search\n"
		"for a package by name, press ESC.",
    "Package Targets", "These are the packages you've selected for extraction.\n\n"
	"If you're sure of these choices, select OK.\n"
	    "If not, select Cancel to go back to the package selection menu.\n",
    "All", "All available packages in all categories.",
    "applications", "User application software.",
    "archivers", "Utilities for archiving and unarchiving data.",
    "audio", "Audio utilities - most require a supported sound card.",
    "benchmarks", "Utilities for measuring system performance.",
    "benchmarking", "Utilities for measuring system performance.",
    "cad", "Computer Aided Design utilities.",
    "comms", "Communications utilities.",
    "databases", "Database software.",
    "devel", "Software development utilities and libraries.",
    "development", "Software development utilities and libraries.",
    "documentation", "Document preparation utilities.",
    "editors", "Common text editors.",
    "emulation", "Utilities for emulating other OS types.",
    "emulators", "Utilities for emulating other OS types.",
    "games", "Various and sundry amusements.",
    "graphics", "Graphics libraries and utilities.",
    "japanese", "Ported software for the Japanese market.",
    "lang", "Computer languages.",
    "languages", "Computer languages.",
    "libraries", "Software development libraries.",
    "mail", "Electronic mail packages and utilities.",
    "math", "Mathematical computation software.",
    "net", "Networking utilities.",
    "networking", "Networking utilities.",
    "news", "USENET News support software.",
    "numeric", "Mathematical computation software.",
    "orphans", "Packages without a home elsewhere.",
    "plan9", "Software from the plan9 Operating System.",
    "print", "Utilities for dealing with printing.",
    "printing", "Utilities for dealing with printing.",
    "programming", "Software development utilities and libraries.",
    "russian", "Ported software for the Russian market.",
    "security", "System security software.",
    "shells", "Various shells (tcsh, bash, etc).",
    "sysutils", "Various system utilities.",
    "troff", "TROFF Text formatting utilities.",
    "utils", "Various user utilities.",
    "utilities", "Various user utilities.",
    "x11", "X Window System based utilities.",
    NULL, NULL,
};

static char *make_playpen(char *pen, size_t sz);

static char *
fetch_desc(char *name)
{
    int i;

    for (i = 0; descrs[i]; i += 2) {
	if (!strcmp(descrs[i], name))
	    return descrs[i + 1];
    }
    return "No description provided";
}

static PkgNodePtr
new_pkg_node(char *name, node_type type)
{
    PkgNodePtr tmp = malloc(sizeof(PkgNode));

    if (!tmp) {
	fprintf(stderr, "Out of memory - unable to allocate a new package node!\n");
	exit(1);
    }
    bzero(tmp, sizeof(PkgNode));
    tmp->name = _strdup(name);
    tmp->type = type;
    return tmp;
}

static IndexEntryPtr
new_index(char *name, char *pathto, char *prefix, char *comment, char *descr, char *maint)
{
    IndexEntryPtr tmp = malloc(sizeof(IndexEntry));

    if (!tmp) {
	fprintf(stderr, "Out of memory - unable to allocate a new index node!\n");
	exit(1);
    }

    bzero(tmp, sizeof(IndexEntry));
    tmp->name =		_strdup(name);
    tmp->path =		_strdup(pathto);
    tmp->prefix =	_strdup(prefix);
    tmp->comment =	_strdup(comment);
    tmp->descrfile =	_strdup(descr);
    tmp->maintainer =	_strdup(maint);
    return tmp;
}

static char *
clip(char *str, int len)
{
    int len2;

    len2 = strlen(str);
    if (len2 > len)
	str[len] = '\0';
    return str;
}

static void
index_register(PkgNodePtr top, char *where, IndexEntryPtr ptr)
{
    PkgNodePtr p, q;

    q = NULL;
    p = top->kids;
    while (p) {
	if (!strcmp(p->name, where)) {
	    q = p;
	    break;
	}
	p = p->next;
    }
    if (!p) {
	/* Add new category */
	q = new_pkg_node(where, PLACE);
	q->desc = fetch_desc(where);
	q->next = top->kids;
	top->kids = q;
    }
    p = new_pkg_node(ptr->name, PACKAGE);
    p->desc = clip(ptr->comment, 54 - (strlen(ptr->name) / 2));
    p->data = ptr;
    p->next = q->kids;
    q->kids = p;
}

static int
copy_to_sep(char *to, char *from, int sep)
{
    char *tok;

    tok = strchr(from, sep);
    if (!tok) {
	fprintf(stderr, "missing '%c' token.\n", sep);
	*to = '\0';
	return 0;
    }
    *tok = '\0';
    strcpy(to, from);
    return tok + 1 - from;
}

int
index_parse(FILE *fp, char *name, char *pathto, char *prefix, char *comment, char *descr, char *maint,
	    char *cats, char *keys)
{
    char line[1024];
    char *cp;
    int len;

    if (!fgets(line, 1024, fp)) {
	if (feof(fp))
	    return EOF;
	else
	    return -2;
    }
    len = strlen(line);
    if (line[len - 1] == '\n')
	line[len - 1] = '\0';
    cp = line;
    cp += copy_to_sep(name, cp, '|');
    cp += copy_to_sep(pathto, cp, '|');
    cp += copy_to_sep(prefix, cp, '|');
    cp += copy_to_sep(comment, cp, '|');
    cp += copy_to_sep(descr, cp, '|');
    cp += copy_to_sep(maint, cp, '|');
    cp += copy_to_sep(cats, cp, '|');
    strcpy(keys, cp);
    return 0;
}

int
index_read(char *fname, PkgNodePtr papa)
{
    FILE *fp;
    int i;

    fp = fopen(fname, "r");
    if (!fp) {
	fprintf(stderr, "Unable to open index file `%s' for reading.\n", fname);
	i = -1;
    }
    else {
	i = index_fread(fp, papa);
	fclose(fp);
    }
    return i;
}

int
index_fread(FILE *fp, PkgNodePtr papa)
{
    char name[127], pathto[255], prefix[255], comment[255], descr[127], maint[127], cats[511], keys[511];

    while (!index_parse(fp, name, pathto, prefix, comment, descr, maint, cats, keys)) {
	char *cp, *cp2, tmp[511];
	IndexEntryPtr idx;

	idx = new_index(name, pathto, prefix, comment, descr, maint);
	/* For now, we only add things to menus if they're in categories.  Keywords are ignored */
	cp = strcpy(tmp, cats);
	while ((cp2 = strchr(cp, ' ')) != NULL) {
	    *cp2 = '\0';
	    index_register(papa, cp, idx);
	    cp = cp2 + 1;
	}
	index_register(papa, cp, idx);

	/* Add to special "All" category */
	index_register(papa, "All", idx);
    }
    fclose(fp);
    return 0;
}

void
index_init(PkgNodePtr top, PkgNodePtr plist)
{
    top->next = top->kids = NULL;
    top->name = "Package Selection";
    top->type = PLACE;
    top->desc = fetch_desc(top->name);

    plist->next = plist->kids = NULL;
    plist->name = "Package Targets";
    plist->type = PLACE;
    plist->desc = fetch_desc(plist->name);
}

void
index_entry_free(IndexEntryPtr top)
{
    safe_free(top->name);
    safe_free(top->path);
    safe_free(top->prefix);
    safe_free(top->comment);
    safe_free(top->descrfile);
    safe_free(top->maintainer);
    free(top);
}

void
index_node_free(PkgNodePtr top, PkgNodePtr plist)
{
    PkgNodePtr tmp;

    tmp = plist;
    while (tmp) {
	PkgNodePtr tmp2 = tmp->next;
	    
	safe_free(tmp);
	tmp = tmp2;
    }

    tmp = top;
    while (tmp) {
	free(tmp->name);
	if (tmp->type == PACKAGE && tmp->data)
	    index_entry_free((IndexEntryPtr)tmp->data);
	if (tmp->kids)
	    index_node_free(tmp->kids, NULL);
	tmp = tmp->next;
    }
}
	
void
index_print(PkgNodePtr top, int level)
{
    int i;

    while (top) {
	for (i = 0; i < level; i++) putchar('\t');
	printf("name [%s]: %s\n", top->type == PLACE ? "place" : "package", top->name);
	for (i = 0; i < level; i++) putchar('\t');
	printf("desc: %s\n", top->desc);
	if (top->kids)
	    index_print(top->kids, level + 1);
	top = top->next;
    }
}

/* Swap one node for another */
static void
swap_nodes(PkgNodePtr a, PkgNodePtr b)
{
    PkgNode tmp;

    tmp = *a;
    *a = *b;
    a->next = tmp.next;
    tmp.next = b->next;
    *b = tmp;
}

/* Use a disgustingly simplistic bubble sort to put our lists in order */
void
index_sort(PkgNodePtr top)
{
    PkgNodePtr p, q;

    /* Sort everything at the top level */
    p = top->kids;
    while (p) {
	q = top->kids;
	while (q) {
	    if (q->next && strcmp(q->name, q->next->name) > 0)
		swap_nodes(q, q->next);
	    q = q->next;
	}
	p = p->next;
    }

    /* Now sub-sort everything n levels down */
    p = top->kids;
    while (p) {
	if (p->kids)
	    index_sort(p);
	p = p->next;
    }
}

/*
 * No, we don't free n because someone else is still pointing at it.
 * It's just clone linked from another location, which we're adjusting.
 */
void
index_delete(PkgNodePtr n)
{
    if (n->next)
	*n = *(n->next);
    else /* Kludgy end sentinal */
	n->name = NULL;
}

PkgNodePtr
index_search(PkgNodePtr top, char *str, PkgNodePtr *tp)
{
    PkgNodePtr p, sp;

    for (p = top->kids; p && p->name; p = p->next) {
	/* Subtract out the All category from searches */
	if (!strcmp(p->name, "All"))
	    continue;

	/* If tp == NULL, we're looking for an exact package match */
	if (!tp && !strcmp(p->name, str))
	    return p;

	/* If tp, we're looking for both a package and a pointer to the place it's in */
	if (tp && strstr(p->name, str)) {
	    *tp = top;
	    return p;
	}

	/* The usual recursion-out-of-laziness ploy */
	if (p->kids)
	    if ((sp = index_search(p, str, tp)) != NULL)
		return sp;
    }
    if (p && !p->name)
	p = NULL;
    return p;
}

/* Work function for seeing if name x is in result string y */
static Boolean
is_selected_in(char *name, char *result)
{
    while (result) {
	char *cp;

	cp = index(result, '\n');
	if (cp)
	   *cp++ = 0;
	/* Were no options actually selected? */
	if (!*result)
	    return FALSE;
	if (!strcmp(result, name))
	    return TRUE;
	result = cp;
    }
    return FALSE;
}

int
index_menu(PkgNodePtr top, PkgNodePtr plist, int *pos, int *scroll)
{
    int n, rval;
    int curr, max;
    PkgNodePtr sp, kp;
    char **nitems;
    char result[127];
    Boolean hasPackages;

    curr = max = 0;
    hasPackages = FALSE;
    nitems = NULL;

    n = 0;
    kp = top->kids;
    /* Figure out if this menu is full of "leaves" or "branches" */
    while (kp && kp->name) {
	++n;
	if (kp->type == PACKAGE) {
	    hasPackages = TRUE;
	    break;
	}
	kp = kp->next;
    }
    if (!n) {
	msgConfirm("The %s menu is empty.", top->name);
	return RET_DONE;
    }

    dialog_clear();
    while (1) {
	n = 0;
	kp = top->kids;
	if (!hasPackages && kp && kp->name && plist) {
	    nitems = item_add_pair(nitems, "UP", ".. RETURN TO PREVIOUS MENU ..", &curr, &max);
	    ++n;
	}
	while (kp && kp->name) {
	    nitems = item_add_pair(nitems, kp->name, kp->desc, &curr, &max);
	    if (hasPackages) {
		if (kp->type == PACKAGE)
		    nitems = item_add(nitems, index_search(plist, kp->name, NULL) ? "ON" : "OFF", &curr, &max);
		else
		    nitems = item_add(nitems, "OFF", &curr, &max);
	    }
	    ++n;
	    kp = kp->next;
	}
	nitems = item_add(nitems, NULL, &curr, &max);

	if (hasPackages)
	    rval = dialog_checklist(top->name, top->desc, -1, -1, n > MAX_MENU ? MAX_MENU : n, n,
				    (unsigned char **)nitems, result);
	else	/* It's a categories menu */
	    rval = dialog_menu(top->name, top->desc, -1, -1, n > MAX_MENU ? MAX_MENU : n, n,
			       (unsigned char **)nitems, result, pos, scroll);
	items_free(nitems, &curr, &max);
	if (!rval && plist && strcmp(result, "UP")) {
	    kp = top->kids;
	    while (kp) {
		if (kp->type == PACKAGE) {
		    sp = index_search(plist, kp->name, NULL);
		    if (is_selected_in(kp->name, result)) {
			if (!sp) {
			    PkgNodePtr n = (PkgNodePtr)malloc(sizeof(PkgNode));
			    
			    if (n) {
				*n = *kp;
				n->next = plist->kids;
				plist->kids = n;
				standout();
				mvprintw(23, 0, "Adding package %s to selection list\n", kp->name);
				standend();
				refresh();
			    }
			}
		    }
		    else if (sp) {
			standout();
			mvprintw(23, 0, "Deleting package %s from selection list\n", kp->name);
			standend();
			refresh();
			index_delete(sp);
		    }
		}
		else if (!strcmp(kp->name, result)) {	/* Not a package, must be a directory */
		    int p, s;
		    
		    p = s = 0;
		    index_menu(kp, plist, &p, &s);
		}
		kp = kp->next;
	    }
	}
	else if (rval == -1) {
	    static char *cp;
	    PkgNodePtr menu;

	    /* Search */
	    if ((cp = msgGetInput(cp, "Search by package name.  Please enter search string:")) != NULL) {
		PkgNodePtr p = index_search(top, cp, &menu);

		if (p) {
		    int pos, scroll;

		    /* These need to be set to point at the found item, actually.  Hmmm! */
		    pos = scroll = 0;
		    index_menu(menu, plist, &pos, &scroll);
		}
		else
		    msgConfirm("Search string: %s yielded no hits.", cp);
	    }
	}
	else {
	    dialog_clear();
	    return rval ? RET_FAIL : RET_SUCCESS;
	}
    }
}

int
index_extract(Device *dev, PkgNodePtr plist)
{
    PkgNodePtr tmp;
    int fd;


    dev->flags |= OPT_EXPLORATORY_GET;
    fd = dev->get(dev, "packages/INDEX", NULL);
    dev->flags &= ~OPT_EXPLORATORY_GET;
    if (fd < 0) {
	msgNotify("Can't find packages/INDEX file - looking for ports/INDEX.");
	fd = mediaDevice->get(mediaDevice, "ports/INDEX", NULL);
	if (fd < 0) {
	    msgConfirm("Unable to get ports/INDEX file from selected media.\n"
		       "Please verify media (or path to media) and try again.");
	    return RET_FAIL;
	}
    }

    tmp = plist->kids;
    while (tmp) {
	char path[511];
	char pen[FILENAME_MAX];
	char *where;
	int i;

	sprintf(path, "packages/%s", tmp->name);
	fd = dev->get(dev, path, NULL);
	if (fd >= 0) {
	    pid_t tpid;

	    if (isDebug())
		msgDebug("Got target %s from media type %d\n", path, dev->type);
	    pen[0] = '\0';
	    if ((where = make_playpen(pen, 0)) != NULL) {
		if (isDebug())
		    msgDebug("Working in temporary directory %s, will return to %s\n", pen, where);
		tpid = fork();
		if (!tpid) {
		    dup2(fd, 0);
		    i = vsystem("tar %s-xzf -", !strcmp(variable_get(CPIO_VERBOSITY_LEVEL), "low") ? "" : "-v ");
		    if (isDebug())
			msgDebug("tar command returns %d status\n", i);
		    exit(i);
		}
		else {
		    int pstat;

		    close(fd);
		    tpid = waitpid(tpid, &pstat, 0);
		    if (vsystem("(pwd; cat +CONTENTS) | pkg_add -S"))
			msgConfirm("An error occurred while trying to pkg_add %s.\n"
				   "Please check debugging screen for possible further details.");
		}
		if (chdir(where) == -1)
		    msgFatal("Unable to get back to where I was before, jojo!\n(%s)\n", where);
		vsystem("rm -rf %s", pen);
	    }
	    else
		msgConfirm("Unable to find a temporary location to unpack this stuff in.\n"
			   "You must simply not have enough space or you've configured your\n"
			   "system oddly.  Sorry!");
	    dev->close(dev, fd);
	    if (dev->type == DEVICE_TYPE_TAPE)
		unlink(path);
	}
	else {
	    if (optionIsSet(OPT_NO_CONFIRM))
		msgNotify("Unable to locate package %s..", path);
	    else
		msgConfirm("Unable to locate package %s..", path);
	}
	tmp = tmp->next;
    }
    return RET_SUCCESS;
}

static size_t
min_free(char *tmpdir)
{
    struct statfs buf;

    if (statfs(tmpdir, &buf) != 0) {
	perror("Error in statfs");
	return -1;
    }
    return buf.f_bavail * buf.f_bsize;
}

/* Find a good place to play. */
static char *
find_play_pen(char *pen, size_t sz)
{
    struct stat sb;

    if (pen[0] && stat(pen, &sb) != RET_FAIL && (min_free(pen) >= sz))
	return pen;
    else if (stat("/var/tmp", &sb) != RET_FAIL && min_free("/var/tmp") >= sz)
	strcpy(pen, "/var/tmp/instmp.XXXXXX");
    else if (stat("/tmp", &sb) != RET_FAIL && min_free("/tmp") >= sz)
	strcpy(pen, "/tmp/instmp.XXXXXX");
    else if ((stat("/usr/tmp", &sb) == RET_SUCCESS || mkdir("/usr/tmp", 01777) == RET_SUCCESS) &&
	     min_free("/usr/tmp") >= sz)
	strcpy(pen, "/usr/tmp/instmp.XXXXXX");
    else {
	msgConfirm("Can't find enough temporary space to extract the files, please try\n"
		   "This again after your system is up (you can run /stand/sysinstall\n"
		   "directly) and you've had a chance to point /var/tmp somewhere with\n"
		   "sufficient temporary space available.");
	return NULL;
    }
    return pen;
}

/*
 * Make a temporary directory to play in and chdir() to it, returning
 * pathname of previous working directory.
 */
static char *
make_playpen(char *pen, size_t sz)
{
    static char Previous[FILENAME_MAX];

    if (!find_play_pen(pen, sz))
	return NULL;

    if (!mktemp(pen)) {
	msgConfirm("Can't mktemp '%s'.", pen);
	return NULL;
    }
    if (mkdir(pen, 0755) == RET_FAIL) {
	msgConfirm("Can't mkdir '%s'.", pen);
	return NULL;
    }
    if (isDebug()) {
	if (sz)
	    msgDebug("Requested space: %d bytes, free space: %d bytes in %s\n", (int)sz, min_free(pen), pen);
    }
    if (min_free(pen) < sz) {
	rmdir(pen);
	msgConfirm("Not enough free space to create: `%s'\n"
		   "Please try this again after your system is up (you can run\n"
		   "/stand/sysinstall directly) and you've had a chance to point\n"
		   "/var/tmp somewhere with sufficient temporary space available.");
        return NULL;
    }
    if (!getcwd(Previous, FILENAME_MAX)) {
	msgConfirm("getcwd");
	return NULL;
    }
    if (chdir(pen) == RET_FAIL)
	msgConfirm("Can't chdir to '%s'.", pen);
    return Previous;
}

