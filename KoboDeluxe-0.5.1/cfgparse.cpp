/*(LGPL)
-------------------------------------------------------------------
	cfgparse.cpp - Generic Config File and Argument Parser
-------------------------------------------------------------------
 * Copyright (C) 2001, 2007 David Olofson
 *
 * This code is released under the terms of the GNU LGPL.
 */

#include "config.h"
#include "logger.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "cfgparse.h"

#define	MAX_CONFIG_LINES	1024


/*----------------------------------------------------------
	cfg_key_t
----------------------------------------------------------*/

cfg_key_t::cfg_key_t()
{
	next = NULL;
	name = "<unnamed>";
	save = 1;
	typecode = CFG_NONE;
	redefined = 0;
	description = NULL;
}


cfg_key_t::~cfg_key_t()
{
	free(description);
}


int cfg_key_t::read(int argc, char *argv[])
{
	return 0;
}


int cfg_key_t::test(const char *arg)
{
	if(arg[0] == '-' || arg[0] == '/')
		++arg;
	return !strcmp(name, arg);
}


/*----------------------------------------------------------
	cfg_comment_t
----------------------------------------------------------*/

void cfg_comment_t::write(FILE *f)
{
	fprintf(f, "#%s\n", name);
}


int cfg_comment_t::copy(cfg_key_t *from)
{
	if(typecode != from->typecode)
		return -1;
	return 0;
}


/*----------------------------------------------------------
	cfg_switch_t
----------------------------------------------------------*/

cfg_switch_t::cfg_switch_t(const char *_name, int &var, int def, int _save)
{
	name = _name;
	value = &var;
	default_value = def;
	save = _save;
	typecode = CFG_BOOL;
}


int cfg_switch_t::copy(cfg_key_t *from)
{
	if(typecode != from->typecode)
		return -1;
	*value = *((cfg_switch_t *)from)->value;
	return 0;
}


int cfg_switch_t::test(const char *arg)
{
	if(arg[0] == '-' || arg[0] == '/')
		++arg;
	if(strncmp("no", name, 2))
		if(!strncmp("no", arg, 2))
			arg += 2;
	return !strcmp(name, arg);
}


int cfg_switch_t::read(int argc, char *argv[])
{
	char *arg = argv[0];
	*value = 1;
	if(arg[0] == '-' || arg[0] == '/')
		++arg;
	if(!strncmp("no", arg, 2))
		*value = 0;
	if(!strncmp("no", name, 2))
		*value = !(*value);
	return 1;
}


void cfg_switch_t::write(FILE *f)
{
	fprintf(f, *value ? "%s\n" : "no%s\n", name);
}


void cfg_switch_t::set_default()
{
	*value = default_value;
}


int cfg_switch_t::is_your_var(void *var)
{
	return var == (void *)value;
}



/*----------------------------------------------------------
	cfg_key_int_t
----------------------------------------------------------*/

cfg_key_int_t::cfg_key_int_t(const char *_name, int &var, int def, int _save)
{
	name = _name;
	value = &var;
	default_value = def;
	save = _save;
	typecode = CFG_INT;
}


int cfg_key_int_t::copy(cfg_key_t *from)
{
	if(typecode != from->typecode)
		return -1;
	*value = *((cfg_key_int_t *)from)->value;
	return 0;
}


int cfg_key_int_t::read(int argc, char *argv[])
{
	if(argc < 2)
		return -1;
	*value = atoi(argv[1]);
	return 2;
}


void cfg_key_int_t::write(FILE *f)
{
	fprintf(f, "%s %d\n", name, *value);
}


void cfg_key_int_t::set_default()
{
	*value = default_value;
}


int cfg_key_int_t::is_your_var(void *var)
{
	return var == (void *)value;
}



/*----------------------------------------------------------
	cfg_key_float_t
----------------------------------------------------------*/

cfg_key_float_t::cfg_key_float_t(const char *_name, float &var, float def, int _save)
{
	name = _name;
	value = &var;
	default_value = def;
	save = _save;
	typecode = CFG_FLOAT;
}


