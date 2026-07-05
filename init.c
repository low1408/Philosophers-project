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

static int	init_banker(t_data *data)
{
	int	i;

	data->fork_available = malloc(sizeof(bool) * data->num_philos);
	if (!data->fork_available)
		return (1);
	data->banker_has_left = malloc(sizeof(bool) * data->num_philos);
	if (!data->banker_has_left)
	{
		free(data->fork_available);
		data->fork_available = NULL;
		return (1);
	}
	data->banker_has_right = malloc(sizeof(bool) * data->num_philos);
	if (!data->banker_has_right)
	{
		free(data->banker_has_left);
		free(data->fork_available);
		data->banker_has_left = NULL;
		data->fork_available = NULL;
		return (1);
	}
	i = 0;
	while (i < data->num_philos)
	{
		data->fork_available[i] = true;
		data->banker_has_left[i] = false;
		data->banker_has_right[i] = false;
		i++;
	}
	return (0);
}

static int	alloc_chandy(t_data *data)
{
	data->chandy_owner = malloc(sizeof(int) * data->num_philos);
	if (!data->chandy_owner)
		return (1);
	data->chandy_request = malloc(sizeof(int) * data->num_philos);
	if (!data->chandy_request)
		return (1);
	data->chandy_dirty = malloc(sizeof(bool) * data->num_philos);
	if (!data->chandy_dirty)
		return (1);
	data->chandy_eating = malloc(sizeof(bool) * data->num_philos);
	if (!data->chandy_eating)
		return (1);
	return (0);
}

static int	init_chandy(t_data *data)
{
	int	i;

	if (alloc_chandy(data))
		return (1);
	i = 0;
	while (i < data->num_philos)
	{
		if (i == 0)
			data->chandy_owner[i] = 0;
		else
			data->chandy_owner[i] = i - 1;
		data->chandy_request[i] = -1;
		data->chandy_dirty[i] = true;
		data->chandy_eating[i] = false;
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
		data->philos[i].left_fork_index = i;
		data->philos[i].right_fork_index = (i + 1) % data->num_philos;
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
	data->fork_available = NULL;
	data->banker_has_left = NULL;
	data->banker_has_right = NULL;
	data->chandy_owner = NULL;
	data->chandy_request = NULL;
	data->chandy_dirty = NULL;
	data->chandy_eating = NULL;
	if (pthread_mutex_init(&data->stop_mutex, NULL) != 0)
		return (1);
	if (pthread_mutex_init(&data->print_mutex, NULL) != 0)
	{
		pthread_mutex_destroy(&data->stop_mutex);
		return (1);
	}
	if (pthread_mutex_init(&data->banker_mutex, NULL) != 0)
	{
		pthread_mutex_destroy(&data->print_mutex);
		pthread_mutex_destroy(&data->stop_mutex);
		return (1);
	}
	if (pthread_cond_init(&data->banker_cond, NULL) != 0)
	{
		pthread_mutex_destroy(&data->banker_mutex);
		pthread_mutex_destroy(&data->print_mutex);
		pthread_mutex_destroy(&data->stop_mutex);
		return (1);
	}
	if (pthread_mutex_init(&data->chandy_mutex, NULL) != 0)
	{
		pthread_cond_destroy(&data->banker_cond);
		pthread_mutex_destroy(&data->banker_mutex);
		pthread_mutex_destroy(&data->print_mutex);
		pthread_mutex_destroy(&data->stop_mutex);
		return (1);
	}
	if (pthread_cond_init(&data->chandy_cond, NULL) != 0)
	{
		pthread_mutex_destroy(&data->chandy_mutex);
		pthread_cond_destroy(&data->banker_cond);
		pthread_mutex_destroy(&data->banker_mutex);
		pthread_mutex_destroy(&data->print_mutex);
		pthread_mutex_destroy(&data->stop_mutex);
		return (1);
	}
	if (init_forks(data))
	{
		pthread_cond_destroy(&data->chandy_cond);
		pthread_mutex_destroy(&data->chandy_mutex);
		pthread_cond_destroy(&data->banker_cond);
		pthread_mutex_destroy(&data->banker_mutex);
		pthread_mutex_destroy(&data->print_mutex);
		pthread_mutex_destroy(&data->stop_mutex);
		return (1);
	}
	if (init_banker(data))
	{
		cleanup_data(data);
		return (1);
	}
	if (init_chandy(data))
	{
		cleanup_data(data);
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
	if (data->fork_available)
		free(data->fork_available);
	if (data->banker_has_left)
		free(data->banker_has_left);
	if (data->banker_has_right)
		free(data->banker_has_right);
	if (data->chandy_owner)
		free(data->chandy_owner);
	if (data->chandy_request)
		free(data->chandy_request);
	if (data->chandy_dirty)
		free(data->chandy_dirty);
	if (data->chandy_eating)
		free(data->chandy_eating);
	pthread_cond_destroy(&data->chandy_cond);
	pthread_mutex_destroy(&data->chandy_mutex);
	pthread_cond_destroy(&data->banker_cond);
	pthread_mutex_destroy(&data->banker_mutex);
	pthread_mutex_destroy(&data->print_mutex);
	pthread_mutex_destroy(&data->stop_mutex);
}
