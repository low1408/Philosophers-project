#include "philo.h"

static void	set_wait_deadline(struct timespec *deadline)
{
	struct timeval	now;

	gettimeofday(&now, NULL);
	deadline->tv_sec = now.tv_sec;
	deadline->tv_nsec = (now.tv_usec + 1000) * 1000;
	if (deadline->tv_nsec >= 1000000000)
	{
		deadline->tv_sec++;
		deadline->tv_nsec -= 1000000000;
	}
}

static bool	owns_fork(t_data *data, t_philo *philo, int fork_index)
{
	return (data->chandy_owner[fork_index] == philo->id - 1);
}

static void	try_transfer_fork(t_data *data, int fork_index)
{
	int	owner;
	int	requester;

	owner = data->chandy_owner[fork_index];
	requester = data->chandy_request[fork_index];
	if (requester == -1 || requester == owner)
		return ;
	if (data->chandy_dirty[fork_index] && !data->chandy_eating[owner])
	{
		data->chandy_owner[fork_index] = requester;
		data->chandy_dirty[fork_index] = false;
		data->chandy_request[fork_index] = -1;
		pthread_cond_broadcast(&data->chandy_cond);
	}
}

static bool	request_one_fork(t_philo *philo, int fork_index)
{
	t_data			*data;
	struct timespec	deadline;

	data = philo->data;
	while (!has_simulation_stopped(data))
	{
		pthread_mutex_lock(&data->chandy_mutex);
		if (owns_fork(data, philo, fork_index))
		{
			pthread_mutex_unlock(&data->chandy_mutex);
			return (true);
		}
		data->chandy_request[fork_index] = philo->id - 1;
		try_transfer_fork(data, fork_index);
		if (owns_fork(data, philo, fork_index))
		{
			pthread_mutex_unlock(&data->chandy_mutex);
			return (true);
		}
		set_wait_deadline(&deadline);
		pthread_cond_timedwait(&data->chandy_cond, &data->chandy_mutex,
			&deadline);
		pthread_mutex_unlock(&data->chandy_mutex);
	}
	return (false);
}

static bool	has_both_forks(t_philo *philo)
{
	t_data	*data;
	bool	has_both;

	data = philo->data;
	pthread_mutex_lock(&data->chandy_mutex);
	has_both = (owns_fork(data, philo, philo->left_fork_index)
			&& owns_fork(data, philo, philo->right_fork_index));
	if (has_both)
		data->chandy_eating[philo->id - 1] = true;
	pthread_mutex_unlock(&data->chandy_mutex);
	return (has_both);
}

bool	chandy_request_forks(t_philo *philo)
{
	while (!has_simulation_stopped(philo->data))
	{
		if (!request_one_fork(philo, philo->left_fork_index))
			return (false);
		if (!request_one_fork(philo, philo->right_fork_index))
			return (false);
		if (has_both_forks(philo))
		{
			print_state(philo, "has taken a fork");
			print_state(philo, "has taken a fork");
			return (true);
		}
	}
	return (false);
}

void	chandy_release_forks(t_philo *philo)
{
	t_data	*data;
	int		i;

	data = philo->data;
	i = philo->id - 1;
	pthread_mutex_lock(&data->chandy_mutex);
	data->chandy_eating[i] = false;
	if (owns_fork(data, philo, philo->left_fork_index))
		data->chandy_dirty[philo->left_fork_index] = true;
	if (owns_fork(data, philo, philo->right_fork_index))
		data->chandy_dirty[philo->right_fork_index] = true;
	try_transfer_fork(data, philo->left_fork_index);
	try_transfer_fork(data, philo->right_fork_index);
	pthread_cond_broadcast(&data->chandy_cond);
	pthread_mutex_unlock(&data->chandy_mutex);
}
