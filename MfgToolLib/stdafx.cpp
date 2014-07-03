// stdafx.cpp : source file that includes just the standard includes
// MfgToolLib.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

int InitEvent(myevent *Ev){
	int ret = 0;
	ret += pthread_mutex_init(&Ev->mutex, 0);
	if (ret != 0)return ret;
	ret += pthread_cond_init(&Ev->cond, 0);
	Ev->triggered = false;
	return ret;
}
void SetEvent(myevent *Ev) {
	pthread_mutex_lock(&Ev->mutex);
	Ev->triggered = true;
	pthread_cond_signal(&Ev->cond);
	pthread_mutex_unlock(&Ev->mutex);
}
void ClearEvent(myevent *Ev) {
	pthread_mutex_lock(&Ev->mutex);
	Ev->triggered = false;
	pthread_mutex_unlock(&Ev->mutex);
}
void WaitOnEvent(myevent *Ev) {
	pthread_mutex_lock(&Ev->mutex);
	while (!Ev->triggered)
		pthread_cond_wait(&Ev->cond, &Ev->mutex);
	pthread_mutex_unlock(&Ev->mutex);
}

