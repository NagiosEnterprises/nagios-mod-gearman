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

#include "config.h"

#define MOD_GM_NEB  /**< set nagios_mod_gearman neb features */
#define NSCORE      /**< enable core features         */

#include "utils.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

/** @file
 *  @brief Nagios-Mod-Gearman NEB Module
 *  @addtogroup nagios_mod_gearman_neb_module NEB Module
 *
 * The Nagios-Mod-Gearman NEB module loads into the core and intercepts scheduled host
 * and service checks as well as eventhander jobs.
 * The module start a single new thread which acts as gearman client and worker.
 * The client is used to send new jobs into the gearman queues (functions). The
 * worker listens on the result queue and puts back the finished results.
 * Before the core reaps the result they will be merged together with the ones
 * from gearman.
 *
 * @{
 */

/* include nagios */
#include "nagios/nagios.h"
#include "nagios/neberrors.h"
#include "nagios/nebstructs.h"
#include "nagios/nebcallbacks.h"
#include "nagios/broker.h"
#include "nagios/macros.h"

/* include the gearman libs */
#include <libgearman/gearman.h>

/** main NEB module init function
 *
 * this function gets initally called when loading the module
 *
 * @param[in] flags  - module flags
 * @param[in] args   - module arguments from the core config
 * @param[in] handle - our module handle
 *
 * @return Zero on success, or a non-zero error value.
 */
int nebmodule_init( int flags, char * args, nebmodule * handle);

/** NEB module deinit function
 *
 * this function gets called before unloading the module from the core
 *
 * @param[in] flags  - module flags
 * @param[in] reason - reason for unloading the module
 *
 * @return nothing
 */
int nebmodule_deinit( int flags, int reason );

/** adds check result to result list
 *
 * @param[in] newcheckresult - new checkresult structure to add to list
 *
 * @return nothing
 */
void mod_gm_add_result_to_list(check_result * newcheckresult);

/** wraps the write_to_all_logs core logger
 *
 * @param[in] type - type of the log event
 * @param[in] data - actual text to log
 *
 * @return nothing
 */
void log_core(int type, char *data);

/**
 * @}
 */
