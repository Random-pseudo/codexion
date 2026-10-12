/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_ops.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lpoisson <lpoisson@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 12:00:00 by lpoisson          #+#    #+#             */
/*   Updated: 2026/10/06 12:00:00 by lpoisson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	sort_queue_indices(t_sim *sim, int *indices, int n)
{
	int	i;
	int	j;
	int	best;
	int	tmp;

	i = 0;
	while (i < n)
	{
		best = i;
		j = i + 1;
		while (j < n)
		{
			if (is_higher_priority(&sim->queue.data[indices[j]],
					&sim->queue.data[indices[best]], sim->queue.policy))
				best = j;
			j++;
		}
		if (best != i)
		{
			tmp = indices[i];
			indices[i] = indices[best];
			indices[best] = tmp;
		}
		i++;
	}
}

static void	mark_dongles_taken(t_coder *coder, int cid, long long now)
{
	pthread_mutex_lock(&coder->first_dongle->mutex);
	coder->first_dongle->is_in_use = 1;
	coder->first_dongle->holder_id = cid;
	pthread_mutex_unlock(&coder->first_dongle->mutex);
	pthread_mutex_lock(&coder->second_dongle->mutex);
	coder->second_dongle->is_in_use = 1;
	coder->second_dongle->holder_id = cid;
	pthread_mutex_unlock(&coder->second_dongle->mutex);
	pthread_mutex_lock(&coder->state_mutex);
	coder->last_compile_start = now;
	pthread_mutex_unlock(&coder->state_mutex);
	coder->is_granted = 1;
	pqueue_remove_by_id(&coder->sim->queue, cid);
	pthread_cond_signal(&coder->cond);
}

static int	try_grant_coder(t_sim *sim, int cid, long long now)
{
	t_coder		*coder;
	t_dongle	*d1;
	t_dongle	*d2;

	coder = &sim->coders[cid - 1];
	d1 = coder->first_dongle;
	d2 = coder->second_dongle;
	if (!d2 || d1->is_in_use || now < d1->cooldown_until
		|| d2->is_in_use || now < d2->cooldown_until)
		return (0);
	mark_dongles_taken(coder, cid, now);
	return (1);
}

static void	init_indices(int *indices, int n)
{
	int	i;

	i = 0;
	while (i < n)
	{
		indices[i] = i;
		i++;
	}
}

void	dispatch(t_sim *sim)
{
	int			n;
	int			i;
	long long	now;

	n = sim->queue.size;
	if (n <= 0)
		return ;
	now = get_time_ms();
	init_indices(sim->sorted_indices, n);
	sort_queue_indices(sim, sim->sorted_indices, n);
	i = 0;
	while (i < n)
	{
		if (try_grant_coder(sim,
				sim->queue.data[sim->sorted_indices[i]].coder_id, now))
		{
			dispatch(sim);
			return ;
		}
		i++;
	}
}
