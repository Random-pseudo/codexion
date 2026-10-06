#include "codexion.h"

static void	cleanup_dongles(t_sim *sim)
{
	int	i;

	if (!sim->dongles)
		return ;
	i = 0;
	while (i < sim->num_coders)
	{
		dongle_destroy(&sim->dongles[i]);
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
}
