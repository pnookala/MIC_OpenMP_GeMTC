/*
 * Sleep.h
 *
 *  Created on: Apr 22, 2015
 *      Author: pnookala
 */
#ifndef __SLEEP_H
#define __SLEEP_H

#ifndef MIC_DEV
#define MIC_DEV 0
#endif

__attribute__ ((target(mic))) void Sleep(void *params, int num_threads, int task_id);
//void Sleep(void *sleepTime, int num_threads, int task_id);

#endif
