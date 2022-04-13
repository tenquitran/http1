#ifndef _INTERVAL_TIMER_H
#define _INTERVAL_TIMER_H

#include <iostream>
#include <signal.h>
#include <time.h>


template <typename T>
class IntervalTimer
{
	struct TimerArgs
	{
	public:
		TimerArgs(IntervalTimer *pTimer, T* pArg)
			: m_pTimer(pTimer), m_pArg(pArg)
		{
		}
	
	public:
		IntervalTimer *m_pTimer    = {nullptr};
		
		T *m_pArg = {nullptr};
	};

public:
	IntervalTimer(T *pArg)
		: m_timerArgs(this, pArg)
	{
	}
	
	bool create()
	{
		m_timerArgs.m_pTimer = this;
		m_timerArgs.m_pArg   = m_timerArgs.m_pArg;
	
		// Timer expiration behaviour.
		sigevent sev = {};
		sev.sigev_notify = SIGEV_THREAD;
		sev.sigev_notify_function = &IntervalTimer::expiredHelper;
		sev.sigev_value.sival_ptr = (void *)&m_timerArgs;

		int res = timer_create(CLOCK_REALTIME, &sev, &m_timerId);

		if (0 != res)
		{
			perror("timer_create() failed");
		    return false;
		}
		
		return true;
	}
	
	bool set()
	{
		int delay = m_timerArgs.m_pArg->m_delaySeconds;
	
		itimerspec its = {};
		its.it_value.tv_sec    = delay;    /* initial delay */
		its.it_interval.tv_sec = delay;    /* interval */

		int res = timer_settime(m_timerId, 0, &its, nullptr);

		if (0 != res)
		{
			perror("timer_settime() failed");
		    return false;
		}
		
		return true;
	}

	bool disarmAndDelete()
	{
		int res = timer_delete(m_timerId);
	
		if (0 != res)
		{
			perror("timer_delete() failed");
			return false;
		}

		return true;
	}

private:
	// Redirector for the timer expiration action.
	static void expiredHelper(union sigval arg)
	{
		TimerArgs *pTa = (TimerArgs *)arg.sival_ptr;
	
		pTa->m_pTimer->expired(pTa->m_pArg);
	}

	// Action on timer expiration.
	virtual void expired(void* p)
	{
		T *pArg = (T *)p;
		//ReloadResponseArgs *pArg = (ReloadResponseArgs *)p;
		
		// TODO: temp
		std::cout << "Timer expired. Path: " << pArg->m_responseFilePath << std::endl;
		
		pArg->periodicAction();
		//loadPostResponse(pArg->m_responseFilePath);
	}

private:
	timer_t m_timerId = {};
	
	TimerArgs m_timerArgs;
};

#endif

