/******************************************************************************
 *
 * nagios-mod-gearman - distribute checks with gearman
 *
 * Copyright (c) 2010 Sven Nierlein - sven.nierlein@consol.de
 * Copyright (c) 2024 Nagios Development Team - devteam@nagios.com
 *
 * This file is part of nagios-mod-gearman.
 *
 *  nagios-mod-gearman is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  nagios-mod-gearman is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with nagios-mod-gearman.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

/** @file
 *  @brief utility components for all parts of nagios-mod-gearman
 *
 *  @{
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stddef.h>
#include <openssl/evp.h>

#include "common.h"

#define GM_PERFDATA_QUEUE    "perfdata"  /**< default performance data queue */

/**
 * escpae newlines
 *
 * @param[in] rawbuf  - text to escape
 * @param[in] trimmed - trim string before escaping
 *
 * @return a text with all newlines escaped
 */
char *gm_escape_newlines(char *rawbuf, int trimmed);

/**
 * real_exit_code
 *
 * converts a exit from wait() into real number
 *
 * @param[in] code - exit code
 *
 * @return real exit code
 */
int real_exit_code(int code);

/**
 * mod_gm_crypt_init
 *
 * wrapper to initialze nagios-mod-gearman crypt module
 *
 * @param[in] key - password for encryption
 *
 * @return openssl ctx
 */
EVP_CIPHER_CTX * mod_gm_crypt_init(const char * key);

/**
 * mod_gm_crypt_deinit
 *
 * wrapper to deinitialze nagios-mod-gearman crypt module
 *
 * @param[in] ctx - openssl context
 *
 * @return nothing
 */
void mod_gm_crypt_deinit(EVP_CIPHER_CTX *);

/**
 * mod_gm_encrypt
 *
 * wrapper to encrypt text
 *
 * @param[in] ctx - openssl context
 * @param[out] ciphertext - pointer to target encrypted text
 * @param[in] plaintext - source text to encrypt
 * @param[in] mode - encryption mode (base64 or aes64 with base64)
 *
 * @return base64 encoded text or aes encrypted text based on mode
 */
int mod_gm_encrypt(EVP_CIPHER_CTX * ctx, char ** ciphertext, const char * plaintext, int mode);

/**
 * mod_gm_decrypt
 *
 * @param[in] ctx - openssl context
 * @param[out] plaintext - pointer to target plaintext text
 * @param[in] ciphertext - source text to decrypt
 * @param[in] ciphertext_size - size of ciphertext
 * @param[in] mode - do only base64 decoding or decryption too
 *
 * @return 1 on success
 */
int mod_gm_decrypt(EVP_CIPHER_CTX * ctx, char ** plaintext, const char * ciphertext, size_t ciphertext_size, int mode);

/**
 * file_exists
 *
 * @param[in] fileName - path to file
 *
 * @return true if file exists
 */
int file_exists (char * fileName);

/**
 * ltrim
 *
 * trim whitespace to the left
 *
 * @param[in] s - text to trim
 *
 * @return trimmed text
 */
char *ltrim(char *s);

/**
 * rtrim
 *
 * trim whitespace to the right
 *
 * @param[in] s - text to trim
 *
 * @return trimmed text
 */
char *rtrim(char *s);

/**
 * trim
 *
 * trim whitespace from left and right
 *
 * @param[in] s - text to trim
 *
 * @return trimmed text
 */
char *trim(char *s);

/**
 * set_default_options
 *
 * fill in default options
 *
 * @param[in] opt - option structure
 *
 * @return true on success
 */
int set_default_options(mod_gm_opt_t *opt);

/**
 * lc
 *
 * lowercase given text
 *
 * @param[in] str - text to lowercase
 *
 * @return lowercased text
 */
char *lc(char * str);

/**
 * parse_args_line
 *
 * parse the command line arguments in our options structure
 *
 * @param[in] opt - options structure
 * @param[in] arg - arguments
 * @param[in] recursion_level - counter for the recursion level
 *
 * @return true on success
 */
int parse_args_line(mod_gm_opt_t *opt, char * arg, int recursion_level);

/**
 * parse_yes_or_no
 *
 * parse a string for yes/no, on/off
 *
 * @param[in] value - string to parse
 * @param[in] dfl - default value if none matches
 *
 * @return parsed value
 */
int parse_yes_or_no(char*value, int dfl);

/**
 * read_config_file
 *
 * read config options from a file
 *
 * @param[in] opt - options structure
 * @param[in] filename - file to parse
 * @param[in] recursion_level - counter for the recursion level
 *
 * @return true on success
 */
int read_config_file(mod_gm_opt_t *opt, char*filename, int recursion_level);

/**
 * dumpconfig
 *
 * dumps config with logger
 *
 * @param[in] opt - options structure
 * @param[in] mode - display mode
 *
 * @return nothing
 */
void dumpconfig(mod_gm_opt_t *opt, int mode);

/**
 * mod_gm_free_opt
 *
 * free options structure
 *
 * @param[in] opt - options structure
 *
 * @return nothing
 */
void mod_gm_free_opt(mod_gm_opt_t *opt);

/**
 * read_keyfile
 *
 * read keyfile into options structure
 *
 * @param[in] opt - options structure
 *
 * @return true on success
 */
int read_keyfile(mod_gm_opt_t *opt);

