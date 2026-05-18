/*
    qadgnuplotlib
    Copyright (c) 2026 David Duchet

    Licensed under the MIT License.
    See: licences/QADGNUPLOTLIB-MIT.txt

    Purpose:
        Provides low-level embedded gnuplot bootstrap and script execution support for the library.
*/

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#if defined(_MSC_VER)
#include <io.h>
#endif

#include "qadgnuplotlib.h"
#include "eval.h"
#include "gadgets.h"
#include "axis.h"
#include "multiplot.h"
#include "plot.h"
#include "command.h"
#include "datablock.h"
#include "scanner.h"
#include "util.h"
#include "misc.h"

static FILE* qadgnuplotlib_log_fp = NULL;
static int saved_stderr_fd = -1;

int qadgnuplotlib_set_log_file(const char* path)
{
	if (qadgnuplotlib_log_fp) {
		fclose(qadgnuplotlib_log_fp);
		qadgnuplotlib_log_fp = NULL;
	}

	/* NULL or empty string restores the default: real CRT stderr. */
	if (!path || !*path)
		return 0;

	qadgnuplotlib_log_fp = fopen(path, "w");
	if (!qadgnuplotlib_log_fp)
		return 1;

	/* Error messages are more useful if they are visible immediately. */
	setvbuf(qadgnuplotlib_log_fp, NULL, _IONBF, 0);
	return 0;
}

static void qadgnuplotlib_begin_stderr_redirect()
{
#if defined(_MSC_VER)

	if (!qadgnuplotlib_log_fp)
		return;

	fflush(stderr);
	
	saved_stderr_fd = _dup(_fileno(stderr));
	if (saved_stderr_fd < 0)
	{
		saved_stderr_fd = -1;
		return;
	}

	int fno = _fileno(qadgnuplotlib_log_fp);
	int dup = _dup(fno);

	if (_dup2(fno, _fileno(stderr)) != 0)
	{
		_close(saved_stderr_fd);
		saved_stderr_fd = -1;
		return;
	}

#endif
}

static void qadgnuplotlib_end_stderr_redirect()
{
#if defined(_MSC_VER)

	if (saved_stderr_fd < 0)
		return;

	fflush(stderr);
	if (_dup2(saved_stderr_fd, _fileno(stderr)))
	{
		printf("stderr destroyed, sorry.\n");
	}

	int fno = _fileno(qadgnuplotlib_log_fp);

	_close(saved_stderr_fd);
	
#endif
}


static char* sgets(char* out, int size, const char** input, int raw_input)
{
	if (!input || !*input || !out || size <= 0)
		return NULL;

	while (**input) {
		const char* start = *input;
		const char* end = start;
		const char* copy_end;
		int len;

		/* Find end of line: stop before '\n' */
		while (*end && *end != '\n')
			end++;

		/* Advance input pointer past '\n' if present */
		*input = (*end == '\n') ? end + 1 : end;

		/*
		 * Exclude Windows CR from CRLF.
		 * Also handles old-style lone CR at end of line.
		 */
		copy_end = end;
		if (copy_end > start && *(copy_end - 1) == '\r')
			copy_end--;

		if (raw_input) {
			len = (int)(copy_end - start);
			if (len >= size)
				len = size - 1;

			memcpy(out, start, len);
			out[len] = '\0';
			return out;
		}

		/* Normal mode: trim + skip empty lines */
		{
			const char* left = start;
			const char* right = copy_end;

			while (left < right && isspace((unsigned char)*left))
				left++;

			while (right > left && isspace((unsigned char)*(right - 1)))
				right--;

			if (left == right)
				continue;

			len = (int)(right - left);
			if (len >= size)
				len = size - 1;

			memcpy(out, left, len);
			out[len] = '\0';
			return out;
		}
	}

	return NULL;
}

static char* argname[] = { "ARG0","ARG1","ARG2","ARG3","ARG4","ARG5","ARG6","ARG7","ARG8","ARG9" };