int cfg_key_float_t::copy(cfg_key_t *from)
{
	if(typecode != from->typecode)
		return -1;
	*value = *((cfg_key_float_t *)from)->value;
	return 0;
}




int cfg_key_float_t::read(int argc, char *argv[])
{
	if(argc < 2)
		return -1;
	*value = atof(argv[1]);
	return 2;
}


void cfg_key_float_t::write(FILE *f)
{
	fprintf(f, "%s %f\n", name, *value);
}


void cfg_key_float_t::set_default()
{
	*value = default_value;
}


int cfg_key_float_t::is_your_var(void *var)
{
	return var == (void *)value;
}



/*----------------------------------------------------------
	cfg_key_string_t
----------------------------------------------------------*/

cfg_key_string_t::cfg_key_string_t(const char *_name, cfg_string_t &var,
		const cfg_string_t def, int _save)
{
	name = _name;
	value = &var;
	strncpy(default_value, def, CFG_STRING_LENGTH);
	save = _save;
	typecode = CFG_STRING;
}


int cfg_key_string_t::copy(cfg_key_t *from)
{
	if(typecode != from->typecode)
		return -1;
	memcpy(*value, *((cfg_key_string_t *)from)->value, CFG_STRING_LENGTH);
	return 0;
}


int cfg_key_string_t::read(int argc, char *argv[])
{
	if(argc < 2)
		return -1;
	strncpy(*value, argv[1], CFG_STRING_LENGTH);
	return 2;
}


void cfg_key_string_t::write(FILE *f)
{
	fprintf(f, "%s \"%s\"\n", name, *value);
}


void cfg_key_string_t::set_default()
{
	strncpy(*value, default_value, CFG_STRING_LENGTH);
}


int cfg_key_string_t::is_your_var(void *var)
{
	return var == (void *)value;
}



/*----------------------------------------------------------
	config_parser_t
----------------------------------------------------------*/

config_parser_t::config_parser_t()
{
	keys = last_key = NULL;
	changed = 0;
	initialized = 0;
}


void config_parser_t::initialize()
{
	if(initialized)
		return;

	init();
	set_defaults();
	build_table();
	initialized = 1;
}

void config_parser_t::build_table()
{
	cfg_key_t *k = keys;
	nkeys = 0;
	while(k)
	{
		++nkeys;
		k = k->next;
	}
	table = new cfg_key_t*[nkeys];

	k = keys;
	int i = 0;
	while(k)
	{
		table[i++] = k;
		k = k->next;
	}
}

config_parser_t::~config_parser_t()
{
	cfg_key_t *k = keys;
	while(k)
	{
		keys = k->next;
		delete k;
		k = keys;
	}
	delete[]table;
}

config_parser_t &config_parser_t::operator = (config_parser_t &from)
{
	initialize();
	from.initialize();
	cfg_key_t *k = keys;
	while(k)
	{
		cfg_key_t *fk = from.keys;
		while(fk)
		{
			if(!strcmp(fk->name, k->name))
			{
				if(k->copy(fk) < 0)
					log_printf(ELOG, "config_parser_t: Tried"
							" to copy from incompatible"
							" source!\n");
				break;
			}
			fk = fk->next;
		}
		k = k->next;
	}
	return *this;
}

void config_parser_t::add(cfg_key_t *_key)
{
	if(last_key)
	{
		last_key->next = _key;
		last_key = _key;
	}
	else
		keys = last_key = _key;
}


void config_parser_t::comment(const char *text)
{
	add(new cfg_comment_t(text));
}


void config_parser_t::yesno(const char *name, int &var, int def, int save)
{
	add(new cfg_switch_t(name, var, def, save));
}


void config_parser_t::command(const char *name, int &var)
{
	add(new cfg_switch_t(name, var, 0, 0));
}


