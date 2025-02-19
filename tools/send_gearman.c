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

/* include header */
#include "send_gearman.h"
#include "utils.h"
#include "gearman_utils.h"
#include "openssl/evp.h"

#include <worker_dummy_functions.c>

mod_gm_opt_t *mod_gm_opt;
gearman_client_st *client;
gearman_client_st *client_dup;
gearman_client_st *current_client;
gearman_client_st *current_client_dup;
int results_sent = 0;
char hostname[GM_SMALLBUFSIZE];

/* work starts here */
int main (int argc, char **argv) {
    int rc;
    EVP_CIPHER_CTX * ctx = NULL;

    /*
     * allocate options structure
     * and parse command line
     */
    if(parse_arguments(argc, argv) != GM_OK) {
        print_usage();
        exit( STATE_UNKNOWN );
    }

    /* set logging */
    mod_gm_opt->logmode = GM_LOG_MODE_TOOLS;

    /* init crypto functions */
    if(mod_gm_opt->encryption == GM_ENABLED) {
        ctx = mod_gm_crypt_init(mod_gm_opt->crypt_key);
        gm_log(GM_LOG_DEBUG, "encryption enabled\n");
    } else {
        mod_gm_opt->transportmode = GM_ENCODE_ONLY;
        gm_log(GM_LOG_DEBUG, "encryption is disabled\n");
    }

    /* create client */
    client = create_client(mod_gm_opt->server_list);
    if(client == NULL) {
        printf("nagios-send-gearman UNKNOWN: cannot start client\n");
        exit(STATE_UNKNOWN);
    }
    current_client = client;

    /* create duplicate client */
    if ( mod_gm_opt->dupserver_num > 0 ) {
        client_dup = create_client(mod_gm_opt->dupserver_list);
        if(client_dup == NULL ) {
            printf("nagios-send-gearman UNKNOWN: cannot start client for duplicate server\n");
            exit( STATE_UNKNOWN );
        }
    }
    current_client_dup = client_dup;

    /* send result message */
    signal(SIGALRM, alarm_sighandler);
    rc = send_result(ctx);

    gm_free_client(&client);
    if( mod_gm_opt->dupserver_num )
        gm_free_client(&client_dup);
    mod_gm_free_opt(mod_gm_opt);
    mod_gm_crypt_deinit(ctx);

    exit( rc );
}


/* parse command line arguments */
int parse_arguments(int argc, char **argv) {
    int i;
    int verify;
    int errors = 0;
    mod_gm_opt = gm_malloc(sizeof(mod_gm_opt_t));
    set_default_options(mod_gm_opt);

    /* special default: encryption enabled if key present */
    mod_gm_opt->encryption = GM_AUTO;

    for(i=1;i<argc;i++) {
        char * arg   = gm_strdup( argv[i] );
        char * arg_c = arg;
        if ( !strcmp( arg, "version" ) || !strcmp( arg, "--version" )  || !strcmp( arg, "-V" ) ) {
            print_version();
        }
        if ( !strcmp( arg, "help" ) || !strcmp( arg, "--help" )  || !strcmp( arg, "-h" ) ) {
            print_usage();
        }
        if(parse_args_line(mod_gm_opt, arg, 0) != GM_OK) {
            errors++;
            free(arg_c);
            break;
        }
        free(arg_c);
    }

    /* verify options */
    verify = verify_options(mod_gm_opt);

    /* read keyfile */
    if(mod_gm_opt->keyfile != NULL && read_keyfile(mod_gm_opt) != GM_OK) {
        errors++;
    }

    if(errors > 0 || verify != GM_OK) {
        return(GM_ERROR);
    }

    return(GM_OK);
}


/* verify our option */
int verify_options(mod_gm_opt_t *opt) {

    /* did we get any server? */
    if(opt->server_num == 0) {
        printf("please specify at least one server\n" );
        return(GM_ERROR);
    }

    if(opt->encryption == GM_AUTO) {
        opt->encryption = GM_DISABLED;
        if(opt->crypt_key != NULL || opt->keyfile != NULL) {
            opt->encryption = GM_ENABLED;
        }
    }

    /* encryption without key? */
    if(opt->encryption == GM_ENABLED) {
        if(opt->crypt_key == NULL && opt->keyfile == NULL) {
            printf("no encryption key provided, please use --key=... or keyfile=... or disable encryption\n");
            return(GM_ERROR);
        }
    }

    if ( mod_gm_opt->result_queue == NULL )
        mod_gm_opt->result_queue = GM_DEFAULT_RESULT_QUEUE;

    mod_gm_opt->logmode = GM_LOG_MODE_STDOUT;

    return(GM_OK);
}