/**
 * string2timeval
 *
 * parse string into timeval
 *
 * @param[in] value - string to parse
 * @param[out] t - pointer to timeval structure
 *
 * @return nothing
 */
void string2timeval(char * value, struct timeval * t);

/**
 * double2timeval
 *
 * parse double into timeval
 *
 * @param[in] value - double value
 * @param[out] t - pointer to timeval structure
 *
 * @return nothing
 */
void double2timeval(long double value, struct timeval * t);

/**
 * timeval2double
 *
 * convert timeval into double
 *
 * @param[in] t - timeval structure
 *
 * @return double value for this timeval structure
 */
long double timeval2double(struct timeval * t);

/**
 *
 * set_default_job
 *
 * fill defaults into job structure
 *
 * @param[in] job - job structure to be filled
 * @param[in] mod_gm_opt - options structure
 *
 * @return true on success
 */
int set_default_job(gm_job_t *job, mod_gm_opt_t *mod_gm_opt);

/**
 *
 * free_job
 *
 * free job structure
 *
 * @param[in] job - job structure to be freed
 *
 * @return true on success
 */
int free_job(gm_job_t *job);


/**
 * pid_alive
 *
 * check if a pid is alive
 *
 * @param[in] pid - pid to check
 *
 * @return true if pid is alive
 */
int pid_alive(int pid);

/**
 * escapestring
 *
 * escape quotes and newlines
 *
 * @param[in] rawbuf - text to escape
 *
 * @return the escaped string
 */
char *escapestring(char *rawbuf);

/**
 * escaped
 *
 * checks wheter a char has to be escaped or not
 *
 * @param[in] ch - char to check
 *
 * @return true if char has to be escaped
 */
int escaped(int ch);

/**
 * escape
 *
 * return escaped variant of char
 *
 * @param[out] out - escaped char
 * @param[in] ch - char to escape
 *
 * @return the escaped string
 */
void escape(char *out, int ch);

/**
 * replace_str
 *
 * return string with old replaced by new
 *
 * @param[in] str - input string
 * @param[in] old - char to replace
 * @param[in] new - char to replace with
 *
 * @return the replaced string
 */
char *replace_str(const char *str, const char *old, const char *new);

/**
 * nebtype2str
 *
 * get human readable name for neb type
 *
 * @param[in] i - integer to translate
 *
 * @return the human readable string
 */
char * nebtype2str(int i);


/**
 * nebcallback2str
 *
 * get human readable name for nebcallback type int
 *
 * @param[in] i - integer to translate
 *
 * @return the human readable string
 */
char * nebcallback2str(int i);

/**
 * eventtype2str
 *
 * get human readable name for eventtype type int
 *
 * @param[in] i - integer to translate
 *
 * @return the human readable string
 */
char * eventtype2str(int i);

/**
 * gm_log
 *
 * general logger
 *
 * @param[in] lvl  - debug level for this message
 * @param[in] text - text to log
 *
 * @return nothing
 */
void gm_log( int lvl, const char *text, ... );

/**
 * write_core_log
 *
 * write log line with core logger
 *
 * @param[in] data - log message
 *
 * @return nothing
 */
void write_core_log(char *data);

/**
 * check_param_server
 *
 * check if server is already in the list
 *
 * @param[in] new_server - server to check
 * @param[in] server_list - list of servers to check for duplicates
 * @param[in] server_num - number of server in this list
 *
 * @returns the new server name or NULL
 */
int check_param_server(gm_server_t * new_server, gm_server_t * server_list[GM_LISTSIZE], int server_num);

/**
 * send_result_back
 *
 * send back result
 *
 * @param[in] exec_job - the exec job with all results
 *
 * @return nothing
 */
void send_result_back(gm_job_t * exec_job, EVP_CIPHER_CTX * ctx);

/**
 * add_server
 *
 * adds parsed server to list
 *
 * @param[in] server_num - insert server at that point
 * @param[in] server_list - add server to this list
 * @param[in] servername - parse and add this server
 *
 * @return nothing
 */
void add_server(int * server_num, gm_server_t * server_list[GM_LISTSIZE], char * servername);

/**
 * starts_with
 *
 * returns true if string starts with another string
 *
 * @param[in] pre - start string
 * @param[in] str - string to search in
 *
 * @return nothing
 */
int starts_with(const char *pre, const char *str);

/**
 * read_filepointer
 *
 * reads filepointer into a malloced char and returns size.
 * increases size if required.
 *
 * @param[in] buffer - buffer to read into
 * @param[in] fp - filepointer to read from
 *
 * @return number of bytes read
 */
int read_filepointer(char **, FILE*);

/**
 * read_pipe
 *
 * reads pipe into a malloced char and returns size.
 * increases size if required.
 *
 * @param[in] buffer - buffer to read into
 * @param[in] pipe - pipe to read from
 *
 * @return number of bytes read
 */
int read_pipe(char **, int);

/**
 * make_uniq
 *
 * create unique identifier from given format
 *
 * @param[in] uniq  - reference to target
 * @param[in] format - printf format string
 *
 * @return nothing
 */
void make_uniq(char *uniq, const char *format, ... );

/**
 * elapsed_time
 *
 * return elapsed time between two points in time
 *
 * @param[in] t1 - start time
 * @param[in] t2 - end time
 *
 * @return elapsed time
 */
double elapsed_time(struct timeval t1, struct timeval t2);

void printf_hex(const char* src, int len, char*prefix);

/**
 * @}
 */
