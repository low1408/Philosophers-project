#include "philo.h"

static int	ft_isdigit(int c)
{
	return (c >= '0' && c <= '9');
}

static int	ft_atoi_safe(const char *str, int *result)
{
	long	num;
	int		i;

	num = 0;
	i = 0;
	if (!str || !str[0])
		return (1);
	while (str[i])
	{
		if (!ft_isdigit(str[i]))
			return (1);
		num = num * 10 + (str[i] - '0');
		if (num > INT_MAX)
			return (1);
		i++;
	}
	*result = (int)num;
	return (0);
}

static int	ft_strcmp(const char *s1, const char *s2)
{
	int	i;

	i = 0;
	while (s1[i] && s2[i] && s1[i] == s2[i])
		i++;
	return ((unsigned char)s1[i] - (unsigned char)s2[i]);
}

static int	parse_strategy(t_data *data, const char *arg)
{
	if (!ft_strcmp(arg, "ordered"))
		data->strategy = STRATEGY_ORDERED;
	else if (!ft_strcmp(arg, "banker"))
		data->strategy = STRATEGY_BANKER;
	else if (!ft_strcmp(arg, "chandy") || !ft_strcmp(arg, "chandy_misra"))
		data->strategy = STRATEGY_CHANDY;
	else
		return (printf("Error: invalid strategy\n"), 1);
	return (0);
}

int	parse_args(t_data *data, int argc, char **argv)
{
	int	val;

	if (argc < 5 || argc > 7)
		return (printf("Error: wrong number of arguments\n"), 1);
	data->strategy = STRATEGY_ORDERED;
	if (ft_atoi_safe(argv[1], &val) || val < 1)
		return (printf("Error: invalid number of philosophers\n"), 1);
	data->num_philos = val;
	if (ft_atoi_safe(argv[2], &val) || val < 1)
		return (printf("Error: invalid time_to_die\n"), 1);
	data->time_to_die = (long long)val;
	if (ft_atoi_safe(argv[3], &val) || val < 1)
		return (printf("Error: invalid time_to_eat\n"), 1);
	data->time_to_eat = (long long)val;
	if (ft_atoi_safe(argv[4], &val) || val < 1)
		return (printf("Error: invalid time_to_sleep\n"), 1);
	data->time_to_sleep = (long long)val;
	data->must_eat_count = -1;
	if (argc >= 6)
	{
		if (argc == 6 && ft_atoi_safe(argv[5], &val))
			return (parse_strategy(data, argv[5]));
		if (ft_atoi_safe(argv[5], &val) || val < 0)
			return (printf("Error: invalid must_eat_count\n"), 1);
		data->must_eat_count = val;
	}
	if (argc == 7)
	{
		if (parse_strategy(data, argv[6]))
			return (1);
	}
	return (0);
}

static int	launch_simulation(t_data *data)
{
	int			i;
	long long	start_time;
	pthread_t	monitor;

	i = 0;
	while (i < data->num_philos)
	{
		if (pthread_create(&data->philos[i].thread_id, NULL,
				philosopher_routine, &data->philos[i]) != 0)
		{
			pthread_mutex_lock(&data->stop_mutex);
			data->simulation_stopped = true;
			data->simulation_started = true;
			pthread_mutex_unlock(&data->stop_mutex);
			while (--i >= 0)
				pthread_join(data->philos[i].thread_id, NULL);
			return (1);
		}
		i++;
	}
	if (pthread_create(&monitor, NULL, monitor_routine, data) != 0)
	{
		pthread_mutex_lock(&data->stop_mutex);
		data->simulation_stopped = true;
		data->simulation_started = true;
		pthread_mutex_unlock(&data->stop_mutex);
		i = data->num_philos;
		while (--i >= 0)
			pthread_join(data->philos[i].thread_id, NULL);
		return (1);
	}
	start_time = get_time_in_ms();
	pthread_mutex_lock(&data->stop_mutex);
	data->start_time = start_time;
	pthread_mutex_unlock(&data->stop_mutex);
	i = 0;
	while (i < data->num_philos)
	{
		pthread_mutex_lock(&data->philos[i].state_mutex);
		data->philos[i].last_meal_time = start_time;
		pthread_mutex_unlock(&data->philos[i].state_mutex);
		i++;
	}
	pthread_mutex_lock(&data->stop_mutex);
	data->simulation_started = true;
	pthread_mutex_unlock(&data->stop_mutex);
	pthread_join(monitor, NULL);
	i = 0;
	while (i < data->num_philos)
	{
		pthread_join(data->philos[i].thread_id, NULL);
		i++;
	}
	return (0);
}

int	main(int argc, char **argv)
{
	t_data	data;

	if (parse_args(&data, argc, argv))
		return (1);
	if (data.must_eat_count == 0)
		return (0);
	if (init_data(&data))
	{
		printf("Error: initialization failed\n");
		return (1);
	}
	launch_simulation(&data);
	cleanup_data(&data);
	return (0);
}