/* print usage */
void print_usage() {
    printf("usage:\n");
    printf("\n");
    printf("nagios-send-gearman [ --debug=<lvl>                ]\n");
    printf("                    [ --help|-h                    ]\n");
    printf("\n");
    printf("                    [ --config=<configfile>        ]\n");
    printf("\n");
    printf("                    [ --server=<server>            ]\n");
    printf("\n");
    printf("                    [ --timeout|-t=<timeout>       ]\n");
    printf("                    [ --delimiter|-d=<delimiter>   ]\n");
    printf("\n");
    printf("                    [ --encryption=<yes|no>        ]\n");
    printf("                    [ --key=<string>               ]\n");
    printf("                    [ --keyfile=<file>             ]\n");
    printf("\n");
    printf("                    [ --host=<hostname>            ]\n");
    printf("                    [ --service=<servicename>      ]\n");
    printf("                    [ --result_queue=<queue>       ]\n");
    printf("                    [ --message|-m=<pluginoutput>  ]\n");
    printf("                    [ --returncode|-r=<returncode> ]\n");
    printf("\n");
    printf("for sending active checks:\n");
    printf("                    [ --active                     ]\n");
    printf("                    [ --starttime=<unixtime>       ]\n");
    printf("                    [ --finishtime=<unixtime>      ]\n");
    printf("                    [ --latency=<seconds>          ]\n");
    printf("\n");
    printf("plugin output is read from stdin unless --message is used.\n");
    printf("Use this mode when plugin has multiple lines of plugin output.\n");
    printf("\n");
    printf("Note: When using a delimiter (-d) you may also submit one result\n");
    printf("      for each line.\n");
    printf("      Service Checks:\n");
    printf("      <host_name>[tab]<svc_description>[tab]<return_code>[tab]<plugin_output>[newline]\n");
    printf("\n");
    printf("      Host Checks:\n");
    printf("      <host_name>[tab]<return_code>[tab]<plugin_output>[newline]\n");
    printf("\n");
    printf("see README for a detailed explaination of all options.\n");
    printf("\n");

    mod_gm_free_opt(mod_gm_opt);
    exit( STATE_UNKNOWN );
}


/* send message to job server */
int send_result(EVP_CIPHER_CTX * ctx) {
    char *ptr1, *ptr2, *ptr3, *ptr4;
    char buffer[GM_BUFFERSIZE];

    gm_log( GM_LOG_TRACE, "send_result()\n" );

    if(mod_gm_opt->result_queue == NULL) {
        printf( "got no result queue, please use --result_queue=...\n" );
        return( STATE_UNKNOWN );
    }

    /* multiple results */
    if(mod_gm_opt->host == NULL) {
        while(fgets(buffer,sizeof(buffer)-1,stdin)) {
            if(feof(stdin))
                break;

            /* disable alarm */
            alarm(0);

            /* read host_name */
            ptr1=strtok(buffer,mod_gm_opt->delimiter);
            if(ptr1==NULL)
                continue;

            /* get the service description or return code */
            ptr2=strtok(NULL,mod_gm_opt->delimiter);
            if(ptr2==NULL)
                continue;

            /* get the return code or plugin output */
            ptr3=strtok(NULL,mod_gm_opt->delimiter);
            if(ptr3==NULL)
                continue;

            /* get the plugin output - if NULL, this is a host check result */
            ptr4=strtok(NULL,"\n");

            free(mod_gm_opt->host);
            if(mod_gm_opt->service != NULL) {
                free(mod_gm_opt->service);
                mod_gm_opt->service = NULL;
            }
            free(mod_gm_opt->message);

            /* host result */
            if(ptr4 == NULL) {
                mod_gm_opt->host        = gm_strdup(ptr1);
                mod_gm_opt->return_code = atoi(ptr2);
                mod_gm_opt->message     = gm_strdup(ptr3);
            } else {
                /* service result */
                mod_gm_opt->host        = gm_strdup(ptr1);
                mod_gm_opt->service     = gm_strdup(ptr2);
                mod_gm_opt->return_code = atoi(ptr3);
                mod_gm_opt->message     = gm_strdup(ptr4);
            }
            if(submit_result(ctx) == STATE_OK) {
                results_sent++;
            } else {
                printf("failed to send result!\n");
                return(STATE_UNKNOWN);
            }
        }
        printf("%d data packet(s) sent to host successfully.\n",results_sent);
        return(STATE_OK);
    }
    /* multi line plugin output */
    else if(mod_gm_opt->message == NULL) {
        /* get all lines from stdin */
        mod_gm_opt->message = gm_malloc(GM_BUFFERSIZE);
        mod_gm_opt->message[0]='\x0';
        alarm(mod_gm_opt->timeout);
        read_filepointer(&mod_gm_opt->message, stdin);
        alarm(0);
    }
    return(submit_result(ctx));
}

