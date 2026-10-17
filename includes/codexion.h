/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lpoisson <lpoisson@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 12:00:00 by lpoisson          #+#    #+#             */
/*   Updated: 2026/10/06 12:00:00 by lpoisson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <sys/time.h>
# include <unistd.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>

# define SCHED_FIFO 1
# define SCHED_EDF 2

typedef struct s_sim	t_sim;

typedef struct s_request
{
	int			coder_id;
	long long	request_time;
	long long	deadline;
	long long	seq;
}	t_request;

typedef struct s_pqueue
{
	t_request	*data;
	int			size;
	int			capacity;
	int			policy;
}	t_pqueue;

typedef struct s_dongle
{
	pthread_mutex_t	mutex;
	int				is_in_use;
	long long		cooldown_until;
}	t_dongle;

typedef struct s_coder
{
	int				id;
	pthread_t		thread;
	t_dongle		*first_dongle;
	t_dongle		*second_dongle;
	long long		last_compile_start;
	int				compile_count;
	int				is_granted;
	pthread_cond_t	cond;
	pthread_mutex_t	state_mutex;
	t_sim			*sim;
}	t_coder;

struct s_sim
{
	int				num_coders;
	long long		time_to_burnout;
	long long		time_to_compile;
	long long		time_to_debug;
	long long		time_to_refactor;
	int				num_compiles_required;
	long long		dongle_cooldown;
	int				scheduler_type;
	long long		start_time_ms;
	int				is_stopped;
	long long		seq_counter;
	pthread_mutex_t	stop_mutex;
	pthread_mutex_t	print_mutex;
	pthread_mutex_t	table_mutex;
	t_pqueue		queue;
	t_dongle		*dongles;
	t_coder			*coders;
	int				*sorted_indices;
	pthread_t		monitor_thread;
};

/* parsing.c */
int			parse_arguments(int argc, char **argv, t_sim *sim);

/* time_utils.c */
long long	get_time_ms(void);
void		precise_sleep(long long duration_ms, t_sim *sim);
void		ms_to_timespec(long long target_ms, struct timespec *ts);

/* pqueue.c */
int			is_higher_priority(t_request *a, t_request *b, int policy);
int			pqueue_init(t_pqueue *pq, int capacity, int policy);
void		pqueue_destroy(t_pqueue *pq);
int			pqueue_push(t_pqueue *pq, t_request req);

/* pqueue_ops.c */
void		sift_down(t_pqueue *pq, int i);
void		sift_up(t_pqueue *pq, int i);
void		pqueue_remove_at(t_pqueue *pq, int idx);
void		pqueue_remove_by_id(t_pqueue *pq, int coder_id);

/* dongle.c */
void		dispatch(t_sim *sim);
int			acquire_dongles(t_coder *coder);
void		release_dongles(t_coder *coder);

/* coder.c */
void		print_log(t_coder *coder, const char *msg);
int			handle_single_coder(t_coder *coder);
void		*coder_routine(void *arg);

/* monitor.c */
int			sim_is_stopped(t_sim *sim);
void		sim_stop_and_broadcast(t_sim *sim);
void		*monitor_routine(void *arg);

/* init.c */
int			init_sim(t_sim *sim);

/* cleanup.c */
void		cleanup_sim(t_sim *sim);

#endif
