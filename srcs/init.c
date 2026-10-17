/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lpoisson <lpoisson@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 12:00:00 by lpoisson          #+#    #+#             */
/*   Updated: 2026/10/06 12:00:00 by lpoisson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	init_dongles(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->num_coders)
	{
		sim->dongles[i].is_in_use = 0;
		sim->dongles[i].cooldown_until = 0;
		if (pthread_mutex_init(&sim->dongles[i].mutex, NULL) != 0)
			return (1);
		i++;
	}
	return (0);
}

static int	init_coders(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->num_coders)
	{
		sim->coders[i].id = i + 1;
		sim->coders[i].sim = sim;
		sim->coders[i].compile_count = 0;
		sim->coders[i].is_granted = 0;
		if (pthread_mutex_init(&sim->coders[i].state_mutex, NULL) != 0)
			return (1);
		if (pthread_cond_init(&sim->coders[i].cond, NULL) != 0)
			return (1);
		sim->coders[i].first_dongle = &sim->dongles[i];
		sim->coders[i].second_dongle = NULL;
		if (sim->num_coders > 1)
			sim->coders[i].second_dongle = &sim->dongles[(i + 1)
				% sim->num_coders];
		i++;
	}
	return (0);
}

int	init_sim(t_sim *sim)
{
	sim->dongles = malloc(sizeof(t_dongle) * sim->num_coders);
	sim->coders = malloc(sizeof(t_coder) * sim->num_coders);
	sim->sorted_indices = malloc(sizeof(int) * (sim->num_coders + 16));
	if (!sim->dongles || !sim->coders || !sim->sorted_indices)
		return (1);
	if (pthread_mutex_init(&sim->stop_mutex, NULL) != 0)
		return (1);
	if (pthread_mutex_init(&sim->print_mutex, NULL) != 0)
		return (1);
	if (pthread_mutex_init(&sim->table_mutex, NULL) != 0)
		return (1);
	if (pqueue_init(&sim->queue, sim->num_coders + 16,
			sim->scheduler_type) != 0)
		return (1);
	if (init_dongles(sim) != 0)
		return (1);
	return (init_coders(sim));
}