void config_parser_t::key(const char *name, int &var, int def, int save)
{
	add(new cfg_key_int_t(name, var, def, save));
}


void config_parser_t::key(const char *name, float &var, float def, int save)
{
	add(new cfg_key_float_t(name, var, def, save));
}


void config_parser_t::key(const char *name, cfg_string_t &var,
		const cfg_string_t def, int save)
{
	add(new cfg_key_string_t(name, var, def, save));
}


void config_parser_t::desc(const char *text)
{
	if(!last_key)
	{
		log_printf(ELOG, "config_parser_t::description() called"
				" with no key registered!\n");
		return;
	}
	last_key->description = strdup(text);
}


void config_parser_t::set_defaults()
{
	cfg_key_t *k = keys;
	while(k)
	{
		k->set_default();
		k->redefined = 0;
		k = k->next;
	}
}


/*
 * "Uuuh, a leetle icky finite state machine parser... :-D"
 * (Actually a very nice and simple way of coding this kind
 * of stuff, if you don't want to drag in Bison or something.)
 */
enum scanstate_t
{
	SS_BLANK,
	SS_QUOTE,
	SS_WORD,
	SS_COMMENT
};

int config_parser_t::read_config(char ***cv, FILE *f)
{
	*cv = (char **)calloc(MAX_CONFIG_LINES, sizeof(char *));
	if(!*cv)
		return -1;

	if(!f)
	{
		free((*cv));
		(*cv) = NULL;
		return -2;
	}

	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	(*cv)[0] = (char *)malloc(len+1);
	if(!(*cv)[0])
	{
		rewind(f);
		free(*cv);
		*cv = NULL;
		return -3;
	}
	rewind(f);
	if(fread((*cv)[0], len, 1, f) < 0)
	{
		free(*cv[0]);
		free(*cv);
		*cv = NULL;
		return -4;
	}
	(*cv)[0][len] = 0;

	char *v = (*cv)[0];
	scanstate_t state = SS_BLANK;
	int arg = 1;
	int start_arg = 0;
	while(*v)
	{
		switch(state)
		{
		  case SS_BLANK:
			if(*v == '"')
			{
				start_arg = 1;
				state = SS_QUOTE;
			}
			else if(*v == '#')
				state = SS_COMMENT;
			else if(*v > ' ')
			{
				start_arg = 1;
				state = SS_WORD;
			}
			break;
		  case SS_QUOTE:
			if(*v == '"')
			{
				*v = 0;
				state = SS_BLANK;
			}
			break;
		  case SS_WORD:
			if(*v <= ' ')
			{
				*v = 0;
				state = SS_BLANK;
			}
			break;
		  case SS_COMMENT:
			if((*v == 10) || (*v == 13))
				state = SS_BLANK;
			break;
		}
		if(start_arg)
		{
			if(arg < MAX_CONFIG_LINES)
			{
				if(state == SS_QUOTE)
					(*cv)[arg++] = v+1;
				else
					(*cv)[arg++] = v;
			}
			start_arg = 0;
		}
		++v;
	}
	rewind(f);
	return arg;
}


int config_parser_t::parse(int argc, char *argv[])
{
	initialize();

	int i = 0;
	while(i < argc)
	{
		if(argv[i][0] == '#')
		{
			++i;
			continue;
		}

		cfg_key_t *k = keys;
		int grabbed = 0;
		while(k)
		{
			if(k->test(argv[i]))
			{
				grabbed = k->read(argc - i, argv + i);
				if(grabbed > 0)
				{
					k->redefined = 1;
					break;
				}
				else if(grabbed < 0)
				{
					log_printf(ELOG, "ERROR: Too few parameters to"
							" argument '%s'!\n",
							k->name);
					return -2;
				}
			}
			k = k->next;
		}
		if(!grabbed)
		{
			log_printf(ELOG, "ERROR: Unknown argument"
					" '%s'!\n", argv[i]);
			return -3;
		}
		i += grabbed;
	}
	postload();
	return 0;	/* Ok. */
}


