#pragma once

#include <mutex>
#include <optional>

template<typename T>
struct Mutex {
	struct Lock {
		~Lock();

		Lock(const Lock&) = delete;
		Lock& operator=(const Lock&) = delete;

		Lock(Lock&& other) noexcept;
		// Probably an error. Can't think of when would you use this.
		//Lock& operator=(Lock&& other) noexcept;

		T* operator->();
		T& operator*();
		const T& operator*() const;

		T& value;
	private:
		friend class Mutex;
		// mutex must be locked.
		Lock(T& value, std::mutex& mutex);

		// Not using a std::lock_guard or std::scoped lock because they dom't have a move constructors which are needed to use std::optional. Could use std::unique_lock but it takes up more space.
		std::mutex* mutex;
	};

	Lock lock();
	std::optional<Lock> tryLock();

private:
	T value;
	std::mutex mutex;
};

template<typename T>
Mutex<T>::Lock::Lock(T& value, std::mutex& mutex)
	: value(value)
	, mutex(&mutex) {}

template<typename T>
Mutex<T>::Lock::~Lock() {
	if (mutex != nullptr) {
		mutex->unlock();
	}
}

template<typename T>
Mutex<T>::Lock::Lock(Lock&& other) noexcept
	: value(other.value)
	, mutex(other.mutex) {
	other.mutex = nullptr;
}

//template<typename T>
//Mutex<T>::Lock& Mutex<T>::Lock::operator=(Lock&& other) noexcept {
// Doesn't set the value because current it's a reference.
//	mutex = other.mutex;
//	other.mutex = nullptr;
//	this->mutex->unlock();
//}

template<typename T>
T* Mutex<T>::Lock::operator->() {
	return &value;
}

template<typename T>
T& Mutex<T>::Lock::operator*() {
	return value;
}

template<typename T>
const T& Mutex<T>::Lock::operator*() const {
	return value;
}

template<typename T>
Mutex<T>::Lock Mutex<T>::lock() {
	mutex.lock();
	return Lock(value, mutex);
}

template<typename T>
std::optional<typename Mutex<T>::Lock> Mutex<T>::tryLock() {
	if (!mutex.try_lock()) {
		return std::nullopt;
	}
	return Lock(value, mutex);
}
