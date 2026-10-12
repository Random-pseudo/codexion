/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lpoisson <lpoisson@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 12:00:00 by lpoisson          #+#    #+#             */
/*   Updated: 2026/10/06 12:00:00 by lpoisson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	print_log(t_coder *coder, const char *msg)
{
	long long	ts;

	pthread_mutex_lock(&coder->sim->print_mutex);
	if (!sim_is_stopped(coder->sim))
	{
		ts = get_time_ms() - coder->sim->start_time_ms;
		printf("%lld %d %s\n", ts, coder->id, msg);
	}
	pthread_mutex_unlock(&coder->sim->print_mutex);
}

static void	print_compile_start(t_coder *coder)
{
	long long	ts_log;

	pthread_mutex_lock(&coder->sim->print_mutex);
	if (!sim_is_stopped(coder->sim))
	{
		ts_log = get_time_ms() - coder->sim->start_time_ms;
		printf("%lld %d has taken a dongle\n", ts_log, coder->id);
		printf("%lld %d has taken a dongle\n", ts_log, coder->id);
		printf("%lld %d is compiling\n", ts_log, coder->id);
	}
	pthread_mutex_unlock(&coder->sim->print_mutex);
}

int	handle_single_coder(t_coder *coder)
{
	pthread_mutex_lock(&coder->first_dongle->mutex);
	if (sim_is_stopped(coder->sim))
	{
		pthread_mutex_unlock(&coder->first_dongle->mutex);
		return (1);
	}
	coder->first_dongle->is_in_use = 1;
	coder->first_dongle->holder_id = coder->id;
	pthread_mutex_unlock(&coder->first_dongle->mutex);
	print_log(coder, "has taken a dongle");
	while (!sim_is_stopped(coder->sim))
		usleep(1000);
	pthread_mutex_lock(&coder->first_dongle->mutex);
	coder->first_dongle->is_in_use = 0;
	coder->first_dongle->holder_id = 0;
	pthread_mutex_unlock(&coder->first_dongle->mutex);
	return (1);
}

static int	perform_compilation(t_coder *coder)
{
	if (acquire_dongles(coder) != 0)
		return (1);
	print_compile_start(coder);
	precise_sleep(coder->sim->time_to_compile, coder->sim);
	release_dongles(coder);
	pthread_mutex_lock(&coder->state_mutex);
	coder->compile_count++;
	pthread_mutex_unlock(&coder->state_mutex);
	return (0);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;

	coder = (t_coder *)arg;
	while (!sim_is_stopped(coder->sim))
	{
		if (perform_compilation(coder) != 0)
			break ;
		if (sim_is_stopped(coder->sim))
			break ;
		print_log(coder, "is debugging");
		precise_sleep(coder->sim->time_to_debug, coder->sim);
		if (sim_is_stopped(coder->sim))
			break ;
		print_log(coder, "is refactoring");
		precise_sleep(coder->sim->time_to_refactor, coder->sim);
	}
	return (NULL);
}