int config_parser_t::read(FILE *f)
{
	int res = -10;
	int cc;
	char **cv;
	cc = read_config(&cv, f);
	if(cc > 0)
	{
		res = parse(cc, cv);
		if(cv)
			free(cv[0]);
		free(cv);
	}
	return res;
}


int config_parser_t::write(FILE *f)
{
	initialize();

	presave();
	cfg_key_t *k = keys;
	while(k)
	{
		if(k->save)
			k->write(f);
		k = k->next;
	}
	return ferror(f);
}


int config_parser_t::_redefined(void *var)
{
	cfg_key_t *k = keys;
	while(k)
	{
		if(k->is_your_var(var))
			return k->redefined;
		k = k->next;
	}
	return 0;
}


void config_parser_t::_accept(void *var)
{
	cfg_key_t *k = keys;
	while(k)
	{
		if(k->is_your_var(var))
		{
			k->redefined = 0;
			break;
		}
		k = k->next;
	}
}



/*----------------------------------------------------------
	Generic Symbol Table Style API
----------------------------------------------------------*/

int config_parser_t::find(const char *name)
{
	if(!table)
		initialize();
	if(!table)
		return -1;
	for(int i = 0; i < nkeys; ++i)
		if(table[i]->test(name))
			return i;
	return -1;
}


int config_parser_t::find_next(int symbol)
{
	if(!table)
		initialize();
	if(!table)
		return -1;

	// Note that this simple code handles the
	// "find first" -1 case for free. :-)
	++symbol;
	if(symbol < 0)
		return -1;
	else if(symbol >= nkeys)
		return -1;

	return symbol;
}


int config_parser_t::check_symbol(int symbol)
{
	if(!table)
		return -1;
	if(symbol < 0)
		return -1;
	else if(symbol >= nkeys)
		return -1;
	return 0;
}


cfg_types_t config_parser_t::type(int symbol)
{
	if(check_symbol(symbol) < 0)
		return CFG_NONE;
	return table[symbol]->typecode;
}


int config_parser_t::do_save(int symbol)
{
	if(check_symbol(symbol) < 0)
		return CFG_NONE;
	return table[symbol]->save;
}


const char *config_parser_t::description(int symbol)
{
	if(check_symbol(symbol) < 0)
		return "<key does not exist!>";
	if(table[symbol]->description)
		return table[symbol]->description;
	else
		return "<key has no description>";
}


const char *config_parser_t::name(int symbol)
{
	if(check_symbol(symbol) < 0)
		return "<key does not exist!>";
	if(table[symbol]->name)
		return table[symbol]->name;
	else
		return "<key has no name>";
}


void config_parser_t::set(int symbol, int value)
{
	if(check_symbol(symbol) < 0)
		return;
	switch(table[symbol]->typecode)
	{
	  case CFG_NONE:
		return;
	  case CFG_BOOL:
		{
			cfg_switch_t *k = (cfg_switch_t *)table[symbol];
			*(k->value) = value ? 1 : 0;
		}
		break;
	  case CFG_INT:
		{
			cfg_key_int_t *k = (cfg_key_int_t *)table[symbol];
			*(k->value) = value;
		}
		break;
	  case CFG_FLOAT:
		{
			cfg_key_float_t *k = (cfg_key_float_t *)table[symbol];
			*(k->value) = (float)value;
		}
		break;
	  case CFG_STRING:
		{
			cfg_key_string_t *k = (cfg_key_string_t *)table[symbol];
			snprintf(*(k->value), CFG_STRING_LENGTH-1, "%d", value);
		}
		break;
	}
	table[symbol]->redefined = 1;
}


