/*(LGPL)
------------------------------------------------------------
	filemap.cpp - Simple Portable File Path Mapper
------------------------------------------------------------
 * Copyright (C) David Olofson, 2001, 2003, 2007
 * This code is released under the terms of the GNU LGPL.
 */

#include "config.h"
#include "kobolog.h"

#include "filemap.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>

#ifdef HAVE_STAT
#include <sys/stat.h>
#include <unistd.h>
#endif


fm_object_t::~fm_object_t()
{
	free(path);
}


/*
 * Delete the 'count' first bytes of a string.
 * Does *not* realloc(), so it's safe to use
 * on static or local (stack) buffers.
 *
 * Returns new length of string.
 */
static int strdel(char *str, int count)
{
	int len = strlen(str);
	memmove(str, str+count, len-count+1);
	return len-count;
}


/*
 * Inserts string 'ins' into 'str', starting at 'pos'.
 * Does *not* realloc(), so it's safe to use
 * on static or local (stack) buffers.
 *
 * Relies on FM_BUFFER_SIZE to prevent buffer overflow!
 * (In fact, it stops at (str + FM_BUFFER_SIZE - 1) to
 * ensure that the result is NULL terminated.)
 *
 * Returns new length of string.
 */
static int strins(char *str, const char *ins, int pos)
{
	char *cpy = strdup(str+pos);
	if(!cpy)
		return strlen(str);	//Silly but harmless...

	snprintf(str+pos, FM_BUFFER_SIZE-pos-1, "%s%s", ins, cpy);
	str[FM_BUFFER_SIZE - 1] = 0;
	free(cpy);
	return strlen(str);
}


filemapper_t::filemapper_t()
{
	keys = NULL;
	app_path = strdup(".");
	next_buffer = 0;
	objects = NULL;
	current_obj = NULL;

	/*
	 * I'll put this here for now.
	 * Don't know if it's the perfect place...
	 */
	char *home = (char *)getenv("HOME");
	if(home)
		addpath("HOME", home);
}


filemapper_t::~filemapper_t()
{
	fm_key_t *k = keys;
	while(k)
	{
		fm_key_t *nk = k->next;
		free(k->path);
		delete k;
		k = nk;
	}
	while(objects)
	{
		fm_object_t *o = objects;
		objects = objects->next;
		delete o;
	}
	free(app_path);
}


char *filemapper_t::salloc()
{
	char *b = buffers[next_buffer++];
	if(next_buffer >= FM_BUFFERS)
		next_buffer = 0;
	return b;
}


void filemapper_t::no_double_slashes(char *buf)
{
	/*
	 * Clean slashes
	 */
	char *c = buf;
	while(c[0])
	{
		if(c[0] == '/')
		{
			if(0 == c[1])
				c[0] = 0;
			else
				while('/' == c[1])
					strdel(c + 1, 1);
		}
		++c;
	}
}


void filemapper_t::unix_slashes(char *buf)
{
	/*
	 * Convert "~/" into "HOME>>".
	 */
	if(strncmp(buf, "~/", 2) == 0)
	{
		strdel(buf, 2);
		strins(buf, "HOME" FM_DEREF_TOKEN, 0);
	}

#if defined(WIN32)
	int len = strlen(buf);
	for(int i = 0; i < len; ++i)
		if(buf[i] == '\\')
			buf[i] = '/';
#elif defined(MACOS)
	int start = 0;
	int len = strlen(buf);
	for(int i = start; i < len; ++i)
		if(buf[i] == ':')
			buf[i] = '/';
#endif
	no_double_slashes(buf);
}


