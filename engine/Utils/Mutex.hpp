#pragma once

#include <mutex>

template<typename T>
struct Mutex {
	struct Lock {
		Lock(T& value, std::mutex& mutex);

		T* operator->();
		T& operator*();
		const T& operator*() const;

		T& value;
	private:
		std::lock_guard<std::mutex> lock;
	};

	Lock lock();

private:
	T value;
	std::mutex mutex;
};