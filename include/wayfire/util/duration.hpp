#pragma once
#include <wayfire/config/option.hpp>
#include <wayfire/config/option-types.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace wf
{
namespace animation
{
namespace smoothing
{
/**
 * A smooth function is a function which takes a double in [0, 1] and returns another double in R. Both ranges
 * represent percentage of a progress of an animation.
 */
using smooth_function = std::function<double (double)>;

/** linear smoothing function, i.e x -> x */
extern smooth_function linear;
/** "circle" smoothing function, i.e x -> sqrt(2x - x*x) */
extern smooth_function circle;
/** "sigmoid" smoothing function, i.e x -> 1.0 / (1 + exp(-12 * x + 6)) */
extern smooth_function sigmoid;
/** custom cubic-bezier as in CSS */
extern smooth_function get_cubic_bezier(double x1, double y1, double x2, double y2);

std::vector<std::string> get_available_smooth_functions();
}
}

struct animation_description_t
{
    int length_ms;
    animation::smoothing::smooth_function easing;
    std::string easing_name;

    bool operator ==(const animation_description_t& other) const;
};

namespace platform
{

/**
 * Interface for platform-specific floating-point comparison.
 *
 * Machine epsilon (~2.2e-16) is too strict for most practical use.
 * Platform implementations may choose a wider relative tolerance that
 * reflects the actual precision needed for the domain (e.g., animation
 * easing control points).
 */
class IEpsilonComparison
{
  public:
    virtual ~IEpsilonComparison() = default;

    /**
     * Compare two doubles for approximate equality.
     * @param a First value.
     * @param b Second value.
     * @return true if |a - b| is within the platform tolerance.
     */
    virtual bool compare(double a, double b) const = 0;
};

/**
 * Interface for platform-specific clock/time source.
 *
 * The standard system_clock can be unstable under virtualization
 * (e.g., FreeBSD VMs with NTP adjustments). Platform implementations
 * may choose a more monotonic clock source for animation timing.
 */
class IClockSource
{
  public:
    virtual ~IClockSource() = default;

    /**
     * Platform-specific opaque time point type.
     */
    struct time_point_t
    {
        int64_t value;
        bool operator ==(const time_point_t& other) const
        {
            return value == other.value;
        }
    };

    /**
     * Get the current time point.
     */
    virtual time_point_t now() const = 0;

    /**
     * Return milliseconds elapsed since @p start.
     */
    virtual int64_t elapsed_ms(const time_point_t& start) const = 0;
};

/**
 * Factory for creating platform-specific implementations.
 *
 * Call once at startup; the returned objects are owned by the factory
 * and remain valid for the lifetime of the process.
 *
 * In tests, set WF_DURATION_USE_MOCK_CLOCK=1 to use a deterministic
 * mock clock that only advances when advance_mock_clock() is called.
 * This makes animation timing tests reliable and platform-independent.
 */
class PlatformFactory
{
  public:
    /**
     * The singleton epsilon comparison instance for this platform.
     */
    static const IEpsilonComparison& epsilon();

    /**
     * The singleton clock source instance for this platform.
     * In test mode (WF_DURATION_USE_MOCK_CLOCK=1), returns a
     * MockClockSource that only advances via advance_mock_clock().
     */
    static const IClockSource& clock();

    /**
     * Advance the mock clock by @p delta_ms milliseconds.
     * Only effective when clock() returns a MockClockSource.
     */
    static void advance_mock_clock(int64_t delta_ms);
};

} // namespace platform

namespace option_type
{
/**
 * Parse the string as an animation description.
 */
template<>
std::optional<animation_description_t> from_string<animation_description_t>(const std::string& value);

/**
 * Convert the given animation description to a string.
 */
template<>
std::string to_string<animation_description_t>(const animation_description_t& value);
}

namespace animation
{
/**
 * A transition from start to end.
 */
struct transition_t
{
    double start, end;
};

/**
 * duration_t is a class which can be used to track progress over a specific time interval.
 */
class duration_t
{
  public:
    /**
     * Construct a new duration. Initially, the duration is not running and its progress is 1.
     *
     * @param length The length of the duration in milliseconds.
     * @param smooth The smoothing function for transitions.
     */
    duration_t(std::shared_ptr<wf::config::option_t<int>> length = nullptr,
        smoothing::smooth_function smooth = smoothing::circle);

    duration_t(std::shared_ptr<wf::config::option_t<animation_description_t>> length);

    /* Copy-constructor */
    duration_t(const duration_t& other);
    /* Copy-assignment */
    duration_t& operator =(const duration_t& other);

    /* Move-constructor */
    duration_t(duration_t&& other) = default;
    /* Move-assignment */
    duration_t& operator =(duration_t&& other) = default;

    /**
     * Start the duration. This means that the progress will get reset to 0.
     */
    void start();

    /**
     * Get the progress of the duration in percentage. The progress will be smoothed using the smoothing
     * function.
     *
     * @return The current progress after smoothing. It is guaranteed that when the duration starts, progress
     * will be close to 0, and when it is finished, it will be close to 1.
     */
    double progress() const;

    /**
     * Check if the duration is still running. Note that even when the duration first finishes, this function
     * will still return that the function is running one time.
     *
     * @return Whether the duration still has not elapsed.
     */
    bool running();

    /**
     * Reverse the duration. The progress will remain the same but the direction will reverse toward the
     * opposite start or end point.
     */
    void reverse();

    /**
     * Get duration direction.
     *  0: reverse 1: forward
     */
    int get_direction();

    class impl;
    /** Implementation details. */
    std::shared_ptr<impl> priv;
};

/**
 * A timed transition is a transition between two states which happens over a period of time.
 *
 * During the transition, the current state is smoothly interpolated between start and end.
 */
struct timed_transition_t : public transition_t
{
    /**
     * Construct a new timed transition using the given duration to measure progress.
     *
     * @duration The duration to use for time measurement
     * @start The start state.
     * @end The end state.
     */
    timed_transition_t(const duration_t& duration,
        double start = 0, double end = 0);

    /**
     * Set the transition start to the current state and the end to the given
     * @new_end.
     */
    void restart_with_end(double new_end);

    /**
     * Set the transition start to the current state, and don't change the end.
     */
    void restart_same_end();

    /**
     * Set the transition start and end state.
     * @param start The start of the transition.
     * @param end The end of the transition.
     */
    void set(double start, double end);

    /**
     * Swap start and end values.
     */
    void flip();

    /**
     * Implicitly convert the transition to its current state.
     */
    operator double() const;

  private:
    std::shared_ptr<const duration_t::impl> duration;
};

class simple_animation_t : public duration_t, public timed_transition_t
{
  public:
    simple_animation_t(
        std::shared_ptr<wf::config::option_t<int>> length = nullptr,
        smoothing::smooth_function smooth = smoothing::circle);

    simple_animation_t(std::shared_ptr<wf::config::option_t<animation_description_t>> length);

    /**
     * Set the start and the end of the animation and start the duration.
     */
    void animate(double start, double end);

    /**
     * Animate from the current progress to the given end, and start the duration.
     */
    void animate(double end);

    /**
     * Animate from the current progress to the current end, and start the duration.
     */
    void animate();
};
}
}
