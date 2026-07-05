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

static int	get_requested_fork(t_philo *philo, bool request_left)
{
	if (request_left)
		return (philo->left_fork_index);
	return (philo->right_fork_index);
}

static bool	needs_available_forks(t_philo *philo, bool *available,
		bool *has_left, bool *has_right)
{
	int	i;

	i = philo->id - 1;
	if (!has_left[i] && !available[philo->left_fork_index])
		return (false);
	if (!has_right[i] && !available[philo->right_fork_index])
		return (false);
	return (true);
}

static void	release_all_simulated(t_philo *philo, bool *available)
{
	available[philo->left_fork_index] = true;
	available[philo->right_fork_index] = true;
}

static bool	finish_simulation(t_data *data, bool *available,
		bool *has_left, bool *has_right)
{
	int		i;
	int		finished_count;
	bool	progress;
	bool	*finished;

	finished = malloc(sizeof(bool) * data->num_philos);
	if (!finished)
		return (false);
	i = 0;
	while (i < data->num_philos)
		finished[i++] = false;
	finished_count = 0;
	progress = true;
	while (progress && finished_count < data->num_philos)
	{
		progress = false;
		i = 0;
		while (i < data->num_philos)
		{
			if (!finished[i] && needs_available_forks(&data->philos[i],
					available, has_left, has_right))
			{
				release_all_simulated(&data->philos[i], available);
				finished[i] = true;
				finished_count++;
				progress = true;
			}
			i++;
		}
	}
	free(finished);
	return (finished_count == data->num_philos);
}

static bool	copy_banker_state(t_data *data, bool **available,
		bool **has_left, bool **has_right)
{
	int	i;

	*available = malloc(sizeof(bool) * data->num_philos);
	*has_left = malloc(sizeof(bool) * data->num_philos);
	*has_right = malloc(sizeof(bool) * data->num_philos);
	if (!*available || !*has_left || !*has_right)
	{
		free(*available);
		free(*has_left);
		free(*has_right);
		return (false);
	}
	i = 0;
	while (i < data->num_philos)
	{
		(*available)[i] = data->fork_available[i];
		(*has_left)[i] = data->banker_has_left[i];
		(*has_right)[i] = data->banker_has_right[i];
		i++;
	}
	return (true);
}

static bool	is_safe_after_grant(t_data *data, t_philo *philo,
		bool request_left)
{
	bool	*available;
	bool	*has_left;
	bool	*has_right;
	bool	safe;
	int		fork_index;
	int		philo_index;

	if (!copy_banker_state(data, &available, &has_left, &has_right))
		return (false);
	fork_index = get_requested_fork(philo, request_left);
	philo_index = philo->id - 1;
	available[fork_index] = false;
	if (request_left)
		has_left[philo_index] = true;
	else
		has_right[philo_index] = true;
	safe = finish_simulation(data, available, has_left, has_right);
	free(available);
	free(has_left);
	free(has_right);
	return (safe);
}

static bool	already_has_fork(t_philo *philo, bool request_left)
{
	if (request_left)
		return (philo->data->banker_has_left[philo->id - 1]);
	return (philo->data->banker_has_right[philo->id - 1]);
}

static void	record_grant(t_philo *philo, bool request_left)
{
	t_data	*data;
	int		fork_index;

	data = philo->data;
	fork_index = get_requested_fork(philo, request_left);
	data->fork_available[fork_index] = false;
	if (request_left)
		data->banker_has_left[philo->id - 1] = true;
	else
		data->banker_has_right[philo->id - 1] = true;
}

static bool	banker_request_one_fork(t_philo *philo, bool request_left)
{
	t_data			*data;
	struct timespec	deadline;
	int				fork_index;

	data = philo->data;
	fork_index = get_requested_fork(philo, request_left);
	while (!has_simulation_stopped(data))
	{
		pthread_mutex_lock(&data->banker_mutex);
		if (already_has_fork(philo, request_left)
			|| (data->fork_available[fork_index]
				&& is_safe_after_grant(data, philo, request_left)))
		{
			if (!already_has_fork(philo, request_left))
				record_grant(philo, request_left);
			pthread_mutex_unlock(&data->banker_mutex);
			print_state(philo, "has taken a fork");
			return (true);
		}
		set_wait_deadline(&deadline);
		pthread_cond_timedwait(&data->banker_cond, &data->banker_mutex,
			&deadline);
		pthread_mutex_unlock(&data->banker_mutex);
	}
	return (false);
}

bool	banker_request_forks(t_philo *philo)
{
	if (!banker_request_one_fork(philo, true))
		return (false);
	if (!banker_request_one_fork(philo, false))
	{
		banker_release_forks(philo);
		return (false);
	}
	return (true);
}

void	banker_release_forks(t_philo *philo)
{
	t_data	*data;
	int		i;

	data = philo->data;
	i = philo->id - 1;
	pthread_mutex_lock(&data->banker_mutex);
	if (data->banker_has_left[i])
	{
		data->banker_has_left[i] = false;
		data->fork_available[philo->left_fork_index] = true;
	}
	if (data->banker_has_right[i])
	{
		data->banker_has_right[i] = false;
		data->fork_available[philo->right_fork_index] = true;
	}
	pthread_cond_broadcast(&data->banker_cond);
	pthread_mutex_unlock(&data->banker_mutex);
}
