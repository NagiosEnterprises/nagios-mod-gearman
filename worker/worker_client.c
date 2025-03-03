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
#include "worker.h"
#include "common.h"
#include "worker_client.h"
#include "utils.h"
#include "check_utils.h"
#include "gearman_utils.h"

gearman_worker_st *worker = NULL;
gearman_client_st *client = NULL;
gearman_client_st *client_dup;
gearman_client_st *current_client;
gearman_client_st *current_client_dup;
gearman_job_st *current_gearman_job;

pid_t current_pid;
gm_job_t * exec_job;

int jobs_done = 0;
int sleep_time_after_error = 1;
int worker_run_mode;
int shm_index = 0;
extern volatile sig_atomic_t shmid;

gm_job_t * current_job;

EVP_CIPHER_CTX * worker_ctx = NULL;

extern mod_gm_opt_t *mod_gm_opt;
extern char hostname[GM_SMALLBUFSIZE];

/* callback for task completed */
void worker_client(int worker_mode, int indx, int shid) {

    gm_log( GM_LOG_TRACE, "%s worker client started\n", (worker_mode == GM_WORKER_STATUS ? "status" : "job" ));

    /* set signal handlers for a clean exit */
    signal(SIGINT, clean_worker_exit);
    signal(SIGTERM,clean_worker_exit);

    worker_run_mode = worker_mode;
    shm_index       = indx;
    shmid           = shid;
    current_pid     = getpid();

    gethostname(hostname, GM_SMALLBUFSIZE-1);

    /* create worker */
    if(set_worker(&worker) != GM_OK) {
        gm_log( GM_LOG_ERROR, "cannot start worker\n" );
        clean_worker_exit(0);
        _exit( EXIT_FAILURE );
    }

    /* create client */
    client = create_client_blocking(mod_gm_opt->server_list);
    if(client == NULL) {
        gm_log( GM_LOG_ERROR, "cannot start client\n" );
        clean_worker_exit(0);
        _exit( EXIT_FAILURE );
    }
    current_client = client;

    /* create duplicate client */
    if( mod_gm_opt->dupserver_num ) {
        client_dup = create_client_blocking(mod_gm_opt->dupserver_list);
        if(client_dup == NULL) {
            gm_log( GM_LOG_ERROR, "cannot start client for duplicate server\n" );
            _exit( EXIT_FAILURE );
        }
        current_client_dup = client_dup;
    }

    worker_ctx = mod_gm_crypt_init(mod_gm_opt->crypt_key);

    worker_loop();

    return;
}


/* main loop of jobs */
void worker_loop() {

    while ( 1 ) {
        gearman_return_t ret;

        /* wait for a job, otherwise exit when hit the idle timeout */
        if(mod_gm_opt->idle_timeout > 0 && ( worker_run_mode == GM_WORKER_MULTI || worker_run_mode == GM_WORKER_STATUS )) {
            signal(SIGALRM, idle_sighandler);
            alarm(mod_gm_opt->idle_timeout);
        }

        ret = gearman_worker_work(worker);

        if (mod_gm_opt->max_jobs > 0 && jobs_done >= mod_gm_opt->max_jobs) {
            gm_log( GM_LOG_TRACE, "jobs done: %i -> exiting...\n", jobs_done );
            clean_worker_exit(0);
            _exit( EXIT_SUCCESS );
        }

        if ( ret != GEARMAN_SUCCESS ) {
            gm_log( GM_LOG_ERROR, "worker error: %s\n", gearman_worker_error(worker) );
            gm_free_worker(&worker);
            gm_free_client(&client);
            if(mod_gm_opt->dupserver_num)
                gm_free_client(&client_dup);

            /* sleep on error to avoid cpu intensive infinite loops */
            sleep(sleep_time_after_error);
            sleep_time_after_error += 3;
            if(sleep_time_after_error > 60)
                sleep_time_after_error = 60;

            /* create new connections */
            set_worker(&worker);
            client = create_client_blocking(mod_gm_opt->server_list);
            current_client = client;
            if( mod_gm_opt->dupserver_num ) {
                client_dup = create_client_blocking(mod_gm_opt->dupserver_list);
                current_client_dup = client_dup;
            }
        }
    }

    return;
}


