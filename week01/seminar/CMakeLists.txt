cmake_minimum_required(VERSION 3.28)
project(workshop1 C)

set(CMAKE_C_STANDARD 17)

add_executable(processes_basic processes/basic.c)

add_executable(processes_daemon processes/daemon.c)

add_executable(processes_zombie processes/zombie.c)

add_executable(processes_resources processes/resources.c)

add_executable(threads_api threads/thread.c)

add_executable(threads_signals threads/thread_signals.c)

add_executable(threads_cancel threads/thread_cancel.c)

add_executable(task_pi task/monte_carlo_pi.c)

add_executable(task_pi_solve task/monte_carlo_pi_solve.c)
