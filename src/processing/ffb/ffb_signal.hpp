#pragma once

#include <cmath>

// This file especially is beyond me. It seems to work fine though,
// if there's a proper way to handle motor control please PR those in or let me know.
// - Tumppi066

namespace ets2la_plugin::ffb
{
    /* Steering servo (layout 5 from MonoCruise ffb). Damped PID on the wheel angle at the game's FFB rate.
       Units are DirectInput magnitude (-10000..10000) and truck.input.steering units (-1..1 of lock, counterclockwise positive). */

    constexpr double FFB_FULL_SCALE      = 10000.0;
    constexpr double FFB_DT_CAP_S        = 0.05;
    constexpr double FFB_PI              = 3.14159265358979323846;

    constexpr double FFB_SERVO_CAP       = 6000.0;
    constexpr double FFB_ASSIST_CAP      = 6500.0;
    constexpr double FFB_CANCEL_CAP      = 6000.0;
    constexpr double FFB_SERVO_IN_S      = 0.3;
    constexpr double FFB_SERVO_OUT_S     = 0.2;
    constexpr double FFB_SERVO_SLEW      = 150000.0;
    constexpr double FFB_STEER_STALE_S   = 0.15;
    constexpr double FFB_STEER_RATE_TAU_S = 0.04;
    constexpr double FFB_RATE_FAST_TAU_S = 0.006;
    constexpr double FFB_EXTRAP_S        = 0.15;
    constexpr double FFB_D_SHARE         = 0.5;
    constexpr double FFB_LEAD_MAX_S      = 0.08;
    constexpr double FFB_FF_STILL_RATE   = 0.02;
    constexpr double FFB_FF_REST_BAND    = 0.002;
    constexpr double FFB_STUCK_FROM      = 1.5;
    constexpr double FFB_BREAK_RAMP_S    = 0.3;
    constexpr double FFB_BREAK_BAND      = 0.003;
    constexpr double FFB_BREAK_HOLD      = 0.0015;

    constexpr double FFB_SETTLE_BAND     = 0.0005;
    constexpr double FFB_SETTLE_RISE     = 0.0005;
    constexpr double FFB_SETTLE_RAMP_S   = 1.0;
    constexpr double FFB_SETTLE_DROP_S   = 0.02;
    constexpr double FFB_SETTLE_STEP_MAX = 0.005;
    constexpr double FFB_SETTLE_RELEASE  = 0.001;
    constexpr double FFB_I_MOVE_RATE     = 0.3;
    constexpr double FFB_D_LAG_MAX_S     = 0.05;
    constexpr double FFB_I_SHARE         = 1.0;
    constexpr double FFB_CANCEL_TAU_S    = 1.5;
    constexpr double FFB_FORCE_TAU_S     = 0.10;

    constexpr double FFB_BUZZ_MIN        = 300.0;
    constexpr double FFB_BUZZ_SWING      = 0.002;
    constexpr int    FFB_BUZZ_FLIPS      = 4;
    constexpr double FFB_BUZZ_WINDOW_S   = 0.5;
    constexpr double FFB_BUZZ_CUT        = 0.5;
    constexpr double FFB_BUZZ_FLOOR      = 0.1;
    constexpr double FFB_BUZZ_RECOVER    = 0.25;

    struct FfbServo
    {
        double gain;
        double value;
        double scale;
        double clock;
        double integ;
        double flips[FFB_BUZZ_FLIPS];
        int    nflips;
        int    sign;
        int    pushed;
        int    swing;
        int    seen;
        double peak;
        double ff;
        double d_ref;
        int    held;
        double held_u;
        double held_target;
        double u_prev;
        double ff_prev;
    };

    struct FfbServoIn
    {
        double target;
        double rate;
        double kp;
        double kd;
        double ki;
        double cap;
        int    dir;
        int    only;
        int    damp_first;
        double accel;
        double ff_inertia;
        double ff_damping;
        double ff_friction;
        double ff_breakaway;
        double stuck;
        double ff_rate;
        double ff_accel;
        double d_lag;
        double i_move;
        int    settle;
        double still_s;
        double interval;
        double step;
    };

    inline double ffb_ease(double u)
    {
        if (u <= 0.0)
            return 0.0;
        if (u >= 1.0)
            return 1.0;
        return 0.5 - 0.5 * std::cos(FFB_PI * u);
    }

