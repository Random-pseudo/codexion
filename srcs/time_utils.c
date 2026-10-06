#include "codexion.h"

long long	get_time_ms(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long long)tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}

void	precise_sleep(long long duration_ms, t_sim *sim)
{
	long long	start;
	long long	remaining;

	start = get_time_ms();
	while (!sim_is_stopped(sim))
	{
		remaining = duration_ms - (get_time_ms() - start);
		if (remaining <= 0)
			break ;
		if (remaining > 5)
			usleep(2000);
		else
			usleep(200);
	}
}

void	ms_to_timespec(long long target_ms, struct timespec *ts)
{
	ts->tv_sec = (time_t)(target_ms / 1000LL);
	ts->tv_nsec = (long)((target_ms % 1000LL) * 1000000L);
}
