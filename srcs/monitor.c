#include "codexion.h"

int	sim_is_stopped(t_sim *sim)
{
	int	stopped;

	pthread_mutex_lock(&sim->stop_mutex);
	stopped = sim->is_stopped;
	pthread_mutex_unlock(&sim->stop_mutex);
	return (stopped);
}

void	sim_stop_and_broadcast(t_sim *sim)
{
	int	i;

	pthread_mutex_lock(&sim->stop_mutex);
	sim->is_stopped = 1;
	pthread_mutex_unlock(&sim->stop_mutex);
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_mutex_lock(&sim->dongles[i].mutex);
		pthread_cond_broadcast(&sim->dongles[i].cond);
		pthread_mutex_unlock(&sim->dongles[i].mutex);
		i++;
	}
}

static int	check_burnout(t_sim *sim, t_coder *coder, long long now)
{
	long long	last_start;
	long long	ts;

	pthread_mutex_lock(&coder->state_mutex);
	last_start = coder->last_compile_start;
	pthread_mutex_unlock(&coder->state_mutex);
	if (now - last_start >= sim->time_to_burnout)
	{
		pthread_mutex_lock(&sim->print_mutex);
		if (!sim_is_stopped(sim))
		{
			ts = now - sim->start_time_ms;
			printf("%lld %d burned out\n", ts, coder->id);
			sim_stop_and_broadcast(sim);
		}
		pthread_mutex_unlock(&sim->print_mutex);
		return (1);
	}
	return (0);
}

static int	check_all_compiled(t_sim *sim)
{
	int	i;
	int	count;

	if (sim->num_compiles_required <= 0)
		return (0);
	i = 0;
	while (i < sim->num_coders)
	{
		pthread_mutex_lock(&sim->coders[i].state_mutex);
		count = sim->coders[i].compile_count;
		pthread_mutex_unlock(&sim->coders[i].state_mutex);
		if (count < sim->num_compiles_required)
			return (0);
		i++;
	}
	sim_stop_and_broadcast(sim);
	return (1);
}

void	*monitor_routine(void *arg)
{
	t_sim		*sim;
	int			i;
	long long	now;

	sim = (t_sim *)arg;
	while (!sim_is_stopped(sim))
	{
		now = get_time_ms();
		i = 0;
		while (i < sim->num_coders)
		{
			if (check_burnout(sim, &sim->coders[i], now))
				return (NULL);
			i++;
		}
		if (check_all_compiled(sim))
			return (NULL);
		usleep(500);
	}
	return (NULL);
}