void filemapper_t::sys_slashes(char *buf)
{
	no_double_slashes(buf);

#if defined(WIN32)
	int len;
	if(strncmp(buf, "./", 2) == 0)
		len = strdel(buf, 2);
	else
		len = strlen(buf);
	for(int i = 0; i < len; ++i)
		if(buf[i] == '/')
			buf[i] = '\\';
#elif defined(MACOS)
	/* 
	 * Untested.
	 */
	int len;
	if(strncmp(buf, "./", 2) == 0)
		len = strdel(buf, 2);
	else
		len = strlen(buf);
	/* Fix absolute/relative */
	if(buf[0] == '/')
		len = strdel(buf, 1);
	else
	{
		memmove(buf + 1, buf, len);
		++len;
		buf[0] = ':';
	}
	/* 
	 * '/' --> ':'
	 */
	for(int i = 0; i < len; ++i)
		if(buf[i] == '/')
			buf[i] = ':';
	/* 
	 * "..:" --> "::"
	 */
	for(int i = 0; i < len; ++i)
		if(strncmp(buf + i, "..:", 3) == 0)
		{
			strdel(buf + i, 1);
			buf[i] = ':';
			--len;
		}
#endif
}


void filemapper_t::exepath(const char *appname)
{
	free(app_path);
	app_path = strdup(sys2unix(appname));
	if(app_path)
	{
		char *c = strrchr(app_path, '/');
		if(c)
			c[0] = 0;
		else
		{
			free(app_path);
			app_path = strdup(".");
		}
	}
	else
		app_path = strdup(".");	
	addpath("EXE", app_path);
	log_printf(DLOG, "Application path: '%s'\n", app_path);
}


void filemapper_t::addpath(const char *key, const char *path, int first)
{
	fm_key_t *k, *insk;
	try
	{
		k = new fm_key_t;
		strncpy(k->key, key, 8);
		k->key[8] = 0;
		k->path = strdup(path);
		if(!k->path)
		{
			delete k;
			return;
		}
		if(first)
		{
			k->next = keys;
			keys = k;
		}
		else
		{
			k->next = NULL;
			if(keys)
			{
				insk = keys;
				while(insk->next)
					insk = insk->next;
				insk->next = k;
			}
			else
				keys = k;
		}
	}
	catch(...)
	{
		return;
	}
}


fm_key_t *filemapper_t::getkey(fm_key_t *key, const char *ref)
{
	int all = ref ? 0 : (ref[0] == '*');
	if(key)
	{
		if(!ref)
			ref = key->key;
		else if(all)
			return key->next;
		else
			key = key->next;
	}
	else
	{
		if(!ref || all)
			return keys;
		else
			key = keys;
	}

	while(key)
	{
		if(strcmp(key->key, ref) == 0)
			return key;
		key = key->next;
	}
	return NULL;
}


/*
 * Checks if an object exists, and if so, what kind it is.
 */
int filemapper_t::probe(const char *syspath)
{
#ifdef HAVE_STAT
	struct stat st;
//	log_printf(DLOG, "probe(\"%s\") (with stat())\n", syspath);
	if(::stat(syspath, &st) == 0)
	{
		if(S_ISDIR(st.st_mode))
			return FM_DIR;
		else
			return FM_FILE;
	}
#else
#warning This platform lacks stat(). File/directory probing may be unreliable!
	// This is wrong and weird, but *does* in fact
	// work on Linux... where it isn't needed! *heh*
	// (Opening a dir with "r" will succeed on Linux, BTW...)
	FILE *f = ::fopen(syspath, "r+");
//	log_printf(DLOG, "probe(\"%s\") (with fopen())\n", syspath);
	if(f)
	{
		::fclose(f);
		return FM_FILE;
	}
	else
		if(errno == EISDIR)
			return FM_DIR;
#endif
	return FM_ERROR;
}


/*
 * Create a file for writing, or if it exists, test if it
 * can be opened in write mode.
 *
 * Returns
 *	FM_FILE if the file exists and is writable,
 *	FM_FILE_CREATE if the file had to be created,
 *	FM_DIR if the path leads to a directory, or
 *	FM_ERROR if the object is not a writable file.
 */
int filemapper_t::test_file_create(const char *syspath)
{
	int exists = (probe(syspath) == FM_FILE);
	FILE *f = ::fopen(syspath, "a");
	if(f)
	{
		fclose(f);
		if(exists)
		{
//			log_printf(DLOG, "  File is�writable!\n");
			return FM_FILE;
		}
		else
		{
//			log_printf(DLOG, "  File created!\n");
			return FM_FILE_CREATE;
		}
	}
	else
	{
		if(errno == EISDIR)
		{
//			log_printf(DLOG, "  Is a directory.\n");
			return FM_DIR;
		}
//		else
//			log_printf(DLOG, "  Nope.\n");
	}
	return FM_ERROR;
}


