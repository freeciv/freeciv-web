/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "log.h"
#include "mem.h"
#include "support.h"

#include "fcthread.h"

#ifdef FREECIV_C11_THR

struct fc_thread_wrap_data {
  void *arg;
  void (*func)(void *arg);
};

/*******************************************************************//**
  Wrapper which fingerprint matches one required by pthread_create().
  Calls function which matches fingerprint required by fc_thread_start()
***********************************************************************/
static int fc_thread_wrapper(void *arg)
{
  struct fc_thread_wrap_data *data = (struct fc_thread_wrap_data *) arg;

  data->func(data->arg);

  free(data);

  return EXIT_SUCCESS;
}

/*******************************************************************//**
  Create new thread
***********************************************************************/
int fc_thread_start(fc_thread *thread, void (*function) (void *arg),
                    void *arg)
{
  int ret;

  /* Freed by child thread once it's finished with data */
  struct fc_thread_wrap_data *data = fc_malloc(sizeof(*data));

  data->arg = arg;
  data->func = function;

  ret = thrd_create(thread, &fc_thread_wrapper, data);

  return ret != thrd_success;
}

/*******************************************************************//**
  Wait for thread to finish
***********************************************************************/
void fc_thread_wait(fc_thread *thread)
{
  int *return_value = NULL;

  thrd_join(*thread, return_value);
}

/*******************************************************************//**
  Initialize mutex
***********************************************************************/
void fc_init_mutex(fc_mutex *mutex)
{
  mtx_init(mutex, mtx_plain|mtx_recursive);
}

/*******************************************************************//**
  Destroy mutex
***********************************************************************/
void fc_destroy_mutex(fc_mutex *mutex)
{
  mtx_destroy(mutex);
}

/*******************************************************************//**
  Lock mutex
***********************************************************************/
void fc_allocate_mutex(fc_mutex *mutex)
{
  mtx_lock(mutex);
}

/*******************************************************************//**
  Release mutex
***********************************************************************/
void fc_release_mutex(fc_mutex *mutex)
{
  mtx_unlock(mutex);
}

/*******************************************************************//**
  Initialize condition
***********************************************************************/
void fc_thread_cond_init(fc_thread_cond *cond)
{
  cnd_init(cond);
}

/*******************************************************************//**
  Destroy condition
***********************************************************************/
void fc_thread_cond_destroy(fc_thread_cond *cond)
{
  cnd_destroy(cond);
}

/*******************************************************************//**
  Wait for condition to be fulfilled
***********************************************************************/
void fc_thread_cond_wait(fc_thread_cond *cond, fc_mutex *mutex)
{
  cnd_wait(cond, mutex);
}

/*******************************************************************//**
  Signal other thread to continue on fulfilled condition
***********************************************************************/
void fc_thread_cond_signal(fc_thread_cond *cond)
{
  cnd_signal(cond);
}

#elif defined(FREECIV_HAVE_PTHREAD)

struct fc_thread_wrap_data {
  void *arg;
  void (*func)(void *arg);
};

/*******************************************************************//**
  Wrapper which fingerprint matches one required by pthread_create().
  Calls function which matches fingerprint required by fc_thread_start()
***********************************************************************/
static void *fc_thread_wrapper(void *arg)
{
  struct fc_thread_wrap_data *data = (struct fc_thread_wrap_data *) arg;

  data->func(data->arg);

  free(data);

  return NULL;
}

/*******************************************************************//**
  Create new thread
***********************************************************************/
int fc_thread_start(fc_thread *thread, void (*function) (void *arg),
                    void *arg)
{
  int ret;
  pthread_attr_t attr;

  /* Freed by child thread once it's finished with data */
  struct fc_thread_wrap_data *data = fc_malloc(sizeof(*data));

  data->arg = arg;
  data->func = function;

  /* Explicitly set thread as joinable to maximize portability
     between pthread implementations */
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  ret = pthread_create(thread, &attr, &fc_thread_wrapper, data);

  pthread_attr_destroy(&attr);

  return ret;
}

/*******************************************************************//**
  Wait for thread to finish
***********************************************************************/
void fc_thread_wait(fc_thread *thread)
{
  void **return_value = NULL;

  pthread_join(*thread, return_value);
}

/*******************************************************************//**
  Initialize mutex
***********************************************************************/
void fc_init_mutex(fc_mutex *mutex)
{
  pthread_mutexattr_t attr;

  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

  pthread_mutex_init(mutex, &attr);

  pthread_mutexattr_destroy(&attr);
}

/*******************************************************************//**
  Destroy mutex
***********************************************************************/
void fc_destroy_mutex(fc_mutex *mutex)
{
  pthread_mutex_destroy(mutex);
}

/*******************************************************************//**
  Lock mutex
***********************************************************************/
void fc_allocate_mutex(fc_mutex *mutex)
{
  pthread_mutex_lock(mutex);
}

