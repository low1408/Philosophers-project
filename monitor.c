#include "philo.h"

static bool	check_philosopher_death(t_data *data, int i, long long now)
{
	long long	last_meal;

	pthread_mutex_lock(&data->philos[i].state_mutex);
	last_meal = data->philos[i].last_meal_time;
	pthread_mutex_unlock(&data->philos[i].state_mutex);
	if ((now - last_meal) >= data->time_to_die)
	{
		pthread_mutex_lock(&data->print_mutex);
		pthread_mutex_lock(&data->stop_mutex);
		if (!data->simulation_stopped)
		{
			data->simulation_stopped = true;
			printf("%lld %d died\n",
				now - data->start_time,
				data->philos[i].id);
		}
		pthread_mutex_unlock(&data->stop_mutex);
		pthread_mutex_unlock(&data->print_mutex);
		return (true);
	}
	return (false);
}

static bool	check_all_ate(t_data *data)
{
	int	i;
	int	finished;
	int	meals;

	if (data->must_eat_count == -1)
		return (false);
	finished = 0;
	i = 0;
	while (i < data->num_philos)
	{
		pthread_mutex_lock(&data->philos[i].state_mutex);
		meals = data->philos[i].meals_eaten;
		pthread_mutex_unlock(&data->philos[i].state_mutex);
		if (meals >= data->must_eat_count)
			finished++;
		i++;
	}
	if (finished == data->num_philos)
	{
		pthread_mutex_lock(&data->stop_mutex);
		data->simulation_stopped = true;
		pthread_mutex_unlock(&data->stop_mutex);
		return (true);
	}
	return (false);
}

void	*monitor_routine(void *arg)
{
	t_data		*data;
	int			i;
	long long	now;

	data = (t_data *)arg;
	while (1)
	{
		if (has_simulation_stopped(data))
			break ;
		now = get_time_in_ms();
		i = 0;
		while (i < data->num_philos)
		{
			if (check_philosopher_death(data, i, now))
				return (NULL);
			i++;
		}
		if (check_all_ate(data))
			return (NULL);
		usleep(1000);
	}
	return (NULL);
}
