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
 *  @brief header for the neb result thread
 *
 *  @{
 */

#include "mod_gearman.h"

#include <libgearman/gearman.h>

void *result_worker(void *);
int set_worker( gearman_worker_st **worker );
void *get_results( gearman_job_st *, void *, size_t *, gearman_return_t * );

/**
 * @}
 */
