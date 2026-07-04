#include "philo.h"

static void	acquire_forks(t_philo *philo)
{
	pthread_mutex_lock(philo->first_fork);
	print_state(philo, "has taken a fork");
	pthread_mutex_lock(philo->second_fork);
	print_state(philo, "has taken a fork");
}

static void	release_forks(t_philo *philo)
{
	pthread_mutex_unlock(philo->second_fork);
	pthread_mutex_unlock(philo->first_fork);
}

static void	philo_eat(t_philo *philo)
{
	acquire_forks(philo);
	pthread_mutex_lock(&philo->state_mutex);
	philo->last_meal_time = get_time_in_ms();
	philo->meals_eaten++;
	pthread_mutex_unlock(&philo->state_mutex);
	print_state(philo, "is eating");
	precise_usleep(philo->data->time_to_eat);
	release_forks(philo);
}

static void	philo_think(t_philo *philo)
{
	long long	think_delay;

	print_state(philo, "is thinking");
	if (philo->data->num_philos % 2 != 0
		&& philo->data->time_to_die
		> (philo->data->time_to_eat + philo->data->time_to_sleep))
	{
		think_delay = (philo->data->time_to_die
				- (philo->data->time_to_eat + philo->data->time_to_sleep)) / 2;
		if (think_delay < 0)
			think_delay = 0;
		precise_usleep(think_delay);
	}
}

void	*philosopher_routine(void *arg)
{
	t_philo	*philo;
	t_data	*data;

	philo = (t_philo *)arg;
	data = philo->data;
	wait_for_simulation_start(data);
	if (has_simulation_stopped(data))
		return (NULL);
	if (data->num_philos == 1)
	{
		pthread_mutex_lock(philo->first_fork);
		print_state(philo, "has taken a fork");
		precise_usleep(data->time_to_die);
		pthread_mutex_unlock(philo->first_fork);
		return (NULL);
	}
	while (!has_simulation_stopped(data))
	{
		philo_eat(philo);
		print_state(philo, "is sleeping");
		precise_usleep(data->time_to_sleep);
		philo_think(philo);
	}
	return (NULL);
}
