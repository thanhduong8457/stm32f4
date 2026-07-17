#include <cstdio>

#include "app/pid_controller.hpp"

namespace
{

int failures = 0;

void expectEqual(int32_t actual, int32_t expected, const char *description)
{
    if (actual != expected)
    {
        std::fprintf(stderr, "FAIL: %s (actual=%ld expected=%ld)\n",
                     description, static_cast<long>(actual),
                     static_cast<long>(expected));
        ++failures;
    }
}

app::PidController controller(app::PidGains gains)
{
    app::PidController result;
    result.configure(gains);
    result.setOutputLimits(0, 1000);
    result.setControlPeriod(100);
    return result;
}

} // namespace

int main()
{
    {
        app::PidController pid = controller(app::PidGains{1000, 0, 0});
        expectEqual(pid.update(200, 100), 100,
                    "proportional RPM error becomes PWM correction");
    }

    {
        app::PidController pid = controller(app::PidGains{});
        pid.configureFeedForward(1000, 10000);
        expectEqual(pid.update(5000, 5000), 500,
                    "feed-forward maps target RPM to baseline duty");
        expectEqual(pid.update(10000, 10000), 1000,
                    "feed-forward reaches configured max duty");
    }

    {
        app::PidController pid = controller(app::PidGains{0, 1000, 0});
        expectEqual(pid.update(100, 0), 10,
                    "integral uses the configured control period");
        expectEqual(pid.update(100, 0), 20, "integral accumulates RPM error");
    }

    {
        app::PidController pid = controller(app::PidGains{10000, 1000, 0});
        for (int index = 0; index < 100; ++index)
        {
            expectEqual(pid.update(200, 0), 1000,
                        "output saturates at maximum PWM");
        }
        expectEqual(
            pid.update(200, 200), 0,
            "conditional integration prevents windup during saturation");
    }

    {
        app::PidController pid = controller(app::PidGains{0, 1000, 0});
        pid.configureFeedForward(1000, 10000);
        expectEqual(
            pid.update(5000, 5100), 490,
            "integral correction can reduce an excessive feed-forward duty");
    }

    {
        app::PidController pid = controller(app::PidGains{0, 0, 1000});
        expectEqual(pid.update(100, 0), 0,
                    "first derivative sample is initialized without a kick");
        expectEqual(pid.update(200, 0), 0,
                    "derivative-on-measurement ignores a setpoint step");
    }

    if (failures != 0)
    {
        std::fprintf(stderr, "%d PID controller test(s) failed\n", failures);
        return 1;
    }

    std::puts("All PID controller tests passed");
    return 0;
}
