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

/**
 * @file
 * @brief nagios-check-gearman nagios plugin
 * @addtogroup nagios_mod_gearman_check_gearman nagios-check-gearman
 *
 * nagios-check-gearman can be used as a nagios plugin to verify gearman server and worker.
 * It is part of the Nagios-Mod-Gearman package but not limited to Nagios-Mod-Gearman.
 *
 * @{
 */

#define MOD_GM_CHECK_GEARMAN             /**< set nagios-check-gearman mode */

#define PLUGIN_NAME    "nagios-check-gearman"   /**< set the name of the plugin */

#include <stdlib.h>
#include <signal.h>
#include "common.h"

/** nagios-check-gearman
 *
 * main function of nagios-check-gearman
 *
 * @param[in] argc - number of arguments
 * @param[in] argv - list of arguments
 *
 * @return exits with a nagios compatible exit code
 */
int main (int argc, char **argv);

/**
 *
 * print the usage and exit
 *
 * @return exits with a nagios compatible exit code
 */
void print_usage(void);

/**
 *
 * print the version and exit
 *
 * @return exits with a nagios compatible exit code
 */
void print_version(void);

/**
 *
 * signal handler for sig alarm
 *
 * @param[in] sig - signal number
 *
 * @return exits with a nagios compatible exit code
 */
void alarm_sighandler(int sig);

/**
 *
 * check a gearmand server
 *
 * @param[in] server - server to check
 *
 * @return returns a nagios compatible exit code
 */
int check_server(char * server, in_port_t port);

/**
 *
 * check a gearman worker
 *
 * @param[in] queue - queue name (function)
 * @param[in] send - put this text as job into the queue
 * @param[in] expect - returning text to expect
 *
 * @return returns a nagios compatible exit code
 */
int check_worker(char * queue, char * send, char * expect);

/**
 * @}
 */
