#ifndef PHILO_H
# define PHILO_H

# include <pthread.h>
# include <sys/time.h>
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <stdbool.h>
# include <limits.h>
# include <time.h>

typedef struct s_data	t_data;

typedef enum e_strategy
{
	STRATEGY_ORDERED,
	STRATEGY_BANKER,
	STRATEGY_CHANDY
}	t_strategy;

typedef struct s_philo
{
	int				id;
	int				meals_eaten;
	long long		last_meal_time;
	pthread_t		thread_id;
	int				left_fork_index;
	int				right_fork_index;
	pthread_mutex_t	*first_fork;
	pthread_mutex_t	*second_fork;
	pthread_mutex_t	state_mutex;
	t_data			*data;
}	t_philo;

struct s_data
{
	int				num_philos;
	long long		time_to_die;
	long long		time_to_eat;
	long long		time_to_sleep;
	int				must_eat_count;
	long long		start_time;
	bool			simulation_started;
	bool			simulation_stopped;
	t_strategy		strategy;
	pthread_mutex_t	stop_mutex;
	pthread_mutex_t	print_mutex;
	pthread_mutex_t	banker_mutex;
	pthread_cond_t	banker_cond;
	bool			*fork_available;
	bool			*banker_has_left;
	bool			*banker_has_right;
	pthread_mutex_t	chandy_mutex;
	pthread_cond_t	chandy_cond;
	int				*chandy_owner;
	int				*chandy_request;
	bool			*chandy_dirty;
	bool			*chandy_eating;
	pthread_mutex_t	*forks;
	t_philo			*philos;
};

/* main.c */
int			parse_args(t_data *data, int argc, char **argv);

/* init.c */
int			init_data(t_data *data);
void		cleanup_data(t_data *data);

/* utils.c */
long long	get_time_in_ms(void);
void		precise_usleep(long long milliseconds);
void		print_state(t_philo *philo, const char *status);
bool		has_simulation_stopped(t_data *data);
void		wait_for_simulation_start(t_data *data);

/* lifecycle.c */
void		*philosopher_routine(void *arg);

/* banker.c */
bool		banker_request_forks(t_philo *philo);
void		banker_release_forks(t_philo *philo);

/* chandy.c */
bool		chandy_request_forks(t_philo *philo);
void		chandy_release_forks(t_philo *philo);

/* monitor.c */
void		*monitor_routine(void *arg);

#endif
