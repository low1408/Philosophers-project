#include "philo.h"

static int	init_forks(t_data *data)
{
	int	i;

	data->forks = malloc(sizeof(pthread_mutex_t) * data->num_philos);
	if (!data->forks)
		return (1);
	i = 0;
	while (i < data->num_philos)
	{
		if (pthread_mutex_init(&data->forks[i], NULL) != 0)
		{
			while (--i >= 0)
				pthread_mutex_destroy(&data->forks[i]);
			free(data->forks);
			data->forks = NULL;
			return (1);
		}
		i++;
	}
	return (0);
}

static void	assign_forks(t_philo *philo, pthread_mutex_t *left,
		pthread_mutex_t *right)
{
	if (left < right)
	{
		philo->first_fork = left;
		philo->second_fork = right;
	}
	else
	{
		philo->first_fork = right;
		philo->second_fork = left;
	}
}

static int	init_philos(t_data *data)
{
	int				i;
	pthread_mutex_t	*left;
	pthread_mutex_t	*right;

	data->philos = malloc(sizeof(t_philo) * data->num_philos);
	if (!data->philos)
		return (1);
	i = 0;
	while (i < data->num_philos)
	{
		data->philos[i].id = i + 1;
		data->philos[i].meals_eaten = 0;
		data->philos[i].last_meal_time = 0;
		data->philos[i].data = data;
		if (pthread_mutex_init(&data->philos[i].state_mutex, NULL) != 0)
		{
			while (--i >= 0)
				pthread_mutex_destroy(&data->philos[i].state_mutex);
			free(data->philos);
			data->philos = NULL;
			return (1);
		}
		left = &data->forks[i];
		right = &data->forks[(i + 1) % data->num_philos];
		assign_forks(&data->philos[i], left, right);
		i++;
	}
	return (0);
}

int	init_data(t_data *data)
{
	data->simulation_started = false;
	data->simulation_stopped = false;
	data->forks = NULL;
	data->philos = NULL;
	if (pthread_mutex_init(&data->stop_mutex, NULL) != 0)
		return (1);
	if (pthread_mutex_init(&data->print_mutex, NULL) != 0)
	{
		pthread_mutex_destroy(&data->stop_mutex);
		return (1);
	}
	if (init_forks(data))
	{
		pthread_mutex_destroy(&data->print_mutex);
		pthread_mutex_destroy(&data->stop_mutex);
		return (1);
	}
	if (init_philos(data))
	{
		cleanup_data(data);
		return (1);
	}
	return (0);
}

void	cleanup_data(t_data *data)
{
	int	i;

	if (data->philos)
	{
		i = 0;
		while (i < data->num_philos)
		{
			pthread_mutex_destroy(&data->philos[i].state_mutex);
			i++;
		}
		free(data->philos);
	}
	if (data->forks)
	{
		i = 0;
		while (i < data->num_philos)
		{
			pthread_mutex_destroy(&data->forks[i]);
			i++;
		}
		free(data->forks);
	}
	pthread_mutex_destroy(&data->print_mutex);
	pthread_mutex_destroy(&data->stop_mutex);
}