/* get a job */
void *get_job( gearman_job_st *job, __attribute__((__unused__)) void *context, size_t *result_size, gearman_return_t *ret_ptr ) {
    sigset_t block_mask;
    int valid_lines;
    const char * workload;
    char * decrypted_data = NULL;
    char * decrypted_data_c;
    char *ptr;
    int is_notification_job = FALSE;
    int is_eventhandler_job = FALSE;
    size_t wsize = 0;

    /* reset timeout for now, will be set befor execution again */
    alarm(0);
    signal(SIGALRM, SIG_IGN);

    jobs_done++;

    /* send start signal to parent */
    set_state(GM_JOB_START);

    gm_log( GM_LOG_TRACE, "get_job()\n" );

    /* set size of result */
    *result_size = 0;

    /* reset sleep time */
    sleep_time_after_error = 1;

    /* ignore sigterms while running job */
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &block_mask, NULL);

    /* get the data */
    current_gearman_job = job;
    wsize = gearman_job_workload_size(job);
    workload = (const char *)gearman_job_workload(job);
    if(workload == NULL) {
        *ret_ptr = GEARMAN_WORK_FAIL;
        return NULL;
    }
    gm_log( GM_LOG_TRACE, "got new job %s\n", gearman_job_handle(job));
    gm_log( GM_LOG_TRACE, "%zu +++>\n%.*s\n<+++\n", wsize, (int)wsize, workload);

    /* decrypt data */
    mod_gm_decrypt(worker_ctx, &decrypted_data, workload, wsize, mod_gm_opt->transportmode);
    decrypted_data_c = decrypted_data;

    if(decrypted_data == NULL) {
        *ret_ptr = GEARMAN_WORK_FAIL;
        return NULL;
    }
    gm_log( GM_LOG_TRACE, "%zu --->\n%s\n<---\n", strlen(decrypted_data), decrypted_data );

    /* set result pointer to success */
    *ret_ptr= GEARMAN_SUCCESS;

    exec_job = ( gm_job_t * )gm_malloc( sizeof *exec_job );
    set_default_job(exec_job, mod_gm_opt);

    valid_lines = 0;
    while ( (ptr = strsep(&decrypted_data, "\n" )) != NULL ) {
        char *key   = strsep( &ptr, "=" );
        char *value = strsep( &ptr, "\x0" );

        if ( key == NULL )
            continue;

        if ( value == NULL || !strcmp( value, "") )
            break;

        if ( !strcmp( key, "host_name" ) ) {
            exec_job->host_name = gm_strdup(value);
            valid_lines++;
        } else if ( !strcmp( key, "service_description" ) ) {
            exec_job->service_description = gm_strdup(value);
            valid_lines++;
        } else if ( !strcmp( key, "type" ) ) {
            exec_job->type = gm_strdup(value);
            valid_lines++;
        } else if ( !strcmp( key, "result_queue" ) ) {
            exec_job->result_queue = gm_strdup(value);
            valid_lines++;
        } else if ( !strcmp( key, "check_options" ) ) {
            exec_job->check_options = atoi(value);
            valid_lines++;
        } else if ( !strcmp( key, "scheduled_check" ) ) {
            exec_job->scheduled_check = atoi(value);
            valid_lines++;
        } else if ( !strcmp( key, "reschedule_check" ) ) {
            exec_job->reschedule_check = atoi(value);
            valid_lines++;
        } else if ( !strcmp( key, "latency" ) ) {
            exec_job->latency = atof(value);
            valid_lines++;
        } else if ( !strcmp( key, "next_check" ) ) {
            string2timeval(value, &exec_job->next_check);
            valid_lines++;
        } else if ( !strcmp( key, "start_time" ) ) {
            /* for compatibility reasons... (used by older mod-gearman neb modules) */
            string2timeval(value, &exec_job->next_check);
            string2timeval(value, &exec_job->core_time);
            valid_lines++;
        } else if ( !strcmp( key, "core_time" ) ) {
            string2timeval(value, &exec_job->core_time);
            valid_lines++;
        } else if ( !strcmp( key, "timeout" ) ) {
            exec_job->timeout = atoi(value);
            valid_lines++;
        } else if ( !strcmp( key, "command_line" ) ) {
            exec_job->command_line = gm_strdup(value);
            valid_lines++;
        } else if ( !strcmp( key, "plugin_output" ) ) {
            exec_job->output = gm_strdup(value);
            valid_lines++;
        } else if ( !strcmp( key, "long_plugin_output" ) ) {
            exec_job->long_output = gm_strdup(value);
            valid_lines++;
        }
    }

    if(exec_job->type != NULL && !strcmp( exec_job->type, "notification")) {
        is_notification_job = TRUE;
    }
    else if (exec_job->type != NULL &&  !strcmp( exec_job->type, "eventhandler" ) ) {
        is_eventhandler_job = TRUE;
    }

    /* put plugin_output and long_plugin_output into the environment
     * which is especcially useful for notifications
     */
    if(is_notification_job == TRUE) {
        if(exec_job->service_description != NULL) {
            if(exec_job->output != NULL)
                setenv("NAGIOS_SERVICEOUTPUT",exec_job->output,1);
            if(exec_job->long_output != NULL)
                setenv("NAGIOS_LONGSERVICEOUTPUT",exec_job->output,1);
        } else {
            if(exec_job->output != NULL)
                setenv("NAGIOS_HOSTOUTPUT",exec_job->output,1);
            if(exec_job->long_output != NULL)
                setenv("NAGIOS_LONGHOSTOUTPUT",exec_job->output,1);
        }
    }

    /* will be overwritten */
    if(exec_job->output != NULL)
        free(exec_job->output);

    if(valid_lines == 0) {
        gm_log( GM_LOG_ERROR, "discarded invalid job (%s), check your encryption settings\n", gearman_job_handle( job ) );
    } else {
        do_exec_job();
    }

    current_gearman_job = NULL;

    /* start listening to SIGTERMs */
    sigprocmask(SIG_UNBLOCK, &block_mask, NULL);

    /* log errors for notifications and eventhandler */
    if((is_notification_job || is_eventhandler_job) && exec_job->return_code != 0) {
        gm_log( GM_LOG_ERROR, "%s %s exited with return code %d\n",
               exec_job->service_description != NULL ? "service" : "host",
               exec_job->type,
               exec_job->return_code
        );
        gm_log( GM_LOG_ERROR, "cmd: %s\n", exec_job->command_line );
        gm_log( GM_LOG_ERROR, "output: %s\n", exec_job->output );
    }

    free(decrypted_data_c);
    free_job(exec_job);

    if(is_notification_job == TRUE) {
        /* clear the environment */
        if(exec_job->service_description != NULL) {
            unsetenv("NAGIOS_SERVICEOUTPUT");
            unsetenv("NAGIOS_LONGSERVICEOUTPUT");
        } else {
            unsetenv("NAGIOS_HOSTOUTPUT");
            unsetenv("NAGIOS_LONGHOSTOUTPUT");
        }
    }

    /* send finish signal to parent */
    set_state(GM_JOB_END);

    return NULL;
}


