#include "philo.h"

long long	get_time_in_ms(void)
{
	struct timeval	tv;

	if (gettimeofday(&tv, NULL) == -1)
		return (0);
	return ((tv.tv_sec * 1000LL) + (tv.tv_usec / 1000));
}

void	precise_usleep(long long milliseconds)
{
	long long	start;
	long long	elapsed;
	long long	remaining;

	start = get_time_in_ms();
	elapsed = 0;
	if (milliseconds > 10)
	{
		remaining = (milliseconds - 10) * 1000;
		while (remaining > 0)
		{
			if (remaining > 500000)
				usleep(500000);
			else
				usleep((useconds_t)remaining);
			remaining -= 500000;
		}
		elapsed = get_time_in_ms() - start;
	}
	while (elapsed < milliseconds)
	{
		usleep(100);
		elapsed = get_time_in_ms() - start;
	}
}

bool	has_simulation_stopped(t_data *data)
{
	bool	stopped;

	pthread_mutex_lock(&data->stop_mutex);
	stopped = data->simulation_stopped;
	pthread_mutex_unlock(&data->stop_mutex);
	return (stopped);
}

void	wait_for_simulation_start(t_data *data)
{
	pthread_mutex_lock(&data->stop_mutex);
	while (!data->simulation_started && !data->simulation_stopped)
	{
		pthread_mutex_unlock(&data->stop_mutex);
		usleep(50);
		pthread_mutex_lock(&data->stop_mutex);
	}
	pthread_mutex_unlock(&data->stop_mutex);
}

void	print_state(t_philo *philo, const char *status)
{
	pthread_mutex_lock(&philo->data->print_mutex);
	pthread_mutex_lock(&philo->data->stop_mutex);
	if (!philo->data->simulation_stopped)
	{
		printf("%lld %d %s\n",
			get_time_in_ms() - philo->data->start_time,
			philo->id,
			status);
	}
	pthread_mutex_unlock(&philo->data->stop_mutex);
	pthread_mutex_unlock(&philo->data->print_mutex);
}
