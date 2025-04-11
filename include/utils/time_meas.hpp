#ifndef DRONE_NAVIGATION_TIME_MEAS_HPP
#define DRONE_NAVIGATION_TIME_MEAS_HPP

#include <chrono>
#include <atomic> // For std::atomic_thread_fence and std::memory_order_seq_cst.


/**
 * Getting a current time and using a thread fence (a full fence or memory barriers)
 * to prevents the reordering of two arbitrary operations. But that guarantee will not hold for
 * StoreLoad operations. They can be reordered.
 *
 * @return Current time.
 */
inline std::chrono::high_resolution_clock::time_point get_current_time_fenced() {
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto res_time = std::chrono::high_resolution_clock::now();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return res_time;
}

/**
 * Converting a duration to a duration measured in milliseconds.
 *
 * @tparam D Class template std::chrono::duration representing a time interval.
 * @param d Duration between 2 time points.
 * @return Duration measured in milliseconds.
 */
template <class D>
inline long long to_ms(const D& d) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
}

/**
 * Converting a duration to a duration measured in microseconds.
 *
 * @tparam D Class template std::chrono::duration representing a time interval.
 * @param d Duration between 2 time points.
 * @return Duration measured in milliseconds.
 */
template <class D>
inline long long to_mcs(const D& d) {
    return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
}

/**
 * Converting a duration to a duration measured in nanoseconds.
 *
 * @tparam D Class template std::chrono::duration representing a time interval.
 * @param d Duration between 2 time points.
 * @return Duration measured in nanoseconds.
 */
template <class D>
inline long long to_ns(const D& d) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
}

#endif //DRONE_NAVIGATION_TIME_MEAS_HPP
