/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pqueue.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lpoisson <lpoisson@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/06 12:00:00 by lpoisson          #+#    #+#             */
/*   Updated: 2026/10/12 13:50:04 by lpoisson         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	is_higher_priority(t_request *a, t_request *b, int policy)
{
	if (policy == SCHED_FIFO)
	{
		if (a->request_time != b->request_time)
			return (a->request_time > b->request_time);
		return (a->seq > b->seq);
	}
	if (a->deadline != b->deadline)
		return (a->deadline < b->deadline);
	if (a->request_time != b->request_time)
		return (a->request_time < b->request_time);
	return (a->coder_id < b->coder_id);
}

int	pqueue_init(t_pqueue *pq, int capacity, int policy)
{
	pq->data = (t_request *)malloc(sizeof(t_request) * capacity);
	if (!pq->data)
		return (1);
	pq->size = 0;
	pq->capacity = capacity;
	pq->policy = policy;
	return (0);
}

void	pqueue_destroy(t_pqueue *pq)
{
	if (pq->data)
	{
		free(pq->data);
		pq->data = NULL;
	}
	pq->size = 0;
	pq->capacity = 0;
}

int	pqueue_peek(t_pqueue *pq, t_request *out)
{
	if (pq->size == 0)
		return (1);
	*out = pq->data[0];
	return (0);
}

int	pqueue_push(t_pqueue *pq, t_request req)
{
	int			i;
	int			parent;
	t_request	tmp;

	if (pq->size >= pq->capacity)
		return (1);
	i = pq->size++;
	pq->data[i] = req;
	while (i > 0)
	{
		parent = (i - 1) / 2;
		if (is_higher_priority(&pq->data[i], &pq->data[parent], pq->policy))
		{
			tmp = pq->data[i];
			pq->data[i] = pq->data[parent];
			pq->data[parent] = tmp;
			i = parent;
		}
		else
			break ;
	}
	return (0);
}