/* do some job */
void do_exec_job( ) {
    struct timeval start_time, end_time;
    int latency, age;

    gm_log( GM_LOG_TRACE, "do_exec_job()\n" );

    if(exec_job->type == NULL) {
        gm_log( GM_LOG_ERROR, "discarded invalid job, no type given\n" );
        return;
    }
    if(exec_job->command_line == NULL) {
        gm_log( GM_LOG_ERROR, "discarded invalid job, no command line given\n" );
        return;
    }

    if ( !strcmp( exec_job->type, "service" ) ) {
        gm_log( GM_LOG_DEBUG, "got service job: %s - %s\n", exec_job->host_name, exec_job->service_description);
    }
    else if ( !strcmp( exec_job->type, "host" ) ) {
        gm_log( GM_LOG_DEBUG, "got host job: %s\n", exec_job->host_name);
    }
    else if ( !strcmp( exec_job->type, "eventhandler" ) ) {
        gm_log( GM_LOG_DEBUG, "got eventhandler job\n");
    }
    else if ( !strcmp( exec_job->type, "notification" ) ) {
        gm_log( GM_LOG_DEBUG, "got notification job\n");
    }

    /* check proper timeout value */
    if( exec_job->timeout <= 0 ) {
        exec_job->timeout = mod_gm_opt->job_timeout;
    }

    /* get the check start time */
    gettimeofday(&start_time,NULL);
    exec_job->start_time = start_time;
    latency = start_time.tv_sec - exec_job->next_check.tv_sec;
    age     = start_time.tv_sec - exec_job->core_time.tv_sec;

    gm_log( GM_LOG_TRACE, "timeout: %i, core latency: %i\n", exec_job->timeout, latency);

    /* job is too old */
    if(mod_gm_opt->max_age > 0 && age > mod_gm_opt->max_age) {
        exec_job->return_code   = 3;

        if ( !strcmp( exec_job->type, "service" ) ) {
            gm_log( GM_LOG_INFO, "discarded too old %s job: %i > %i (%s - %s)\n", exec_job->type, (int)age, mod_gm_opt->max_age, exec_job->host_name, exec_job->service_description);
        } else if ( !strcmp( exec_job->type, "host" ) ) {
            gm_log( GM_LOG_INFO, "discarded too old %s job: %i > %i (%s)\n", exec_job->type, (int)age, mod_gm_opt->max_age, exec_job->host_name);
        } else {
            gm_log( GM_LOG_INFO, "discarded too old %s job: %i > %i\n", exec_job->type, (int)age, mod_gm_opt->max_age);
        }

        gettimeofday(&end_time, NULL);
        exec_job->finish_time = end_time;

        if ( !strcmp( exec_job->type, "service" ) || !strcmp( exec_job->type, "host" ) ) {
            exec_job->output = gm_strdup("(Could Not Start Check In Time)");
            send_result_back(exec_job, worker_ctx);
        }

        return;
    }

    exec_job->early_timeout = 0;

    /* run the command */
    gm_log( GM_LOG_TRACE, "command: %s\n", exec_job->command_line);
    current_job = exec_job;
    execute_safe_command(exec_job, mod_gm_opt->fork_on_exec, mod_gm_opt->identifier );
    current_job = NULL;

    if ( !strcmp( exec_job->type, "service" ) || !strcmp( exec_job->type, "host" ) ) {
        send_result_back(exec_job, worker_ctx);
    }

    return;
}


