// stdafx.cpp : source file that includes just the standard includes
// MfgToolLib.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

int InitEvent(myevent *Ev){
	int ret = 0;
	ret += pthread_mutex_init(&Ev->mutex, 0);
	if (ret != 0)return ret;
	ret += pthread_cond_init(&Ev->cond, 0);
	*Ev->triggered = new bool;
	return ret;
}
void SetEvent(myevent *Ev, sem_t*sem_att = NULL) {
	pthread_mutex_lock(&Ev->mutex);
	*Ev->triggered = true;
	pthread_cond_signal(&Ev->cond);
	pthread_mutex_unlock(&Ev->mutex);
	sem_post(sem_att);
}
void ClearEvent(myevent *Ev) {
	pthread_mutex_lock(&Ev->mutex);
	*Ev->triggered = false;
	pthread_mutex_unlock(&Ev->mutex);
}
bool CheckEvent(myevent *Ev){
	bool temp;
	pthread_mutex_lock(&Ev->mutex);
	temp = *Ev->triggered;
	pthread_mutex_unlock(&Ev->mutex);
	return temp;
}

void WaitOnEvent(myevent *Ev) {
	pthread_mutex_lock(&Ev->mutex);
	while (!*Ev->triggered)
		pthread_cond_wait(&Ev->cond, &Ev->mutex);
	pthread_mutex_unlock(&Ev->mutex);
}

int DestroyEvent(myevent * Ev){
	int ret = 0;
	ret += pthread_mutex_destroy(&Ev->mutex);
	if (ret != 0)return ret;
	ret += pthread_cond_destroy(&Ev->cond);
	delete Ev->triggered;
	return ret;
}
