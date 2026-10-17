/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lpoisson <lpoisson@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 12:00:00 by lpoisson          #+#    #+#             */
/*   Updated: 2026/10/12 19:21:44 by lpoisson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static long long	find_cooldown_wake(t_sim *sim, long long now)
{
	long long	wake_ms;
	int			i;

	wake_ms = 0;
	i = 0;
	while (i < sim->num_coders)
	{
		if (sim->dongles[i].cooldown_until > now)
		{
			if (wake_ms == 0 || sim->dongles[i].cooldown_until < wake_ms)
				wake_ms = sim->dongles[i].cooldown_until;
		}
		i++;
	}
	return (wake_ms);
}

static void	wait_for_dongles(t_coder *coder, t_sim *sim)
{
	long long		now;
	long long		wake_ms;
	struct timespec	ts;

	while (!coder->is_granted && !sim_is_stopped(sim))
	{
		now = get_time_ms();
		wake_ms = find_cooldown_wake(sim, now);
		if (wake_ms > now)
		{
			ms_to_timespec(wake_ms, &ts);
			pthread_cond_timedwait(&coder->cond, &sim->table_mutex, &ts);
		}
		else
			pthread_cond_wait(&coder->cond, &sim->table_mutex);
		dispatch(sim);
	}
}

static t_request	create_request(t_coder *coder)
{
	t_request	req;

	req.coder_id = coder->id;
	req.request_time = get_time_ms();
	pthread_mutex_lock(&coder->state_mutex);
	req.deadline = coder->last_compile_start + coder->sim->time_to_burnout;
	pthread_mutex_unlock(&coder->state_mutex);
	req.seq = ++coder->sim->seq_counter;
	return (req);
}

int	acquire_dongles(t_coder *coder)
{
	t_sim		*sim;
	t_request	req;

	sim = coder->sim;
	if (!coder->second_dongle)
		return (handle_single_coder(coder));
	pthread_mutex_lock(&sim->table_mutex);
	if (sim_is_stopped(sim))
	{
		pthread_mutex_unlock(&sim->table_mutex);
		return (1);
	}
	req = create_request(coder);
	coder->is_granted = 0;
	pqueue_push(&sim->queue, req);
	dispatch(sim);
	wait_for_dongles(coder, sim);
	if (!coder->is_granted)
		pqueue_remove_by_id(&sim->queue, coder->id);
	pthread_mutex_unlock(&sim->table_mutex);
	return (!coder->is_granted);
}

void	release_dongles(t_coder *coder)
{
	t_sim		*sim;
	long long	now;

	sim = coder->sim;
	if (!coder->second_dongle)
		return ;
	now = get_time_ms();
	pthread_mutex_lock(&sim->table_mutex);
	pthread_mutex_lock(&coder->first_dongle->mutex);
	coder->first_dongle->is_in_use = 0;
	coder->first_dongle->cooldown_until = now + sim->dongle_cooldown;
	pthread_mutex_unlock(&coder->first_dongle->mutex);
	pthread_mutex_lock(&coder->second_dongle->mutex);
	coder->second_dongle->is_in_use = 0;
	coder->second_dongle->cooldown_until = now + sim->dongle_cooldown;
	pthread_mutex_unlock(&coder->second_dongle->mutex);
	dispatch(sim);
	pthread_mutex_unlock(&sim->table_mutex);
}