void config_parser_t::set(int symbol, float value)
{
	if(check_symbol(symbol) < 0)
		return;
	switch(table[symbol]->typecode)
	{
	  case CFG_NONE:
		return;
	  case CFG_BOOL:
		{
			cfg_switch_t *k = (cfg_switch_t *)table[symbol];
			*(k->value) = value ? 1 : 0;
		}
		break;
	  case CFG_INT:
		{
			cfg_key_int_t *k = (cfg_key_int_t *)table[symbol];
			*(k->value) = (int)value;
		}
		break;
	  case CFG_FLOAT:
		{
			cfg_key_float_t *k = (cfg_key_float_t *)table[symbol];
			*(k->value) = value;
		}
		break;
	  case CFG_STRING:
		{
			cfg_key_string_t *k = (cfg_key_string_t *)table[symbol];
			snprintf(*(k->value), CFG_STRING_LENGTH-1, "%f", value);
		}
		break;
	}
	table[symbol]->redefined = 1;
}


void config_parser_t::set(int symbol, const char *text)
{
	if(check_symbol(symbol) < 0)
		return;
	switch(table[symbol]->typecode)
	{
	  case CFG_NONE:
		return;
	  case CFG_BOOL:
		{
			/*
			 * Unfortunately, only *real* operating systems have
			 * strcasecmp(), so we have to mess around a little.
			 */
			char buf[16];
			strncpy(buf, text, 15);
			for(int i = 0; i < 15; ++i)
				buf[i] = tolower(buf[i]);
			cfg_switch_t *k = (cfg_switch_t *)table[symbol];
			if( strncmp(buf, "yes", 3) || strncmp(buf, "true", 4) ||
					strcmp(buf, "on") || strncmp(buf, "enable", 6) ||
					strncmp(buf, "activ", 5) ||
					strcmp(buf, "one") || strcmp(buf, "1") )
				*(k->value) = 1;
			else
				*(k->value) = 0;
		}
		break;
	  case CFG_INT:
		{
			cfg_key_int_t *k = (cfg_key_int_t *)table[symbol];
			*(k->value) = atoi(text);
		}
		break;
	  case CFG_FLOAT:
		{
			cfg_key_float_t *k = (cfg_key_float_t *)table[symbol];
			*(k->value) = atof(text);
		}
		break;
	  case CFG_STRING:
		{
			cfg_key_string_t *k = (cfg_key_string_t *)table[symbol];
			strncpy(*(k->value), text, CFG_STRING_LENGTH-1);
			*(k->value)[CFG_STRING_LENGTH-1] = 0;
		}
		break;
	}
	table[symbol]->redefined = 1;
}


int config_parser_t::get_i(int symbol)
{
	if(check_symbol(symbol) < 0)
		return 0;
	switch(table[symbol]->typecode)
	{
	  case CFG_NONE:
		return 0;
	  case CFG_BOOL:
		{
			cfg_switch_t *k = (cfg_switch_t *)table[symbol];
			return *(k->value) ? 1 : 0;
		}
	  case CFG_INT:
		{
			cfg_key_int_t *k = (cfg_key_int_t *)table[symbol];
			return *(k->value);
		}
	  case CFG_FLOAT:
		{
			cfg_key_float_t *k = (cfg_key_float_t *)table[symbol];
			return (int)*(k->value);
		}
	  case CFG_STRING:
		{
			cfg_key_string_t *k = (cfg_key_string_t *)table[symbol];
			return atoi(*(k->value));
		}
	}
	return 0;
}


float config_parser_t::get_f(int symbol)
{
	if(check_symbol(symbol) < 0)
		return 0.0;
	switch(table[symbol]->typecode)
	{
	  case CFG_NONE:
		return 0.0;
	  case CFG_BOOL:
		{
			cfg_switch_t *k = (cfg_switch_t *)table[symbol];
			return *(k->value) ? 1.0 : 0.0;
		}
	  case CFG_INT:
		{
			cfg_key_int_t *k = (cfg_key_int_t *)table[symbol];
			return (float)*(k->value);
		}
	  case CFG_FLOAT:
		{
			cfg_key_float_t *k = (cfg_key_float_t *)table[symbol];
			return *(k->value);
		}
	  case CFG_STRING:
		{
			cfg_key_string_t *k = (cfg_key_string_t *)table[symbol];
			return atof(*(k->value));
		}
	}
	return 0.0;
}


