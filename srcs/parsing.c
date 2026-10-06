#include "codexion.h"

static int	is_all_digits(const char *str)
{
	int	i;

	i = 0;
	if (!str || !str[0])
		return (0);
	while (str[i])
	{
		if (str[i] < '0' || str[i] > '9')
			return (0);
		i++;
	}
	return (1);
}

static long long	parse_positive_ll(const char *str, int *error)
{
	long long	res;
	int			i;

	res = 0;
	i = 0;
	if (!is_all_digits(str))
	{
		*error = 1;
		return (-1);
	}
	while (str[i])
	{
		res = res * 10 + (str[i] - '0');
		if (res < 0)
		{
			*error = 1;
			return (-1);
		}
		i++;
	}
	return (res);
}

int	parse_arguments(int argc, char **argv, t_sim *sim)
{
	int	err;

	if (argc != 9)
		return (1);
	err = 0;
	sim->num_coders = (int)parse_positive_ll(argv[1], &err);
	sim->time_to_burnout = parse_positive_ll(argv[2], &err);
	sim->time_to_compile = parse_positive_ll(argv[3], &err);
	sim->time_to_debug = parse_positive_ll(argv[4], &err);
	sim->time_to_refactor = parse_positive_ll(argv[5], &err);
	sim->num_compiles_required = (int)parse_positive_ll(argv[6], &err);
	sim->dongle_cooldown = parse_positive_ll(argv[7], &err);
	if (err || sim->num_coders <= 0 || sim->time_to_burnout <= 0)
		return (1);
	if (strcmp(argv[8], "fifo") == 0)
		sim->scheduler_type = SCHED_FIFO;
	else if (strcmp(argv[8], "edf") == 0)
		sim->scheduler_type = SCHED_EDF;
	else
		return (1);
	return (0);
}
