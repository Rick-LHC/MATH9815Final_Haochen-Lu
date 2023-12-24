// Timer.hpp
// Author: Kai Zhang
//
// An adapter class to measure how long it takes for an operation to complete

#ifndef TIMER_HPP // Avoid multiple inclusion
#define TIMER_HPP
#include <chrono>

class Timer {
public: 
	Timer() {};
	void Start() { 
		start = std::chrono::system_clock::now(); 
	};
	void Stop() {
		end = std::chrono::system_clock::now();
		elapsed_seconds = end - start;
	};
	void Reset() { 
		elapsed_seconds = std::chrono::system_clock::duration::zero(); 
	}

	double GetTime() const {
		return elapsed_seconds.count(); // xx seconds 
	}
private: 
	Timer(const Timer &) = default;
	Timer & operator=(const Timer &) = default;
	std::chrono::time_point <std::chrono::system_clock> start;
	std::chrono::time_point <std::chrono::system_clock> end;
	std::chrono::duration<double> elapsed_seconds;
};


#endif // !TIMER_HPP