/* create the worker */
int set_worker(gearman_worker_st **w) {
    int x = 0;

    gm_log( GM_LOG_TRACE, "set_worker()\n" );

    *w = create_worker(mod_gm_opt->server_list);
    if(*w == NULL)
        return GM_ERROR;

    if(worker_run_mode == GM_WORKER_STATUS) {
        /* register status function */
        char status_queue[GM_BUFFERSIZE];
        snprintf(status_queue, GM_BUFFERSIZE, "worker_%s", mod_gm_opt->identifier );
        worker_add_function(*w, status_queue, return_status);
    }
    else {
        /* normal worker */
        if(mod_gm_opt->hosts == GM_ENABLED)
            worker_add_function(*w, "host", get_job);

        if(mod_gm_opt->services == GM_ENABLED)
            worker_add_function(*w, "service", get_job);

        if(mod_gm_opt->events == GM_ENABLED)
            worker_add_function(*w, "eventhandler", get_job);

        if(mod_gm_opt->notifications == GM_ENABLED)
            worker_add_function(*w, "notification", get_job);

        while ( mod_gm_opt->hostgroups_list[x] != NULL ) {
            char buffer[GM_BUFFERSIZE];
            snprintf( buffer, (sizeof(buffer)-1), "hostgroup_%s", mod_gm_opt->hostgroups_list[x] );
            worker_add_function(*w, buffer, get_job);
            x++;
        }

        x = 0;
        while ( mod_gm_opt->servicegroups_list[x] != NULL ) {
            char buffer[GM_BUFFERSIZE];
            snprintf( buffer, (sizeof(buffer)-1), "servicegroup_%s", mod_gm_opt->servicegroups_list[x] );
            worker_add_function(*w, buffer, get_job);
            x++;
        }
    }

    return GM_OK;
}

/* called when worker runs into exit timeout */
void exit_sighandler(int sig) {
    gm_log( GM_LOG_TRACE, "exit_sighandler(%i)\n", sig );
    _exit( EXIT_SUCCESS );
}

/* called when worker runs into idle timeout */
void idle_sighandler(int sig) {
    gm_log( GM_LOG_TRACE, "idle_sighandler(%i)\n", sig );
    clean_worker_exit(0);
    _exit( EXIT_SUCCESS );
}


/* tell parent our state */
void set_state(int status) {
    int *shm;

    gm_log( GM_LOG_TRACE, "set_state(%d)\n", status );

    if(worker_run_mode == GM_WORKER_STANDALONE)
        return;

    /* give us 10 seconds to set state */
    signal(SIGALRM, exit_sighandler);
    alarm(10);

    /* Now we attach the segment to our data space. */
    if ((shm = shmat(shmid, NULL, 0)) == (int *) -1) {
        perror("shmat");
        gm_log( GM_LOG_TRACE, "worker finished: %d\n", getpid() );
        clean_worker_exit(0);
        _exit( EXIT_FAILURE );
    }

    if(status == GM_JOB_START)
        shm[shm_index] = current_pid;
    if(status == GM_JOB_END) {
        shm[SHM_JOBS_DONE]++; /* increase jobs done */

        shm[SHM_WORKER_LAST_CHECK] = (int)time(NULL); /* set last job date */

        /* status slot changed to -1 -> exit */
        if( shm[shm_index] == -1 ) {
            gm_log( GM_LOG_TRACE, "worker finished: %d\n", getpid() );
            clean_worker_exit(0);
            _exit( EXIT_SUCCESS );
        }

        /* pid in our status slot changed, this should not happen -> exit */
        if( shm[shm_index] != current_pid && shm[shm_index] != -current_pid ) {
            gm_log( GM_LOG_ERROR, "double used worker slot: %d != %d\n", current_pid, shm[shm_index] );
            clean_worker_exit(0);
            _exit( EXIT_FAILURE );
        }
        shm[shm_index] = -current_pid;
    }

    /* detach from shared memory */
    if(shmdt(shm) < 0)
        perror("shmdt");

    alarm(0);

    return;
}