int filemapper_t::test_file_dir_any(const char *syspath, int kind)
{
	int res = probe(syspath);
	switch (res)
	{
	  case FM_ERROR:
//		log_printf(DLOG, "  Nope.\n");
		break;
	  case FM_FILE:
		if((kind == FM_ANY) || (kind == FM_FILE))
		{
//			log_printf(DLOG, "  Found! (File)\n");
			return 1;
		}
		break;
	  case FM_DIR:
		if((kind == FM_ANY) || (kind == FM_DIR))
		{
//			log_printf(DLOG, "  Found! (Dir)\n");
			return 1;
		}
		break;
	  case FM_ANY:
		log_printf(ELOG, "INTERNAL ERROR: filemapper_t::probe()"
				" returned FM_ANY...\n");
		break;
	}
	if(res != FM_ERROR)
		switch (kind)
		{
		  case FM_FILE:
//			log_printf(DLOG, "  Not a file.\n");
			break;
		  case FM_DIR:
//			log_printf(DLOG, "  Not a directory.\n");
			break;
		  default:
			break;
		}
	return 0;
}


char *filemapper_t::sys2unix(const char *syspath)
{
	char *buffer = salloc();
	strncpy(buffer, syspath, FM_BUFFER_SIZE - 1);
	buffer[FM_BUFFER_SIZE - 1] = 0;
	unix_slashes(buffer);
	return buffer;
}


char *filemapper_t::unix2sys(const char *path)
{
	char *buffer = salloc();
	strncpy(buffer, path, FM_BUFFER_SIZE - 1);
	buffer[FM_BUFFER_SIZE - 1] = 0;
	sys_slashes(buffer);
	return buffer;
}


/*
 * Try the specified operation on a path.
 * The path must be in *system* format.
 */
int filemapper_t::try_get(const char *path, int kind)
{
	switch (kind)
	{
	  case FM_FILE:
	  case FM_DIR:
	  case FM_ANY:
		if(test_file_dir_any(path, kind))
			return 1;
		break;
	  case FM_FILE_CREATE:
		switch (test_file_create(path))
		{
		  case FM_FILE:
		  case FM_FILE_CREATE:
			return 1;
		  default:
			break;
		}
		break;
	  default:
		break;
	}
	return 0;
}


void filemapper_t::add_object(const char *path, int kind)
{
	try
	{
		fm_object_t *obj = new fm_object_t;
		obj->path = strdup(path);
		if(!obj->path)
		{
			delete obj;
			return;
		}
		obj->kind = kind;

		obj->next = NULL;
		if(objects)
		{
			fm_object_t *insobj = objects;
			while(insobj->next)
				insobj = insobj->next;
			insobj->next = obj;
		}
		else
			objects = obj;
	}
	catch(...)
	{
		return;
	}
}


/*
 * Parse the specified path, recurse through all possible
 * combinations, trying each terminal with try_get().
 *
 * When the 'build' argument is 0, the function returns
 * the first valid hit.
 *
 * 'ref' must be in unix format, but 'result' will be in
 * system format.
 *
 * NEW MODE:
 *	When the 'build' argument is non-zero, all
 *	possible paths are generated and tested. All
 *	valid objects are added to a linked list of
 *	fm_object_t.
 *
 *	Note that in this mode, 'result' is not used
 *	or set! You may pass NULL.
 *
TODO: Wildcards...
 *	
 */