    inline double ffb_dt_clamp(double dt)
    {
        if (!(dt > 0.0))
            return 0.0;
        return dt > FFB_DT_CAP_S ? FFB_DT_CAP_S : dt;
    }

    inline void ffb_servo_init(FfbServo* s)
    {
        s->gain        = 0.0;
        s->value       = 0.0;
        s->scale       = 1.0;
        s->clock       = 0.0;
        s->integ       = 0.0;
        s->nflips      = 0;
        s->sign        = 0;
        s->pushed      = 0;
        s->swing       = 0;
        s->seen        = 0;
        s->peak        = 0.0;
        s->ff          = 0.0;
        s->d_ref       = 0.0;
        s->held        = 0;
        s->held_u      = 0.0;
        s->held_target = 0.0;
        s->u_prev      = 0.0;
        s->ff_prev     = 0.0;
        for (int i = 0; i < FFB_BUZZ_FLIPS; i++)
        {
            s->flips[i] = 0.0;
        }
    }

    inline double ffb_clamp(double x, double cap)
    {
        if (!(x == x))
            return 0.0;
        if (x > cap)
            return cap;
        return x < -cap ? -cap : x;
    }

    inline double ffb_servo_cap(double cap)
    {
        return cap > 0.0 ? (cap < FFB_SERVO_CAP ? cap : FFB_SERVO_CAP) : 0.0;
    }

    inline double ffb_finite(double x)
    {
        return x == x && x < 1e30 && x > -1e30 ? x : 0.0;
    }

    inline void ffb_servo_aim(double target, double rate, double accel, double age, double* aim, double* aim_rate)
    {
        double a = age > 0.0 ? (age < FFB_EXTRAP_S ? age : FFB_EXTRAP_S) : 0.0;
        rate     = ffb_finite(rate);
        accel    = ffb_finite(accel);
        *aim      = target + rate * a + 0.5 * accel * a * a;
        *aim_rate = rate + accel * a;
    }

    inline double ffb_servo_ahead(double lead, double reading_age)
    {
        if (!(lead > 0.0))
            return 0.0;
        double ahead = lead + (reading_age > 0.0 ? reading_age : 0.0);
        return ahead < FFB_LEAD_MAX_S ? ahead : FFB_LEAD_MAX_S;
    }

    inline double ffb_servo_stuck_over(double still_s, double interval, double ramp)
    {
        if (!(interval > 0.0) || !(still_s >= 0.0))
            return 0.0;
        double n = (still_s - FFB_STUCK_FROM * interval) / ramp;
        return n > 0.0 ? (n < 1.0 ? n : 1.0) : 0.0;
    }

    inline double ffb_servo_stuck(double still_s, double interval)
    {
        return ffb_servo_stuck_over(still_s, interval, FFB_BREAK_RAMP_S);
    }

    inline double ffb_settle_step(double step, double change)
    {
        double grown = step * 1.001;
        if (!(change > 0.0))
            return step;
        step = step > 0.0 ? (change < grown ? change : grown) : change;
        return step < FFB_SETTLE_STEP_MAX ? step : FFB_SETTLE_STEP_MAX;
    }

    inline double ffb_servo_ff(const FfbServoIn* in, double cap, double err)
    {
        double ff = 0.0;
        if (in->ff_inertia > 0.0)
        {
            ff += in->ff_inertia * ffb_finite(in->ff_accel);
        }
        if (in->ff_damping > 0.0)
        {
            ff += in->ff_damping * ffb_finite(in->ff_rate);
        }
        if (in->ff_friction > 0.0)
        {
            double moving   = std::tanh(ffb_finite(in->ff_rate) / FFB_FF_STILL_RATE);
            double friction = in->ff_friction;
            double rest     = (1.0 - std::fabs(moving)) * std::tanh(ffb_finite(err) / FFB_FF_REST_BAND);
            if (in->ff_breakaway > friction && in->settle)
            {
                double band  = FFB_SETTLE_BAND + 0.5 * in->step;
                double near  = (std::fabs(ffb_finite(err)) - band) / FFB_SETTLE_RISE;
                int    close = std::fabs(ffb_finite(err)) < FFB_BREAK_BAND && std::fabs(moving) < 0.5;
                double base  = close ? 0.0 : friction;
                double stuck = ffb_servo_stuck_over(in->still_s, in->interval, close ? FFB_SETTLE_RAMP_S : FFB_BREAK_RAMP_S);
                near         = near > 0.0 ? (near < 1.0 ? near : 1.0) : 0.0;
                double gate  = std::fabs(moving) > near ? std::fabs(moving) : near;
                rest         = close ? (1.0 - std::fabs(moving)) * std::copysign(near, ffb_finite(err)) : rest * near;
                friction     = base + (ffb_finite(in->ff_breakaway) - base) * stuck * gate;
            }
            else if (in->ff_breakaway > friction)
            {
                double stuck = in->stuck > 0.0 ? (in->stuck < 1.0 ? in->stuck : 1.0) : 0.0;
                double off   = (std::fabs(ffb_finite(err)) - FFB_BREAK_HOLD) / (FFB_BREAK_BAND - FFB_BREAK_HOLD);
                off          = off > 0.0 ? (off < 1.0 ? off : 1.0) : 0.0;
                double gate  = std::fabs(moving) > off ? std::fabs(moving) : off;
                rest *= off;
                friction += (ffb_finite(in->ff_breakaway) - friction) * stuck * gate;
            }
            ff += friction * (moving + rest);
        }
        return ffb_clamp(ff, cap);
    }