/* do a clean exit */
void clean_worker_exit(int sig) {
    int *shm;

    /* give us 30 seconds to stop */
    signal(SIGALRM, exit_sighandler);
    alarm(30);

    gm_log( GM_LOG_TRACE, "clean_worker_exit(%d)\n", sig);

    /* clear gearmans job, otherwise it would be retried and retried */
    if(current_gearman_job != NULL) {
        if(sig == SIGINT) {
            /* if worker stopped with sigint, let the job retry */
        } else {
            send_failed_result(current_job, sig, worker_ctx);
            gearman_job_send_complete(current_gearman_job, NULL, 0);
        }
        /* make sure no processes are left over */
        kill_child_checks();
    }

    gm_log( GM_LOG_TRACE, "cleaning worker\n");
    gm_free_worker(&worker);
    gm_log( GM_LOG_TRACE, "cleaning client\n");
    gm_free_client(&client);
    mod_gm_free_opt(mod_gm_opt);

    if(worker_run_mode == GM_WORKER_STANDALONE)
        exit( EXIT_SUCCESS );

    /* Now we attach the segment to our data space. */
    if((shm = shmat(shmid, NULL, 0)) == (int *) -1) {
        perror("shmat");
        gm_log( GM_LOG_TRACE, "worker finished: %d\n", getpid() );
        _exit( EXIT_FAILURE );
    }
    /* clean our pid from worker list */
    if( shm[shm_index] == current_pid || shm[shm_index] == -current_pid ) {
        shm[shm_index] = -1;
    }

    /* detach from shared memory */
    if(shmdt(shm) < 0)
        perror("shmdt");

    mod_gm_crypt_deinit(worker_ctx);

    _exit( EXIT_SUCCESS );
}


/* answer status querys */
void *return_status( gearman_job_st *job, __attribute__((__unused__)) void *context, size_t *result_size, gearman_return_t *ret_ptr ) {
    size_t wsize = 0;
    const char *workload;
    int *shm;
    char * result = NULL;

    gm_log( GM_LOG_TRACE, "return_status()\n" );

    /* get the data */
    wsize = gearman_job_workload_size(job);
    workload = (const char *)gearman_job_workload(job);
    if(workload == NULL) {
        *ret_ptr = GEARMAN_WORK_FAIL;
        return NULL;
    }
    gm_log( GM_LOG_TRACE, "got status job %s\n", gearman_job_handle(job));
    gm_log( GM_LOG_TRACE, "%zu +++>\n%.*s\n<+++\n", wsize, (int)wsize, workload);

    /* set result pointer to success */
    *ret_ptr= GEARMAN_SUCCESS;

    /* set size of result */
    result = gm_malloc(GM_BUFFERSIZE);
    *result_size = GM_BUFFERSIZE;

    /* give us 10 seconds to get state */
    signal(SIGALRM, exit_sighandler);
    alarm(10);

    /* Now we attach the segment to our data space. */
    if ((shm = shmat(shmid, NULL, 0)) == (int *) -1) {
        perror("shmat");
        *result_size = 0;
        alarm(0);
        gm_free(result);
        return NULL;
    }

    snprintf(result, GM_BUFFERSIZE, "%s has %i worker and is working on %i jobs. Version: %s|worker=%i;;;%i;%i jobs=%ic", hostname, shm[SHM_WORKER_TOTAL], shm[SHM_WORKER_RUNNING], GM_VERSION, shm[SHM_WORKER_TOTAL], mod_gm_opt->min_worker, mod_gm_opt->max_worker, shm[SHM_JOBS_DONE] );

    /* and increase job counter */
    shm[SHM_JOBS_DONE]++;

    /* detach from shared memory */
    if(shmdt(shm) < 0)
        perror("shmdt");

    alarm(0);

    return((void*)result);
}


#ifdef GM_DEBUG
/* write text to a debug file */
void write_debug_file(char ** text) {
    FILE * fd;
    fd = fopen( "/tmp/nagios-mod-gearman-worker.txt", "a+" );
    if(fd == NULL) {
        perror("fopen");
    } else {
        fputs( "------------->\n", fd );
        fputs( *text, fd );
        fputs( "\n<-------------\n", fd );
        fclose( fd );
    }
}
#endif