/*******************************************************************//**
  Release mutex
***********************************************************************/
void fc_release_mutex(fc_mutex *mutex)
{
  pthread_mutex_unlock(mutex);
}

/*******************************************************************//**
  Initialize condition
***********************************************************************/
void fc_thread_cond_init(fc_thread_cond *cond)
{
  pthread_cond_init(cond, NULL);
}

/*******************************************************************//**
  Destroy condition
***********************************************************************/
void fc_thread_cond_destroy(fc_thread_cond *cond)
{
  pthread_cond_destroy(cond);
}

/*******************************************************************//**
  Wait for condition to be fulfilled
***********************************************************************/
void fc_thread_cond_wait(fc_thread_cond *cond, fc_mutex *mutex)
{
  pthread_cond_wait(cond, mutex);
}

/*******************************************************************//**
  Signal other thread to continue on fulfilled condition
***********************************************************************/
void fc_thread_cond_signal(fc_thread_cond *cond)
{
  pthread_cond_signal(cond);
}

#elif defined(FREECIV_HAVE_WINTHREADS)

struct fc_thread_wrap_data {
  void *arg;
  void (*func)(void *arg);
};

/*******************************************************************//**
  Wrapper which fingerprint matches one required by CreateThread().
  Calls function which matches fingerprint required by fc_thread_start()
***********************************************************************/
static DWORD WINAPI fc_thread_wrapper(LPVOID arg)
{
  struct fc_thread_wrap_data *data = (struct fc_thread_wrap_data *) arg;

  data->func(data->arg);

  free(data);

  return 0;
}

/*******************************************************************//**
  Create new thread
***********************************************************************/
int fc_thread_start(fc_thread *thread, void (*function) (void *arg), void *arg)
{
  /* Freed by child thread once it's finished with data */
  struct fc_thread_wrap_data *data = fc_malloc(sizeof(*data));

  data->arg = arg;
  data->func = function;

  *thread = CreateThread(NULL, 0, &fc_thread_wrapper, data, 0, NULL);

  if (*thread == NULL) {
    return 1;
  }

  return 0;
}

/*******************************************************************//**
  Wait for thread to finish
***********************************************************************/
void fc_thread_wait(fc_thread *thread)
{
  DWORD exit_code;

  GetExitCodeThread(*thread, &exit_code);

  while (exit_code == STILL_ACTIVE) {
    fc_usleep(1000);
    GetExitCodeThread(*thread, &exit_code);
  }

  CloseHandle(*thread);
}

/*******************************************************************//**
  Initialize mutex
***********************************************************************/
void fc_init_mutex(fc_mutex *mutex)
{
  *mutex = CreateMutex(NULL, FALSE, NULL);
}

/*******************************************************************//**
  Destroy mutex
***********************************************************************/
void fc_destroy_mutex(fc_mutex *mutex)
{
  CloseHandle(*mutex);
}

/*******************************************************************//**
  Lock mutex
***********************************************************************/
void fc_allocate_mutex(fc_mutex *mutex)
{
  WaitForSingleObject(*mutex, INFINITE);
}

/*******************************************************************//**
  Release mutex
***********************************************************************/
void fc_release_mutex(fc_mutex *mutex)
{
  ReleaseMutex(*mutex);
}

/* TODO: Windows thread condition variable support.
 *       Currently related functions are always dummy ones below
 *       (see #ifndef FREECIV_HAVE_THREAD_COND) */

#else /* No thread implementation */

#error "No working thread implementation"

#endif /* FREECIV_HAVE_PTHREAD || FREECIV_HAVE_WINTHREADS */


#ifndef FREECIV_HAVE_THREAD_COND

/* Dummy thread condition variable functions */

/*******************************************************************//**
  Dummy fc_thread_cond_init()
***********************************************************************/
void fc_thread_cond_init(fc_thread_cond *cond)
{}

/*******************************************************************//**
  Dummy fc_thread_cond_destroy()
***********************************************************************/
void fc_thread_cond_destroy(fc_thread_cond *cond)
{}

/*******************************************************************//**
  Dummy fc_thread_cond_wait()
***********************************************************************/
void fc_thread_cond_wait(fc_thread_cond *cond, fc_mutex *mutex)
{}

/*******************************************************************//**
  Dummy fc_thread_cond_signal()
***********************************************************************/
void fc_thread_cond_signal(fc_thread_cond *cond)
{}

#endif /* !FREECIV_HAVE_THREAD_COND */

/*******************************************************************//**
  Has freeciv thread condition variable implementation
***********************************************************************/
bool has_thread_cond_impl(void)
{
#ifdef FREECIV_HAVE_THREAD_COND
  return TRUE;
#else
  return FALSE;
#endif
}
