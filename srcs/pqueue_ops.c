#include "codexion.h"

static void	sift_down(t_pqueue *pq, int i)
{
	int			smallest;
	int			left;
	int			right;
	t_request	tmp;

	while (1)
	{
		smallest = i;
		left = 2 * i + 1;
		right = 2 * i + 2;
		if (left < pq->size && is_higher_priority(&pq->data[left],
				&pq->data[smallest], pq->policy))
			smallest = left;
		if (right < pq->size && is_higher_priority(&pq->data[right],
				&pq->data[smallest], pq->policy))
			smallest = right;
		if (smallest == i)
			break ;
		tmp = pq->data[i];
		pq->data[i] = pq->data[smallest];
		pq->data[smallest] = tmp;
		i = smallest;
	}
}

static void	sift_up(t_pqueue *pq, int i)
{
	int			parent;
	t_request	tmp;

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
}

int	pqueue_pop(t_pqueue *pq)
{
	if (pq->size == 0)
		return (1);
	pq->data[0] = pq->data[pq->size - 1];
	pq->size--;
	if (pq->size > 0)
		sift_down(pq, 0);
	return (0);
}

void	pqueue_remove_by_id(t_pqueue *pq, int coder_id)
{
	int	i;

	i = 0;
	while (i < pq->size)
	{
		if (pq->data[i].coder_id == coder_id)
		{
			pq->data[i] = pq->data[pq->size - 1];
			pq->size--;
			if (i < pq->size)
			{
				sift_down(pq, i);
				sift_up(pq, i);
			}
			return ;
		}
		i++;
	}
}
