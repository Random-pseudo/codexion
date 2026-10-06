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

static void	handle_single_coder(t_coder *coder)
{
	if (dongle_take(coder->first_dongle, coder) != 0)
		return ;
	while (!sim_is_stopped(coder->sim))
		usleep(1000);
	dongle_release(coder->first_dongle, coder);
}

static int	perform_compilation(t_coder *coder)
{
	if (dongle_take(coder->first_dongle, coder) != 0)
		return (1);
	if (!coder->second_dongle)
	{
		handle_single_coder(coder);
		return (1);
	}
	if (dongle_take(coder->second_dongle, coder) != 0)
	{
		dongle_release(coder->first_dongle, coder);
		return (1);
	}
	pthread_mutex_lock(&coder->state_mutex);
	coder->last_compile_start = get_time_ms();
	coder->compile_count++;
	pthread_mutex_unlock(&coder->state_mutex);
	print_log(coder, "is compiling");
	precise_sleep(coder->sim->time_to_compile, coder->sim);
	dongle_release(coder->second_dongle, coder);
	dongle_release(coder->first_dongle, coder);
	return (0);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;

	coder = (t_coder *)arg;
	if (coder->sim->num_coders > 1 && coder->id % 2 == 0)
		usleep(1000);
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