const char *config_parser_t::get_s(int symbol)
{
	if(check_symbol(symbol) < 0)
		return "";
	switch(table[symbol]->typecode)
	{
	  case CFG_NONE:
		return "";
	  case CFG_BOOL:
		{
			cfg_switch_t *k = (cfg_switch_t *)table[symbol];
			return *(k->value) ? "On" : "Off";
		}
	  case CFG_INT:
		{
			cfg_key_int_t *k = (cfg_key_int_t *)table[symbol];
			snprintf(retbuf, sizeof(retbuf)-1, "%d", *(k->value));
			return retbuf;
		}
	  case CFG_FLOAT:
		{
			cfg_key_float_t *k = (cfg_key_float_t *)table[symbol];
			snprintf(retbuf, sizeof(retbuf)-1, "%f", *(k->value));
			return retbuf;
		}
	  case CFG_STRING:
		{
			cfg_key_string_t *k = (cfg_key_string_t *)table[symbol];
			return *(k->value);
		}
		break;
	}
	return "";
}


int config_parser_t::get_default_i(int symbol)
{
	if(check_symbol(symbol) < 0)
		return 0;
	switch(table[symbol]->typecode)
	{
	  case CFG_NONE:
		return 0;
	  case CFG_BOOL:
		{
			cfg_switch_t *k = (cfg_switch_t *)table[symbol];
			return k->default_value ? 1 : 0;
		}
	  case CFG_INT:
		{
			cfg_key_int_t *k = (cfg_key_int_t *)table[symbol];
			return k->default_value;
		}
	  case CFG_FLOAT:
		{
			cfg_key_float_t *k = (cfg_key_float_t *)table[symbol];
			return (int)k->default_value;
		}
	  case CFG_STRING:
		{
			cfg_key_string_t *k = (cfg_key_string_t *)table[symbol];
			return atoi(k->default_value);
		}
	}
	return 0;
}


float config_parser_t::get_default_f(int symbol)
{
	if(check_symbol(symbol) < 0)
		return 0.0;
	switch(table[symbol]->typecode)
	{
	  case CFG_NONE:
		return 0.0;
	  case CFG_BOOL:
		{
			cfg_switch_t *k = (cfg_switch_t *)table[symbol];
			return k->default_value ? 1.0 : 0.0;
		}
	  case CFG_INT:
		{
			cfg_key_int_t *k = (cfg_key_int_t *)table[symbol];
			return (float)k->default_value;
		}
	  case CFG_FLOAT:
		{
			cfg_key_float_t *k = (cfg_key_float_t *)table[symbol];
			return k->default_value;
		}
	  case CFG_STRING:
		{
			cfg_key_string_t *k = (cfg_key_string_t *)table[symbol];
			return atof(k->default_value);
		}
	}
	return 0.0;
}


const char *config_parser_t::get_default_s(int symbol)
{
	if(check_symbol(symbol) < 0)
		return "";
	switch(table[symbol]->typecode)
	{
	  case CFG_NONE:
		return "";
	  case CFG_BOOL:
		{
			cfg_switch_t *k = (cfg_switch_t *)table[symbol];
			return k->default_value ? "On" : "Off";
		}
	  case CFG_INT:
		{
			cfg_key_int_t *k = (cfg_key_int_t *)table[symbol];
			snprintf(retbuf, sizeof(retbuf)-1, "%d", k->default_value);
			return retbuf;
		}
	  case CFG_FLOAT:
		{
			cfg_key_float_t *k = (cfg_key_float_t *)table[symbol];
			snprintf(retbuf, sizeof(retbuf)-1, "%f", k->default_value);
			return retbuf;
		}
	  case CFG_STRING:
		{
			cfg_key_string_t *k = (cfg_key_string_t *)table[symbol];
			return k->default_value;
		}
		break;
	}
	return "";
}