int filemapper_t::recurse_get(char *result, const char *ref, int kind,
		int level, int build)
{
//	log_printf(DLOG, " [level %d]\n", level);
	char buffer[FM_BUFFER_SIZE];
	strncpy(buffer, ref, FM_BUFFER_SIZE);

	// Check for the ">>" dereferencing token.
	char *obj = (char *)strstr(ref, FM_DEREF_TOKEN);

	// If there isn't one, the object path will
	// be tried as is.
	if(!obj)
	{
		int res;
		snprintf(buffer, FM_BUFFER_SIZE, "%s", ref);
		buffer[FM_BUFFER_SIZE-1] = 0;
#if 0
		{
			for(int s=0; s<level; ++s)
				log_printf(DLOG, "  ");
		}
		log_printf(DLOG, "('%s') (no key!)\n", buffer);
		log_printf(DLOG, "Trying '%s'...", ref);
#endif
		sys_slashes(buffer);
		res = try_get(buffer, kind);
		if(build && res)
			add_object(buffer, kind);
		if(res && !build)
		{
			strncpy(result, buffer, FM_BUFFER_SIZE);
			return 1;
		}
		else
			return 0;
	}

	char kclass[9];
	int len = obj - ref;
	obj += 2;	// Skip the ">>"
	if(len > 8)
		len = 8;
	memcpy(kclass, ref, len);
	kclass[len] = 0;

	fm_key_t *k = NULL;
	int hits = 0;
	while( (k = getkey(k, kclass)) )
	{
		++hits;
		snprintf(buffer, FM_BUFFER_SIZE, "%s/%s", k->path, obj);
		buffer[FM_BUFFER_SIZE-1] = 0;
#if 0
		{
			for(int s=0; s<level; ++s)
				log_printf(DLOG, "  ");
		}
		log_printf(DLOG, "('%s')\n", buffer);
		{
			for(int s=0; s<level; ++s)
				log_printf(DLOG, "  ");
		}
#endif
		int res;
		char *dt = strstr(buffer, FM_DEREF_TOKEN);
		if(dt)
		{
//			log_printf(DLOG, "Reparsing '%s'", k->path);
			if(dt[2] == '/')	// No slash after ">>"!
				strdel(dt + 2, 1);
			res = recurse_get(buffer, buffer, kind, level + 1,
					build);
		}
		else
		{
//			log_printf(DLOG, "Trying '%s' -> '%s'...", k->key,
//					k->path);
			sys_slashes(buffer);
			res = try_get(buffer, kind);
			if(build && res)
				add_object(buffer, kind);
		}
		if(res && !build)
		{
			strncpy(result, buffer, FM_BUFFER_SIZE);
			return 1;
		}
	}
#if 0
	if(!hits)
	{
		for(int s=0; s<level; ++s)
			log_printf(DLOG, "  ");
		log_printf(DLOG, "No paths registered for '%s'!\n", kclass);
	}
#endif
	return 0;
}


const char *filemapper_t::get(const char *ref, int kind)
{
#if 0
	if(kind == FM_FILE_CREATE)
		log_printf(DLOG, "Trying to create '%s'...", ref);
	else if(kind == FM_DIR)
		log_printf(DLOG, "Looking for '%s'...", ref);
	else
		log_printf(DLOG, "Looking for '%s'...", ref);
#endif
	char *buffer = salloc();
	if(recurse_get(buffer, ref, kind, 1, 0))
		return buffer;
	else
		return NULL;
}


void filemapper_t::get_all(const char *ref, int kind)
{
	while(objects)
	{
		fm_object_t *obj = objects;
		objects = objects->next;
		delete obj;
	}

	if(kind == FM_FILE_CREATE)
	{
//		log_printf(ELOG, "filemapper_t: Tried to create files"
//				" in get_all()!\n");
		return;
	}
#if 0
	else if(kind == FM_DIR)
		log_printf(DLOG, "get_all('%s', DIR)...", ref);
	else
		log_printf(DLOG, "get_all('%s')...", ref);
#endif
	recurse_get(NULL, ref, kind, 1, 1);
	current_obj = objects;
	current_dir = NULL;
}