/* submit result */
int submit_result(EVP_CIPHER_CTX * ctx) {
    char * buf;
    char * temp_buffer;
    char * result;
    struct timeval now;
    struct timeval starttime;
    struct timeval finishtime;
    int resultsize;

    gettimeofday(&now, NULL);
    if(mod_gm_opt->has_starttime == FALSE) {
        starttime = now;
    } else {
        starttime = mod_gm_opt->starttime;
    }

    if(mod_gm_opt->has_finishtime == FALSE) {
        finishtime = now;
    } else {
        finishtime = mod_gm_opt->finishtime;
    }

    if(mod_gm_opt->has_latency == FALSE) {
        mod_gm_opt->latency.tv_sec  = 0;
        mod_gm_opt->latency.tv_usec = 0;
    }

    /* escape newline */
    buf = gm_escape_newlines(mod_gm_opt->message, GM_DISABLED);
    free(mod_gm_opt->message);
    mod_gm_opt->message = gm_strdup(buf);
    free(buf);

    gm_log( GM_LOG_TRACE, "queue: %s\n", mod_gm_opt->result_queue );
    resultsize = sizeof(char) * strlen(mod_gm_opt->message) + GM_BUFFERSIZE;
    result = gm_malloc(resultsize);
    snprintf( result, resultsize-1, "type=%s\nhost_name=%s\nstart_time=%Lf\nfinish_time=%Lf\nlatency=%Lf\nreturn_code=%i\nsource=nagios-send-gearman\n",
              mod_gm_opt->active == GM_ENABLED ? "active" : "passive",
              mod_gm_opt->host,
              timeval2double(&starttime),
              timeval2double(&finishtime),
              timeval2double(&mod_gm_opt->latency),
              mod_gm_opt->return_code
            );

    temp_buffer = gm_malloc(resultsize);
    if(mod_gm_opt->service != NULL) {
        temp_buffer[0]='\x0';
        strcat(temp_buffer, "service_description=");
        strcat(temp_buffer, mod_gm_opt->service);
        strcat(temp_buffer, "\n");
        strcat(result, temp_buffer);
    }

    if(mod_gm_opt->message != NULL) {
        temp_buffer[0]='\x0';
        strcat(temp_buffer, "output=");
        strcat(temp_buffer, mod_gm_opt->message);
        strcat(temp_buffer, "\n");
        strcat(result, temp_buffer);
    }
    strcat(result, "\n");

    gm_log( GM_LOG_TRACE, "data:\n%s\n", result);

    if(add_job_to_queue( &client,
                         mod_gm_opt->server_list,
                         mod_gm_opt->result_queue,
                         NULL,
                         result,
                         GM_JOB_PRIO_NORMAL,
                         GM_DEFAULT_JOB_RETRIES,
                         mod_gm_opt->transportmode,
                         ctx,
                         0,
                         1
                        ) == GM_OK) {
        gm_log( GM_LOG_TRACE, "send_result_back() finished successfully\n" );

        if( mod_gm_opt->dupserver_num ) {
            if(add_job_to_queue(&client_dup,
                                 mod_gm_opt->dupserver_list,
                                 mod_gm_opt->result_queue,
                                 NULL,
                                 result,
                                 GM_JOB_PRIO_NORMAL,
                                 GM_DEFAULT_JOB_RETRIES,
                                 mod_gm_opt->transportmode,
                                 ctx,
                                 0,
                                 1
                            ) == GM_OK) {
                gm_log( GM_LOG_TRACE, "send_result_back() finished successfully for duplicate server.\n" );
            }
            else {
                gm_log( GM_LOG_TRACE, "send_result_back() finished unsuccessfully for duplicate server\n" );
            }
        }
    }
    else {
        gm_log( GM_LOG_TRACE, "send_result_back() finished unsuccessfully\n" );
        free(result);
        free(temp_buffer);
        return( STATE_UNKNOWN );
    }
    free(result);
    free(temp_buffer);
    return( STATE_OK );
}


/* called when check runs into timeout */
void alarm_sighandler(int sig) {
    gm_log( GM_LOG_TRACE, "alarm_sighandler(%i)\n", sig );

    printf("got no input after %i seconds! Either send plugin output to stdin or use --message=...\n", mod_gm_opt->timeout);

    exit( STATE_UNKNOWN );
}


/* print version */
void print_version() {
    printf("nagios-send-gearman: version %s running on libgearman %s\n", GM_VERSION, gearman_version());
    printf("\n");
    exit( STATE_UNKNOWN );
}


/* core log wrapper */
void write_core_log(char *data) {
    printf("core logger is not available for tools: %s", data);
    return;
}
