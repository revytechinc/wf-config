#include <wayfire/util/duration.hpp>
#include <wayfire/util/log.hpp>
#include <wayfire/config/types.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <map>
#include <sstream>

// ---------------------------------------------------------------------------
// Platform implementations
// ---------------------------------------------------------------------------

namespace wf
{
namespace platform
{

/** Linux: strict machine epsilon. */
class LinuxEpsilonComparison : public IEpsilonComparison
{
  public:
    bool compare(double a, double b) const override
    {
        return std::fabs(a - b) <= std::numeric_limits<double>::epsilon() * std::fabs(a + b);
    }
};

/** FreeBSD: wider tolerance suitable for animation control points. */
class FreeBSDEpsilonComparison : public IEpsilonComparison
{
    // 1e-12 captures differences that matter for easing curves while
    // treating genuinely close values (e.g. 1.0 and 1+1e-14) as equal.
    static constexpr double TOLERANCE = 1e-12;

  public:
    bool compare(double a, double b) const override
    {
        return std::fabs(a - b) <= TOLERANCE;
    }
};

/** Linux: system_clock — fine on bare metal, can drift in VMs. */
class LinuxClockSource : public IClockSource
{
  public:
    time_point_t now() const override
    {
        using namespace std::chrono;
        return {duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()};
    }

    int64_t elapsed_ms(const time_point_t& start) const override
    {
        using namespace std::chrono;
        auto now_pt = system_clock::now().time_since_epoch();
        return duration_cast<milliseconds>(now_pt).count() - start.value;
    }
};

/** FreeBSD: steady_clock — monotonic, unaffected by NTP/vm-clock jitter. */
class FreeBSDClockSource : public IClockSource
{
  public:
    time_point_t now() const override
    {
        using namespace std::chrono;
        return {duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()};
    }

    int64_t elapsed_ms(const time_point_t& start) const override
    {
        using namespace std::chrono;
        auto now_pt = steady_clock::now().time_since_epoch();
        return duration_cast<milliseconds>(now_pt).count() - start.value;
    }
};

/** Deterministic clock for tests — only advances via advance_mock_clock(). */
class MockClockSource : public IClockSource
{
  public:
    // Start at a large epoch value so is_ready() returns true before start().
    // This mirrors real-clock behavior where elapsed time is always huge.
    mutable int64_t current_ms = INT64_MAX - 1000000;

    time_point_t now() const override
    {
        return {current_ms};
    }

    int64_t elapsed_ms(const time_point_t& start) const override
    {
        return current_ms - start.value;
    }
};

static MockClockSource& mock_clock()
{
    static MockClockSource instance;
    return instance;
}

// Factory — the single #ifdef lives here, nowhere else.
const IEpsilonComparison& PlatformFactory::epsilon()
{
#ifdef __FreeBSD__
    static const FreeBSDEpsilonComparison instance;
#else
    static const LinuxEpsilonComparison instance;
#endif
    return instance;
}

const IClockSource& PlatformFactory::clock()
{
    static const bool use_mock =
        (std::getenv("WF_DURATION_USE_MOCK_CLOCK") != nullptr);
    if (use_mock)
    {
        return mock_clock();
    }
#ifdef __FreeBSD__
    static const FreeBSDClockSource real_instance;
    return real_instance;
#else
    static const LinuxClockSource real_instance;
    return real_instance;
#endif
}

void PlatformFactory::advance_mock_clock(int64_t delta_ms)
{
    mock_clock().current_ms += delta_ms;
}

} // namespace platform
} // namespace wf

// ---------------------------------------------------------------------------
// Easing helpers (platform-agnostic)
// ---------------------------------------------------------------------------

static double bezier_helper(double t, double p0, double p1, double p2, double p3)
{
    const double u = 1 - t;
    return u * u * u * p0 + 3 * u * u * t * p1 + 3 * u * t * t * p2 + t * t * t * p3;
}

namespace wf
{
namespace animation
{
namespace smoothing
{
smooth_function linear =
    [] (double x) -> double { return x; };
smooth_function circle =
    [] (double x) -> double { return std::sqrt(2 * x - x * x); };

const double sigmoid_max = 1 + std::exp(-6);
smooth_function sigmoid  =
    [] (double x) -> double { return sigmoid_max / (1 + exp(-12 * x + 6)); };

smooth_function get_cubic_bezier(double x1, double y1, double x2, double y2)
{
    // https://en.wikipedia.org/wiki/Newton%27s_method
    return [=] (double x)
    {
        double t = x;
        for (int i = 0; i < 10; ++i)
        {
            const double f  = bezier_helper(t, 0, x1, x2, 1) - x;
            const double df = 3 * (1 - t) * (1 - t) * x1 + 6 * (1 - t) * t * (x2 - x1) + 3 * t * t * (1 - x2);
            if (std::abs(f) < 1e-6)
            {
                break;
            }

            t -= f / df;
        }

        return bezier_helper(t, 0, y1, y2, 1);
    };
}
}
} // namespace animation
}

