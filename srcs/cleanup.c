/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cleanup.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lpoisson <lpoisson@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 12:00:00 by lpoisson          #+#    #+#             */
/*   Updated: 2026/10/06 12:00:00 by lpoisson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	cleanup_dongles(t_sim *sim)
{
	int	i;

	if (!sim->dongles)
		return ;
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_mutex_destroy(&sim->dongles[i].mutex);
		pthread_cond_destroy(&sim->dongles[i].cond);
		i++;
	}
	free(sim->dongles);
	sim->dongles = NULL;
}

static void	cleanup_coders(t_sim *sim)
{
	int	i;

	if (!sim->coders)
		return ;
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_mutex_destroy(&sim->coders[i].state_mutex);
		pthread_cond_destroy(&sim->coders[i].cond);
		i++;
	}
	free(sim->coders);
	sim->coders = NULL;
}

void	cleanup_sim(t_sim *sim)
{
	cleanup_dongles(sim);
	cleanup_coders(sim);
	pthread_mutex_destroy(&sim->stop_mutex);
	pthread_mutex_destroy(&sim->print_mutex);
	pthread_mutex_destroy(&sim->table_mutex);
	pthread_cond_destroy(&sim->table_cond);
	pqueue_destroy(&sim->queue);
	if (sim->sorted_indices)
	{
		free(sim->sorted_indices);
		sim->sorted_indices = NULL;
	}
}