    inline void ffb_settle_reset(FfbServo* s)
    {
        s->held    = 0;
        s->u_prev  = 0.0;
        s->ff_prev = 0.0;
    }

    inline void ffb_settle_note(FfbServo* s, const FfbServoIn* in, double target, double e, double dt)
    {
        double band    = FFB_SETTLE_BAND + 0.5 * in->step;
        double release = in->step > FFB_SETTLE_RELEASE ? in->step : FFB_SETTLE_RELEASE;
        if (!in->settle)
        {
            s->held = 0;
            return;
        }
        if (std::fabs(s->ff) < std::fabs(s->ff_prev) && s->ff * s->ff_prev >= 0.0)
        {
            s->ff = s->ff_prev + (s->ff - s->ff_prev) * (1.0 - std::exp(-dt / FFB_SETTLE_DROP_S));
        }
        s->ff_prev   = s->ff;
        int standing = in->interval > 0.0 && in->still_s >= FFB_STUCK_FROM * in->interval;
        int calm     = std::fabs(ffb_finite(in->rate)) < FFB_FF_STILL_RATE && std::fabs(ffb_finite(in->ff_rate)) < FFB_FF_STILL_RATE;
        if (s->held && (std::fabs(e) > band + release || !calm || std::fabs(target - s->held_target) > band))
        {
            s->held = 0;
        }
        else if (!s->held && standing && calm && std::fabs(e) <= band)
        {
            s->held        = 1;
            s->held_u      = s->u_prev;
            s->held_target = target;
        }
    }

    inline double ffb_servo_sum(double p, double d, double integ, double cap, int damp_first)
    {
        double pi = p + integ;
        if (damp_first)
        {
            double room = cap - std::fabs(d);
            pi          = ffb_clamp(pi, room > 0.0 ? room : 0.0);
        }
        return ffb_clamp(pi + d, cap);
    }

    inline double ffb_servo_pid(FfbServo* s, const FfbServoIn* in, double target, double angle, double rate, double dt)
    {
        double cap = ffb_servo_cap(in->cap);
        dt         = ffb_dt_clamp(dt);
        s->ff      = 0.0;
        if (!in->dir || !(in->kp >= 0.0) || !(in->kd >= 0.0) || !(in->ki >= 0.0))
        {
            s->integ = 0.0;
            s->d_ref = 0.0;
            ffb_settle_reset(s);
            return 0.0;
        }
        double e = target - angle;
        // Bleed integral on zero-crossing (when error changes sign against accumulated integral)
        // to prevent ~2 Hz stiction limit-cycle oscillations around the setpoint on geared wheels.
        if (s->integ * e < 0.0)
        {
            s->integ *= std::exp(-dt / 0.015);
        }
        s->ff    = ffb_servo_ff(in, cap, e);
        ffb_settle_note(s, in, target, e, dt);
        double p = in->kp * e + s->ff;
        if (in->d_lag > 0.0)
        {
            s->d_ref += (in->rate - s->d_ref) * (1.0 - std::exp(-dt / in->d_lag));
        }
        else
        {
            s->d_ref = in->rate;
        }
        double d    = ffb_clamp(in->kd * (s->d_ref - rate), FFB_D_SHARE * cap);
        double room = in->damp_first ? cap - std::fabs(d) : cap;
        double u    = in->damp_first ? p + s->integ : p + d + s->integ;
        int moving  = in->i_move > 0.0 && (std::fabs(in->rate) > in->i_move || std::fabs(ffb_finite(in->ff_rate)) > in->i_move);
        if (!moving && !s->held && (std::fabs(u) < room || u * e < 0.0))
        {
            s->integ = ffb_clamp(s->integ + in->ki * e * dt, 0.25 * cap);
        }
        if (in->only && s->integ * in->only < 0.0)
        {
            s->integ = 0.0;
        }
        u         = s->held ? ffb_clamp(s->held_u, cap) : ffb_servo_sum(p, d, s->integ, cap, in->damp_first);
        s->u_prev = u;
        if (in->only)
        {
            return u * in->only < 0.0 ? 0.0 : static_cast<double>(in->dir) * u;
        }
        return static_cast<double>(in->dir) * u;
    }