bool wf::animation_description_t::operator ==(const animation_description_t & other) const
{
    if (easing_name == other.easing_name)
    {
        return (length_ms == other.length_ms);
    }

    // Cubic-bezier easings need parsing to handle epsilon
    std::stringstream easing_a(easing_name);
    std::stringstream easing_b(other.easing_name);
    std::string easing_type_a, easing_type_b;
    easing_a >> easing_type_a;
    easing_b >> easing_type_b;
    if ((easing_type_a != "cubic-bezier") || (easing_type_b != "cubic-bezier"))
    {
        return false;
    }

    double x1_a, y1_a, x2_a, y2_a, x1_b, y1_b, x2_b, y2_b;
    easing_a >> x1_a >> y1_a >> x2_a >> y2_a;
    easing_b >> x1_b >> y1_b >> x2_b >> y2_b;
    const auto& eps = platform::PlatformFactory::epsilon();
    return eps.compare(x1_a, x1_b) &&
           eps.compare(y1_a, y1_b) &&
           eps.compare(x2_a, x2_b) &&
           eps.compare(y2_a, y2_b);
}

class wf::animation::duration_t::impl
{
  public:
    platform::IClockSource::time_point_t start_point{0};

    std::shared_ptr<wf::config::option_t<int>> length;
    std::shared_ptr<wf::config::option_t<animation_description_t>> descr;

    smoothing::smooth_function smooth_function;
    bool is_running = false;
    bool reverse    = false;

    int64_t get_elapsed() const
    {
        return platform::PlatformFactory::clock().elapsed_ms(start_point);
    }

    int get_duration() const
    {
        if (descr)
        {
            return std::max(1, descr->get_value().length_ms);
        }

        if (length)
        {
            return std::max(1, length->get_value());
        }

        LOGD("Calling methods on wf::animation::duration_t without a length");
        return 1;
    }

    bool is_ready() const
    {
        return get_elapsed() >= get_duration();
    }

    double get_progress_percentage() const
    {
        if ((!length && !descr) || is_ready())
        {
            return 1.0;
        }

        auto progress = 1.0 * get_elapsed() / get_duration();
        if (reverse)
        {
            progress = 1.0 - progress;
        }

        return std::clamp(progress, 0.0, 1.0);
    }

    double progress() const
    {
        if (is_ready())
        {
            return reverse ? 0.0 : 1.0;
        }

        if (descr)
        {
            return descr->get_value().easing(get_progress_percentage());
        } else
        {
            return smooth_function(get_progress_percentage());
        }
    }
};

wf::animation::duration_t::duration_t(
    std::shared_ptr<wf::config::option_t<int>> length,
    smoothing::smooth_function smooth)
{
    this->priv = std::make_shared<impl>();
    this->priv->length = length;
    this->priv->smooth_function = smooth;
}

wf::animation::duration_t::duration_t(
    std::shared_ptr<wf::config::option_t<animation_description_t>> length)
{
    this->priv = std::make_shared<impl>();
    this->priv->descr = length;
}

wf::animation::duration_t::duration_t(const duration_t& other)
{
    this->priv = std::make_shared<impl>(*other.priv);
}

wf::animation::duration_t& wf::animation::duration_t::operator =(
    const duration_t& other)
{
    if (&other != this)
    {
        this->priv = std::make_shared<impl>(*other.priv);
    }

    return *this;
}

void wf::animation::duration_t::start()
{
    this->priv->is_running  = 1;
    this->priv->start_point = platform::PlatformFactory::clock().now();
}

double wf::animation::duration_t::progress() const
{
    return this->priv->progress();
}

bool wf::animation::duration_t::running()
{
    if (this->priv->is_ready())
    {
        bool was_running = this->priv->is_running;
        this->priv->is_running = false;
        return was_running;
    }

    return true;
}

void wf::animation::duration_t::reverse()
{
    auto total_duration = this->priv->get_duration();
    auto elapsed   = std::min(this->priv->get_elapsed(), (int64_t)total_duration);
    this->priv->start_point = platform::PlatformFactory::clock().now();
    this->priv->start_point.value -= (total_duration - elapsed);
    this->priv->reverse     = !this->priv->reverse;
}

int wf::animation::duration_t::get_direction()
{
    return !this->priv->reverse;
}

