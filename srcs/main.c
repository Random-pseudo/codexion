#include "codexion.h"

static int	init_coders(t_sim *sim)
{
	int	i;
	int	d1;
	int	d2;

	i = 0;
	while (i < sim->num_coders)
	{
		sim->coders[i].id = i + 1;
		sim->coders[i].sim = sim;
		sim->coders[i].compile_count = 0;
		if (pthread_mutex_init(&sim->coders[i].state_mutex, NULL) != 0)
			return (1);
		d1 = i;
		d2 = (i + 1) % sim->num_coders;
		if (sim->num_coders == 1 || d1 < d2)
			sim->coders[i].first_dongle = &sim->dongles[d1 < d2 ? d1 : 0];
		else
			sim->coders[i].first_dongle = &sim->dongles[d2];
		if (sim->num_coders == 1)
			sim->coders[i].second_dongle = NULL;
		else
			sim->coders[i].second_dongle = &sim->dongles[d1 < d2 ? d2 : d1];
		i++;
	}
	return (0);
}

static int	init_sim(t_sim *sim)
{
	int	i;

	sim->dongles = malloc(sizeof(t_dongle) * sim->num_coders);
	sim->coders = malloc(sizeof(t_coder) * sim->num_coders);
	if (!sim->dongles || !sim->coders)
		return (1);
	if (pthread_mutex_init(&sim->stop_mutex, NULL) != 0)
		return (1);
	if (pthread_mutex_init(&sim->print_mutex, NULL) != 0)
		return (1);
	i = 0;
	while (i < sim->num_coders)
	{
		if (dongle_init(&sim->dongles[i], i + 1,
				sim->num_coders, sim->scheduler_type) != 0)
			return (1);
		i++;
	}
	return (init_coders(sim));
}

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