static void
prepare_call(int calltype, udvt_entry* functionblock)
{
	struct udvt_entry* udv;
	struct value* ARGV;
	int argindex;
	int argv_size;

	/* call_args[] will hold arguments as strings.
	 * argval[] will be a private copy of numeric arguments as udvs.
	 * Later we will fill ARGV[] from one or the other.
	 */
	struct value argval[9];
	for (argindex = 0; argindex < 9; argindex++)
		argval[argindex].type = NOTDEFINED;

	if (calltype == 2 || calltype == 7) {
		call_argc = 0;
		while (!END_OF_COMMAND && call_argc < 9) {
			call_args[call_argc] = try_to_get_string();
			if (!call_args[call_argc]) {
				int save_token = c_token;

				/* This catches call "file" STRINGVAR (expression) */
				if (type_udv(c_token) == STRING) {
					call_args[call_argc] = gp_strdup(add_udv(c_token)->udv_value.v.string_val);
					c_token++;

					/* Evaluate a parenthesized expression or a bare numeric user variable
					 * and store the result in a string
					 */
				}
				else if (equals(c_token, "(")
					|| (type_udv(c_token) == INTGR || type_udv(c_token) == CMPLX)) {
					char val_as_string[32];
					struct value a;
					const_express(&a);
					argval[call_argc] = a;
					switch (a.type) {
					case CMPLX:
						sprintf(val_as_string, "%g", a.v.cmplx_val.real);
						call_args[call_argc] = gp_strdup(val_as_string);
						break;
					default:
						int_error(save_token, "Unrecognized argument type");
						break;
					case INTGR:
						sprintf(val_as_string, PLD, a.v.int_val);
						call_args[call_argc] = gp_strdup(val_as_string);
						break;
					}

					/* Old (pre version 5) style wrapping of bare tokens as strings
					 * is still used for storing numerical constants ARGn but not ARGV[n]
					 */
				}
				else {
					double temp;
					char* endptr;
					m_capture(&call_args[call_argc], c_token, c_token);
					c_token++;
					temp = strtod(call_args[call_argc], &endptr);
					if (endptr != call_args[call_argc] && *endptr == '\0')
						Gcomplex(&argval[call_argc], temp, 0.0);
				}
			}
			call_argc++;
		}
		lf_head->c_token = c_token;
		if (!END_OF_COMMAND)
			int_error(++c_token, "too many arguments for 'call <file>'");

	}
	else if (calltype == 5) {
		/* lf_push() moved our call arguments from call_args[] to lf->call_args[] */
		/* call_argc was determined at program entry */
		for (argindex = 0; argindex < 10; argindex++) {
			call_args[argindex] = lf_head->call_args[argindex];
			lf_head->call_args[argindex] = NULL;	/* just to be safe */
		}

#ifdef USE_FUNCTIONBLOCKS
	}
	else if (calltype == 8) {
		/* $functionblock(arg1, ...) */
		cache_at(&lf_head->shadow_at, &lf_head->shadow_at_size);
		memcpy(argval, eval_parameters, sizeof(argval));
		call_argc = 0;
		while ((call_argc < 9) && (argval[call_argc].type != NOTDEFINED)) {
			/* Execute the equivalent of local paramN = ARGV[N] */
			if (functionblock->udv_value.v.functionblock.parnames) {
				char* name = functionblock->udv_value.v.functionblock.parnames[call_argc];
				if (name) {
					struct udvt_entry* udv = add_udv_local(0, name, lf_head->depth);
					udv->udv_value = eval_parameters[call_argc];
					if (udv->udv_value.type == STRING)
						udv->udv_value.v.string_val = strdup(udv->udv_value.v.string_val);
					if (udv->udv_value.type == ARRAY) {
						/* local arrays are freed only by lf_exit_scope() */
						make_array_permanent(&(udv->udv_value));
						udv->udv_value.v.value_array[0].type = LOCAL_ARRAY;
					}
					lf_head->local_variables = TRUE;
				}
			}
			call_argc++;
		}
		if ((call_argc < 9)
			&& (functionblock->udv_value.v.functionblock.parnames != NULL)
			&& (functionblock->udv_value.v.functionblock.parnames[call_argc] != NULL))
			int_warn(c_token - 1, "Not enough parameters for %s", lf_head->name);
		if (evaluate_inside_functionblock == 0) {
			evaluate_inside_functionblock = lf_head->depth + 1;
			FPRINTF((stderr, "setting flag evaluate_inside_functionblock to %d\n",
				lf_head->depth + 1));
		}
#endif

	}
	else {
		/* "load" command has no arguments */
		call_argc = 0;
	}

	/* Old-style "call" arguments were referenced as $0 ... $9 and $#.
	 * New-style has ARG0 = script-name, ARG1 ... ARG9 and ARGC
	 * Version 5.3 adds ARGV[n]
	 */
	udv = add_udv_by_name("ARGC");
	Ginteger(&(udv->udv_value), call_argc);

	udv = add_udv_by_name("ARGV");
	argv_size = GPMIN(call_argc, 9);
	init_array(udv, argv_size);
	ARGV = udv->udv_value.v.value_array;

#ifdef USE_FUNCTIONBLOCKS
	if (calltype == 8) {
		for (argindex = 1; argindex <= argv_size; argindex++)
			ARGV[argindex] = argval[argindex - 1];
		return;
	}
#endif

	udv = add_udv_by_name("ARG0");
	gpfree_string(&(udv->udv_value));
	Gstring(&(udv->udv_value), gp_strdup(lf_head->name));

	for (argindex = 1; argindex <= 9; argindex++) {
		char* argstring = call_args[argindex - 1];

		udv = add_udv_by_name(argname[argindex]);
		gpfree_string(&(udv->udv_value));
		Gstring(&(udv->udv_value), argstring ? gp_strdup(argstring) : gp_strdup(""));

		if (argindex > argv_size)
			continue;
		if (argval[argindex - 1].type == NOTDEFINED)
			Gstring(&ARGV[argindex], gp_strdup(udv->udv_value.v.string_val));
		else
			ARGV[argindex] = argval[argindex - 1];
	}
}