wf::animation::timed_transition_t::timed_transition_t(
    const duration_t& dur, double start, double end) : duration(dur.priv)
{
    this->set(start, end);
}

void wf::animation::timed_transition_t::restart_with_end(double new_end)
{
    this->start = (double)*this;
    this->end   = new_end;
}

void wf::animation::timed_transition_t::restart_same_end()
{
    this->start = (double)*this;
}

void wf::animation::timed_transition_t::set(double start, double end)
{
    this->start = start;
    this->end   = end;
}

void wf::animation::timed_transition_t::flip()
{
    std::swap(this->start, this->end);
}

wf::animation::timed_transition_t::operator double() const
{
    double alpha = this->duration->progress();
    return (1 - alpha) * start + alpha * end;
}

wf::animation::simple_animation_t::simple_animation_t(
    std::shared_ptr<wf::config::option_t<int>> length,
    smoothing::smooth_function smooth) :
    duration_t(length, smooth),
    timed_transition_t((duration_t&)*this)
{}

wf::animation::simple_animation_t::simple_animation_t(
    std::shared_ptr<wf::config::option_t<animation_description_t>> length) :
    duration_t(length), timed_transition_t((duration_t&)*this)
{}

void wf::animation::simple_animation_t::animate(double start, double end)
{
    this->set(start, end);
    this->duration_t::start();
}

void wf::animation::simple_animation_t::animate(double end)
{
    this->restart_with_end(end);
    this->duration_t::start();
}

void wf::animation::simple_animation_t::animate()
{
    this->restart_same_end();
    this->duration_t::start();
}

namespace wf
{
namespace animation
{
namespace smoothing
{
// Thanks https://github.com/MrRobinOfficial/EasingFunctions
smooth_function ease_out_elastic = [] (double x) -> double
{
    float d = 1.0f;
    float p = d * 0.6f;
    float s;
    float a = 0;

    if (x == 0)
    {
        return 0;
    }

    if ((x /= d) == 1)
    {
        return 1;
    }

    if ((a == 0.0f) || (a < std::abs(1.0)))
    {
        a = 1.0;
        s = p * 0.25f;
    } else
    {
        s = p / (2 * std::acos(-1)) * std::asin(1.0 / a);
    }

    return (a * std::pow(2, -10 * x) * std::sin((x * d - s) * (2 * std::acos(-1)) / p) + 1.0);
};

static const std::map<std::string, animation::smoothing::smooth_function> easing_map = {
    {"linear", animation::smoothing::linear},
    {"circle", animation::smoothing::circle},
    {"sigmoid", animation::smoothing::sigmoid},
    {"easeOutElastic", animation::smoothing::ease_out_elastic},
};

std::vector<std::string> get_available_smooth_functions()
{
    std::vector<std::string> result;
    for (auto& func : easing_map)
    {
        result.push_back(func.first);
    }

    return result;
}
} // namespace smoothing
}

namespace option_type
{
template<>
std::optional<animation_description_t> from_string<animation_description_t>(const std::string& value)
{
    // Format 1: N (backwards compatible fallback)
    if (auto val = from_string<int>(value))
    {
        return animation_description_t{
            .length_ms = *val,
            .easing    = animation::smoothing::circle,
            .easing_name = "circle"
        };
    }

    // Format 2: N <s|ms> <easing>
    std::istringstream stream(value);
    double N;
    std::string suffix, easing;
    if (!(stream >> N >> suffix))
    {
        return {};
    }

    animation_description_t result;
    if ((suffix != "ms") && (suffix != "s"))
    {
        return {};
    }

    if (!(stream >> result.easing_name))
    {
        result.easing_name = "circle";
    }

    if (animation::smoothing::easing_map.count(result.easing_name))
    {
        result.easing = animation::smoothing::easing_map.at(result.easing_name);
    } else if (result.easing_name == "cubic-bezier")
    {
        double x1 = 0, y1 = 0, x2 = 1, y2 = 1;
        stream >> x1 >> y1 >> x2 >> y2;
        result.easing = animation::smoothing::get_cubic_bezier(x1, y1, x2, y2);
        result.easing_name = "cubic-bezier " +
            to_string(x1) +
            " " + to_string(y1) +
            " " + to_string(x2) +
            " " + to_string(y2);
    } else
    {
        return {};
    }

    std::string tmp;
    if (stream >> tmp)
    {
        // Trailing data
        return {};
    }

    if (suffix == "s")
    {
        result.length_ms = N * 1000;
    } else
    {
        result.length_ms = N;
    }

    return result;
}

template<>
std::string to_string<animation_description_t>(const animation_description_t& value)
{
    return to_string(value.length_ms) + "ms " + to_string(value.easing_name);
}
}
}
