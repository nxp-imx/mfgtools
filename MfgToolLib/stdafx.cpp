// stdafx.cpp : source file that includes just the standard includes
// MfgToolLib.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

int InitEvent(myevent **Ev){
	int ret = 0;
	*Ev = new myevent();
	(*Ev)->mutex = new pthread_mutex_t;
	ret += pthread_mutex_init((*Ev)->mutex, NULL);
	if (ret != 0){
		//pthread_mutex_destroy((*Ev)->mutex);
		delete *Ev;
		*Ev = 0;
		return ret;
	}
	(*Ev)->cond = new pthread_cond_t;
	ret += pthread_cond_init((*Ev)->cond, NULL);
	if (ret != 0){
		//pthread_mutex_destroy((*Ev)->mutex);
		//pthread_cond_destroy((*Ev)->cond);
		delete *Ev;
		*Ev = 0;
	}
	return ret;
}
void SetEvent(myevent *Ev, sem_t*sem_att) {
	pthread_mutex_lock(Ev->mutex);
	Ev->triggered = true;
	pthread_cond_signal(Ev->cond);
	pthread_mutex_unlock(Ev->mutex);
	if (sem_att != NULL){
	sem_post(sem_att);
	}
}
void ClearEvent(myevent *Ev) {
	pthread_mutex_lock(Ev->mutex);
	Ev->triggered = false;
	pthread_mutex_unlock(Ev->mutex);
}
bool CheckEvent(myevent *Ev){
	bool temp;
	pthread_mutex_lock(Ev->mutex);
	temp = Ev->triggered;
	pthread_mutex_unlock(Ev->mutex);
	return temp;
}

void WaitOnEvent(myevent *Ev) {
	pthread_mutex_lock(Ev->mutex);
	while (!Ev->triggered)
		pthread_cond_wait(Ev->cond, Ev->mutex);
	pthread_mutex_unlock(Ev->mutex);
	return;
}

int DestroyEvent(myevent * Ev){
	int ret = 0;
	ret += pthread_mutex_destroy(Ev->mutex);
	if (ret != 0)return ret;
	ret += pthread_cond_destroy(Ev->cond);
	delete Ev;
	return ret;
}
int CheckArrayOfEvents(myevent *container[], int length){
	for (int i = 0; i < length; i++){
		if (CheckEvent(container[i])){
			return i;
		}
	}
	return -1;
}

#ifndef __linux__
int gettimeofday(struct timeval * tv){
	FILETIME ft;
	unsigned __int64 tmpres;
	GetSystemTimeAsFileTime(&ft);

	tmpres = ft.dwHighDateTime;
	tmpres <<= 32;
	tmpres |= ft.dwLowDateTime;

	/*converting file time to unix epoch*/
	tmpres -= DELTA_EPOCH_IN_MICROSECS;
	tmpres /= 10;  /*convert into microseconds*/
	tv->tv_sec = (long)(tmpres / 1000000UL);
	tv->tv_usec = (long)(tmpres % 1000000UL);
	return 0;
}

#endif
