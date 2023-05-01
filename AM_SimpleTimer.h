#pragma once
#include <chrono>

class AM_SimpleTimer
{
public:
	static AM_SimpleTimer& GetInstance()
	{
		static AM_SimpleTimer instance;
		return instance;
	}

	std::chrono::steady_clock::time_point GetTime()
	{
		return std::chrono::high_resolution_clock::now();
	}
	std::chrono::steady_clock::time_point GetStartTime()
	{
		const static auto startTime = GetTime();
		return startTime;
	}
	float GetTimeElapsed()
	{
		return std::chrono::duration<float, std::chrono::seconds::period>(GetTime() - GetStartTime()).count();
	}

	float GetDeltaTime()
	{
		auto newTime = GetTime();
		const float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - myLastTime).count();
		myLastTime = newTime;
		return deltaTime;
	}

private:
	AM_SimpleTimer();
	~AM_SimpleTimer(){}
	AM_SimpleTimer(const AM_SimpleTimer&) = delete;
	AM_SimpleTimer(AM_SimpleTimer&&) = delete;
	AM_SimpleTimer& operator=(const AM_SimpleTimer&) = delete;
	AM_SimpleTimer& operator=(AM_SimpleTimer&&) = delete;

	std::chrono::steady_clock::time_point myLastTime;
};

