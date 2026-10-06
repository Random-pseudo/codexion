#include "codexion.h"

int	dongle_init(t_dongle *dongle, int id, int num_coders, int policy)
{
	dongle->id = id;
	dongle->is_in_use = 0;
	dongle->holder_id = 0;
	dongle->cooldown_until = 0;
	dongle->seq_counter = 0;
	if (pthread_mutex_init(&dongle->mutex, NULL) != 0)
		return (1);
	if (pthread_cond_init(&dongle->cond, NULL) != 0)
	{
		pthread_mutex_destroy(&dongle->mutex);
		return (1);
	}
	if (pqueue_init(&dongle->queue, num_coders + 4, policy) != 0)
	{
		pthread_mutex_destroy(&dongle->mutex);
		pthread_cond_destroy(&dongle->cond);
		return (1);
	}
	return (0);
}

void	dongle_destroy(t_dongle *dongle)
{
	pthread_mutex_destroy(&dongle->mutex);
	pthread_cond_destroy(&dongle->cond);
	pqueue_destroy(&dongle->queue);
}

static int	wait_for_dongle(t_dongle *dongle, t_coder *coder)
{
	t_request		top;
	long long		now;
	struct timespec	ts;

	while (!sim_is_stopped(coder->sim))
	{
		pqueue_peek(&dongle->queue, &top);
		now = get_time_ms();
		if (top.coder_id == coder->id && !dongle->is_in_use
			&& now >= dongle->cooldown_until)
		{
			pqueue_pop(&dongle->queue);
			dongle->is_in_use = 1;
			dongle->holder_id = coder->id;
			return (0);
		}
		if (dongle->is_in_use || top.coder_id != coder->id)
			pthread_cond_wait(&dongle->cond, &dongle->mutex);
		else
		{
			ms_to_timespec(dongle->cooldown_until, &ts);
			pthread_cond_timedwait(&dongle->cond, &dongle->mutex, &ts);
		}
	}
	return (1);
}

int	dongle_take(t_dongle *dongle, t_coder *coder)
{
	t_request	req;
	int			ret;

	pthread_mutex_lock(&dongle->mutex);
	if (sim_is_stopped(coder->sim))
	{
		pthread_mutex_unlock(&dongle->mutex);
		return (1);
	}
	req.coder_id = coder->id;
	req.request_time = get_time_ms();
	pthread_mutex_lock(&coder->state_mutex);
	req.deadline = coder->last_compile_start + coder->sim->time_to_burnout;
	pthread_mutex_unlock(&coder->state_mutex);
	req.seq = ++dongle->seq_counter;
	pqueue_push(&dongle->queue, req);
	ret = wait_for_dongle(dongle, coder);
	if (ret != 0)
		pqueue_remove_by_id(&dongle->queue, coder->id);
	pthread_mutex_unlock(&dongle->mutex);
	if (ret == 0)
		print_log(coder, "has taken a dongle");
	return (ret);
}

void	dongle_release(t_dongle *dongle, t_coder *coder)
{
	pthread_mutex_lock(&dongle->mutex);
	if (dongle->holder_id == coder->id)
	{
		dongle->is_in_use = 0;
		dongle->holder_id = 0;
		dongle->cooldown_until = get_time_ms() + coder->sim->dongle_cooldown;
		pthread_cond_broadcast(&dongle->cond);
	}
	pthread_mutex_unlock(&dongle->mutex);
}
