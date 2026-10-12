/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lpoisson <lpoisson@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 12:00:00 by lpoisson          #+#    #+#             */
/*   Updated: 2026/10/06 12:00:00 by lpoisson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	start_sim(t_sim *sim)
{
	int	i;

	sim->start_time_ms = get_time_ms();
	sim->is_stopped = 0;
	i = 0;
	while (i < sim->num_coders)
	{
		sim->coders[i].last_compile_start = sim->start_time_ms;
		if (pthread_create(&sim->coders[i].thread, NULL,
				coder_routine, &sim->coders[i]) != 0)
			return (1);
		i++;
	}
	if (pthread_create(&sim->monitor_thread, NULL,
			monitor_routine, sim) != 0)
		return (1);
	return (0);
}

static void	wait_sim(t_sim *sim)
{
	int	i;

	pthread_join(sim->monitor_thread, NULL);
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
}

int	main(int argc, char **argv)
{
	t_sim	sim;

	memset(&sim, 0, sizeof(t_sim));
	if (parse_arguments(argc, argv, &sim) != 0)
	{
		write(2, "Error: Invalid arguments\n", 25);
		return (1);
	}
	if (sim.num_compiles_required == 0)
		return (0);
	if (init_sim(&sim) != 0)
	{
		cleanup_sim(&sim);
		write(2, "Error: Initialization failed\n", 29);
		return (1);
	}
	if (start_sim(&sim) == 0)
		wait_sim(&sim);
	cleanup_sim(&sim);
	return (0);
}