    inline void ffb_buzz_note(FfbServo* s, double want, double angle)
    {
        int sign = want > FFB_BUZZ_MIN ? 1 : (want < -FFB_BUZZ_MIN ? -1 : 0);
        int kept = 0;
        if (sign && sign != s->sign)
        {
            s->pushed = s->pushed || s->sign != 0;
            s->sign   = sign;
        }
        if (!(angle == angle))
        {
            s->seen   = 0;
            s->swing  = 0;
            s->pushed = 0;
            return;
        }
        if (!s->seen)
        {
            s->peak = angle;
            s->seen = 1;
            return;
        }
        if (!s->swing)
        {
            if (std::fabs(angle - s->peak) >= FFB_BUZZ_SWING)
            {
                s->swing = angle > s->peak ? 1 : -1;
                s->peak  = angle;
            }
            return;
        }
        if ((angle - s->peak) * s->swing >= 0.0)
        {
            s->peak = angle;
            return;
        }
        if (std::fabs(angle - s->peak) < FFB_BUZZ_SWING)
        {
            return;
        }
        s->swing = -s->swing;
        s->peak  = angle;
        if (!s->pushed)
        {
            return;
        }
        s->pushed = 0;
        for (int i = 0; i < s->nflips; i++)
        {
            if (s->clock - s->flips[i] <= FFB_BUZZ_WINDOW_S)
            {
                s->flips[kept++] = s->flips[i];
            }
        }
        s->nflips = kept;
        if (s->nflips < FFB_BUZZ_FLIPS)
        {
            s->flips[s->nflips++] = s->clock;
        }
        if (s->nflips >= FFB_BUZZ_FLIPS)
        {
            s->scale *= FFB_BUZZ_CUT;
            if (s->scale < FFB_BUZZ_FLOOR)
            {
                s->scale = FFB_BUZZ_FLOOR;
            }
            s->nflips = 0;
        }
    }

    inline double ffb_servo_step(FfbServo* s, double want, int on, double angle, double dt)
    {
        dt       = ffb_dt_clamp(dt);
        s->clock += dt;
        s->scale += FFB_BUZZ_RECOVER * dt;
        if (s->scale > 1.0)
        {
            s->scale = 1.0;
        }
        want = on ? ffb_clamp(want, FFB_SERVO_CAP) : 0.0;
        if (on)
        {
            ffb_buzz_note(s, want, angle);
        }
        else
        {
            s->integ = 0.0;
            ffb_settle_reset(s);
        }
        s->gain += on ? dt / FFB_SERVO_IN_S : -dt / FFB_SERVO_OUT_S;
        s->gain  = s->gain < 0.0 ? 0.0 : (s->gain > 1.0 ? 1.0 : s->gain);
        double target = want * s->scale * ffb_ease(s->gain);
        double step   = target - s->value;
        double limit  = FFB_SERVO_SLEW * dt;
        s->value += step > limit ? limit : (step < -limit ? -limit : step);
        if (!on && s->gain <= 0.0 && std::fabs(s->value) < 0.5)
        {
            s->value  = 0.0;
            s->sign   = 0;
            s->pushed = 0;
            s->swing  = 0;
            s->seen   = 0;
            s->nflips = 0;
        }
        return s->value;
    }

    inline long ffb_round(double x)
    {
        return static_cast<long>(x >= 0.0 ? x + 0.5 : x - 0.5);
    }

    inline long ffb_clamp_mag(long mag)
    {
        if (mag > 10000)
            return 10000;
        if (mag < -10000)
            return -10000;
        return mag;
    }
}