int qadgnuplotlib_do_line_protected(int* did_longjmp);
int qadgnuplotlib_init(void);

static int qadgnuplotlib_initialized = 0;

/*
 * calltype indicates whether load_file() is called from
 * (1) the "load" command, no arguments substitution is done
 * (2) the "call" command, arguments are substituted for $0, $1, etc.
 * (3) on program entry to load initialization files (acts like "load")
 * (4) to execute script files given on the command line (acts like "load")
 * (5) to execute a single script file given with -c (acts like "call")
 * (6) "load $datablock"
 * (7) "call $datablock"
 * (8) execute commands in function block (called from f_eval)
 */
static int qadgnuplotlib_execute_script2(const char* script, char * name, int calltype)
{
	if (script == NULL) {
		return -1;
	}

	if (!qadgnuplotlib_initialized) {
		int err = qadgnuplotlib_init();
		if (err != 0)
			return err;

		qadgnuplotlib_initialized = 1;
	}

	const char* sp = script;

	int start, left;
	int more;
	int stop = FALSE;
	udvt_entry* gpval_lineno = NULL;
	char** datablock_input_line = NULL;
	udvt_entry* functionblock = NULL;
	TBOOLEAN playback_state_on_entry = multiplot_playback;

	/* Support for "load $datablock" */
	if (calltype == 6 || calltype == 7) {
		datablock_input_line = get_datablock(name)->data;
		/* Prevent attempt to recursively load $GPVAL_LAST_MULTIPLOT */
		if (!strcmp(name, "$GPVAL_LAST_MULTIPLOT")) {
			if (in_multiplot)
				return 0;
			multiplot_playback = TRUE;
		}
	}

#ifdef USE_FUNCTIONBLOCKS
	/* Support for function blocks */
	if (calltype == 8) {
		functionblock = (udvt_entry*)(name);
		datablock_input_line = functionblock->udv_value.v.functionblock.blockdata->data;
		name = strdup(functionblock->udv_name);
	}
#endif

	/* Provide a user-visible copy of the current line number in the input file */
	gpval_lineno = add_udv_by_name("GPVAL_LINENO");
	Ginteger(&gpval_lineno->udv_value, 0);

	lf_push(stdin, name, NULL); /* save state for errors and recursion, DD: stdin is a good option as it will not be closed */

	/* things to do after lf_push */
	inline_num = 0;
	/* go into non-interactive mode during load */
	/* will be undone below, or in reset_load_stack_after_error */
	interactive = FALSE;

	/* We actually will read from a file */
	prepare_call(calltype, functionblock);

	while (!stop) {	/* read all lines in file */
		left = gp_input_line_len;
		start = 0;
		more = TRUE;

		/* read one logical line */
		while (more) {
			if (sp && sgets(&(gp_input_line[start]), left, &sp, 1) == (char*)NULL) {
				/* EOF in input file */
				stop = TRUE;
				gp_input_line[start] = '\0';
				more = FALSE;
			}
			else if (!sp && datablock_input_line && (*datablock_input_line == NULL)) {
				/* End of input datablock */
				stop = TRUE;
				gp_input_line[start] = '\0';
				more = FALSE;
			}
			else {
				/* Either we successfully read a line from input file fp
				 * or we are about to copy a line from a datablock.
				 * Either way we have to process line-ending '\' as a
				 * continuation request.
				 */
				int len = strlen(gp_input_line);
				if (sp) {
					left = gp_input_line_len - len;
					if (left <= MAX_LINE_LEN) {
						extend_input_line();
						left = gp_input_line_len - len;
					}
				}
				else if (datablock_input_line) {
					int chunk_len = strlen(*datablock_input_line);
					while (left <= chunk_len) {
						extend_input_line();
						left = gp_input_line_len - (start + chunk_len);
					}
					safe_strncpy(&(gp_input_line[start]), *datablock_input_line, left);
					len = strlen(gp_input_line);
					/* strip trailing whitespace */
					while (len > 0 && isblank(gp_input_line[len - 1]))
						gp_input_line[--len] = '\0';
					left = gp_input_line_len - len;
					datablock_input_line++;
				}

				inline_num++;
				gpval_lineno->udv_value.v.int_val = inline_num;	/* User visible copy */

				/* nothing added to command line */
				if (len == 0)
					continue;

				/* point to last character in gp_input_line */
				--len;
				if (gp_input_line[len] == '\n') {	/* remove any newline */
					gp_input_line[len--] = '\0';
					if (len <= 0)
						continue;
					if (gp_input_line[len] == '\r') {	/* remove any carriage return */
						gp_input_line[len--] = '\0';
					}
					/* Remove trailing whitespace leading up to the newline */
					while (len > 0 && isblank(gp_input_line[len]))
						gp_input_line[len--] = '\0';
				}
				start = len + 1;
				left = gp_input_line_len - start;

				if (gp_input_line[len] == '\\') {
					/* line continuation */
					start = len;
					left = gp_input_line_len - start;
				}

				else {
					/* EAM May 2011 - handle multi-line bracketed clauses {...}.
					 * Introduces a requirement for scanner.c and scanner.h
					 * This code is redundant with part of do_line(),
					 * but do_line() assumes continuation lines come from stdin.
					 */

					 /* macros in a clause are problematic, as they are */
					 /* only expanded once even if the clause is replayed */
					string_expand_macros();

					/* Strip off trailing comment and count curly braces */
					num_tokens = scanner(&gp_input_line, &gp_input_line_len);
					if (gp_input_line[token[num_tokens].start_index] == '#') {
						gp_input_line[token[num_tokens].start_index] = NUL;
						start = token[num_tokens].start_index;
						left = gp_input_line_len - start;
					}
					/* Read additional lines if necessary to complete a
					 * bracketed clause {...}
					 */
					if (curly_brace_count < 0)
						int_error(NO_CARET, "Unexpected }");
					if (curly_brace_count > 0) {
						if ((strlen(gp_input_line) + 4) > gp_input_line_len)
							extend_input_line();
						strcat(gp_input_line, ";\n");
						start = strlen(gp_input_line);
						left = gp_input_line_len - start;
						continue;
					}
					more = FALSE;
				}
			}
		}   /* end while (more) */

		/* If we hit a 'break' or 'continue' statement in the lines just processed */
		if (iteration_early_exit())
			continue;

		/* process line */
		if (strlen(gp_input_line) > 0) {
			int did_longjmp = 0;

			screen_ok = FALSE;  /* make sure command line is echoed on error */
			if (qadgnuplotlib_do_line_protected(&did_longjmp))
				stop = TRUE;

			if (did_longjmp) {
				/* Preserve gnuplot's load-stack cleanup path, but return an error to the host. */
				lf_pop();
				multiplot_playback = playback_state_on_entry;
				return 1;
			}
		}
	}

	/* pop state */
	lf_pop();		/* also closes file fp. DD: but not stdin :-) */
	multiplot_playback = playback_state_on_entry;

	return 0;
}


static char* duplicate_string(const char* src)
{
	size_t n;
	char* dst;

	if (!src)
		return NULL;

	n = strlen(src) + 1;
	dst = (char*)malloc(n);
	if (!dst)
		return NULL;

	memcpy(dst, src, n);
	return dst;
}


int qadgnuplotlib_execute_script(const char* script)
{
	int rc;
	int saved_stderr_fd = -1;

	/* Ownership of dummy_name is transferred to lf_push()/lf_pop(). */
	char* dummy_name;

	if (!script)
		return -1;

	dummy_name = duplicate_string("{inline script}");

	if (!dummy_name)
		return 1;

	qadgnuplotlib_begin_stderr_redirect();
	
	rc = qadgnuplotlib_execute_script2(script, dummy_name, 1);

	qadgnuplotlib_end_stderr_redirect();
	return rc;
}