const char *filemapper_t::get_next()
{
	while(1)
	{
		if(!current_dir)
		{
			//
			// Get next object!
			//
			if(!current_obj)
				return NULL;	//All done.

			switch(probe(current_obj->path))
			{
			  case FM_FILE:
			  {
				const char *res = current_obj->path;
				current_obj = current_obj->next;
//				log_printf(DLOG, "get_next() found file '%s'.\n", res);
				return res;
			  }
			  case FM_DIR:
				current_dir = ::opendir(current_obj->path);
//				log_printf(DLOG, "get_next() found dir '%s'.\n",
//						current_obj->path);
				if(!current_dir)
				{
					//Couldn't opendir()! Skip object...
					current_obj = current_obj->next;
					continue;
				}
				break;
			  default:
				//Probing failed! Skip this object.
//				log_printf(DLOG, "get_next(): Object '%s' skipped!\n",
//						current_obj->path);
				current_obj = current_obj->next;
				continue;
			}
		}

		//
		// Get first/next dir entry!
		//
		struct dirent *d = readdir(current_dir);
		if(!d)
		{
			// Error or end-of-dir ==> try next object
			closedir(current_dir);
			current_dir = NULL;
			current_obj = current_obj->next;
//			log_printf(DLOG, "get_next(): end-of-dir\n");
			continue;
		}

		// Get dir entry
		char *path = salloc();
		if(!path)
		{
			log_printf(CELOG, "Out of memory in get_next()!\n", path);
			return NULL;
		}

		// Skip "parent dir" links!
#ifdef MACOS
		if(!strcmp(d->d_name, "::"))	//???
#else
		if(!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
#endif
			continue;

		// Check dir entry!
		snprintf(path, FM_BUFFER_SIZE,
#ifdef WIN32
				 "%s\\%s",
#elif defined(MACOS)
				 "%s:%s",
#else
				 "%s/%s",
#endif
				current_obj->path, d->d_name);
		switch(probe(path))
		{
		  case FM_FILE:
//			log_printf(DLOG, "get_next() found file '%s'.\n", path);
			return path;
		  case FM_DIR:
			//We don't do recursive, so skip subdirs...
//			log_printf(DLOG, "get_next() found dir '%s'.\n", path);
			continue;
		  default:
//			log_printf(DLOG, "get_next() found crap in dir.\n");
			continue;
		}
	}
}


FILE *filemapper_t::fopen(const char *ref, const char *mode)
{
	const char *path = NULL;
	int md = 0;

	if(strchr(mode, 'r'))
		md = 1;
	else if(strchr(mode, 'w'))
		md = 3;
	else if(strchr(mode, 'a'))
		md = 5;
	if(md)
		if(strchr(mode, '+'))
			md += 1;

	switch(md)
	{
	  case 0:	//Error!
		return NULL;
	  case 1:	//r
	  case 2:	//r+
		path = get(ref, FM_FILE);
		break;
	  case 3:	//w
	  case 4:	//w+  
	  case 5:	//a
	  case 6:	//a+
		path = get(ref, FM_FILE_CREATE);
		break;
	}
	if(path)
		return ::fopen(path, mode);
	else
		return NULL;
}


DIR *filemapper_t::opendir(const char *ref)
{
	const char *path = get(ref, FM_DIR);
	if(path)
		return ::opendir(path);
	else
		return NULL;
}


int filemapper_t::mkdir(const char *ref, int perm)
{
	log_printf(ELOG, "filemapper_t::mkdir() not yet implemented!\n");
	return -1;
#if 0
	const char *path = get(ref, FM_DIR_CREATE);
//	::mkdir(path, perm);
#endif
}


void filemapper_t::print(FILE *f, const char *ref)
{
	char key[9];
	key[8] = 0;
	char *obj = (char *)strstr(ref, FM_DEREF_TOKEN);
	if(!obj)
		strncpy(key, ref, 8);
	else
	{
		int len = obj - ref;
		strncpy(key, ref, 8);
		if(len > 8)
			len = 8;
		key[len] = 0;
	}
	int all = (key[0] == '*');
	fm_key_t *k = keys;
	while(k)
	{
		if(strcmp(k->key, key) == 0 || all)
			fprintf(f, "%s" FM_DEREF_TOKEN" --> \"%s\"\n", k->key, k->path);
		k = k->next;
	}
}
