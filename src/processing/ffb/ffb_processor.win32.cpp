#include "processing/ffb/ffb_processor.hpp"
#include "processing/ffb/ffb_signal.hpp"

#include <common/scssdk_telemetry_truck_common_channels.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <Windows.h>
#include <dinput.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

// This file is largely based on MonoCruise. See this folder's README.md for more information.
// This file was also largely created by Gemini, and again while I did some cleanup, I am largely
// assuming it works code and logic wise.

namespace ets2la_plugin
{
    namespace
    {
        using namespace ets2la_plugin::ffb;

        constexpr int VT_RELEASE         = 2;
        constexpr int VT_GET_CAPS        = 3;
        constexpr int VT_GET_EFFECT_GUID = 4;
        constexpr int VT_GET_PARAMS      = 5;
        constexpr int VT_SET_PARAMS      = 6;
        constexpr int VT_GET_STATE       = 9;
        constexpr int VT_SET_FORMAT      = 11;
        constexpr int VT_CREATE_EFFECT   = 18;
        constexpr int VT_ENUM_EFFECTS    = 23;

        static_assert(sizeof(DIEFFECT) == 80, "DIEFFECT layout");
        static_assert(offsetof(DIEFFECT, cbTypeSpecificParams) == 56, "DIEFFECT params");
        static_assert(offsetof(DIEFFECT, lpvTypeSpecificParams) == 64, "DIEFFECT param pointer");

        // Effect kinds in dinput.h
        // GUID_ConstantForce is 13541C20, each next kind is +1.
        constexpr int FFB_FX_CONSTANT = 0;
        constexpr int FFB_FX_SPRING   = 7;
        constexpr int FFB_FX_FRICTION = 10;
        constexpr int FFB_FX_KINDS    = 12;
        constexpr int FFB_FX_OTHER    = 31;

        // Default PID parameters
        constexpr double DEFAULT_SERVO_KP_NEAR   = 30000.0;
        constexpr double DEFAULT_SERVO_KP_FAR    = 45000.0;
        constexpr double DEFAULT_SERVO_KD        = 8500.0;
        constexpr double DEFAULT_SERVO_KI        = 10000.0;
        constexpr double DEFAULT_SERVO_CAP       = 4200.0;
        constexpr double DEFAULT_FF_DAMPING      = 400.0;
        constexpr double DEFAULT_FF_FRICTION     = 160.0;
        constexpr double TARGET_SMOOTH_TAU_S     = 0.045;

        const GUID k_constant_force = {
            0x13541C20, 0x8E33, 0x11D0, {0x9A, 0xD0, 0x00, 0xA0, 0xC9, 0xA0, 0x6E, 0x35}
        };
        const GUID k_iid_di8_w = {
            0xBF798031, 0x483A, 0x4DA2, {0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00}
        };

        using SetFmtFn    = HRESULT(WINAPI*)(void* self, const DIDATAFORMAT* fmt);
        using CreateFxFn  = HRESULT(WINAPI*)(void* self, const GUID* guid, const DIEFFECT* eff, void** out, void* unk);
        using GdsFn       = HRESULT(WINAPI*)(void* self, DWORD cb, void* data);
        using SetFn       = HRESULT(WINAPI*)(void* self, const DIEFFECT* eff, DWORD flags);
        using GetCapsFn   = HRESULT(WINAPI*)(void* self, DIDEVCAPS* caps);
        using GetGuidFn   = HRESULT(WINAPI*)(void* self, GUID* guid);
        using GetParamsFn = HRESULT(WINAPI*)(void* self, DIEFFECT* eff, DWORD flags);
        using EnumFxFn    = HRESULT(WINAPI*)(void* self, LPDIENUMCREATEDEFFECTOBJECTSCALLBACK cb, void* ctx, DWORD flags);
        using RelFn       = ULONG(WINAPI*)(void* self);

        struct Patch
        {
            void** slot;
            void*  original;
        };

        struct SavedEffect
        {
            void*       fx;
            int         kind;
            DWORD       gain;
            DICONDITION cond[2];
            DWORD       cb_cond;
            bool        has_gain;
            bool        has_cond;
            bool        muted;
        };

        scs_log_t     g_log          = nullptr;
        volatile LONG g_live         = 0;
        volatile LONG g_dead         = 0;
        volatile LONG g_driving      = 0;
        volatile LONG g_depth        = 0;
        volatile LONG g_busy         = 0;
        volatile LONG g_seen_cf      = 0;
        volatile LONG g_have_mag     = 0;
        volatile LONG g_first_mag    = 0;
        volatile LONG g_servoing     = 0;
        volatile LONG g_steer_ok     = 0;
        volatile LONG g_steer_count  = 0;
        volatile LONG g_engaged      = 0;
        volatile LONG g_servo_dir    = 0; // +1 or -1 once auto-detected from wheel movement, defaults to +1

        volatile float g_steer           = 0.0f;
        volatile float g_steer_rate      = 0.0f;
        volatile float g_steer_rate_fast = 0.0f;
        volatile float g_steer_interval  = 0.0f;
        volatile float g_settle_step     = 0.0f;

        volatile float    g_target       = 0.0f;
        volatile float    g_target_rate  = 0.0f;
        volatile float    g_target_accel = 0.0f;
        volatile uint32_t g_target_stamp = 0;
        float* volatile   g_vehicle_steer_ptr = nullptr;

        uint32_t      g_stamp_last     = 0;
        LARGE_INTEGER g_stamp_at       = {};
        LARGE_INTEGER g_cf_at          = {};
        LARGE_INTEGER g_steer_at       = {};
        LARGE_INTEGER g_steer_moved_at = {};
        LARGE_INTEGER g_qpf            = {};
        LARGE_INTEGER g_qpc            = {};

        FfbServo g_servo = {};

        // Direction auto-calibration & adaptive stiction state
        double g_cal_score          = 0.0;
        double g_last_u             = 0.0;
        double g_adaptive_breakaway = 0.0;

        Patch      g_patches[8]  = {};
        int        g_npatches    = 0;
        void*      g_set_vt[2]   = {};
        SetFn      g_set_orig[2] = {};
        void*      g_gds_vt[2]   = {};
        GdsFn      g_gds_orig[2] = {};
        void*      g_ce_vt[2]    = {};
        CreateFxFn g_ce_orig[2]  = {};
        GUID       g_devs[8]     = {};
        int        g_ndev        = 0;
        void*      g_skip[8]     = {};
        int        g_nskip       = 0;
        int        g_game_enum_done = 0;

        SavedEffect g_saved_fx[16] = {};
        int         g_nsaved_fx    = 0;

        void log_raw(const char* msg)
        {
            if (g_log && msg)
            {
                g_log(0, msg);
            }
        }

        void log_hr(const char* what, HRESULT hr)
        {
            char buf[160];
            std::snprintf(buf, sizeof(buf), "[ets2la_plugin][FfbProcessor] %s 0x%08lX", what, static_cast<unsigned long>(hr));
            log_raw(buf);
        }

        bool guid_eq(const GUID* a, const GUID* b)
        {
            return std::memcmp(a, b, sizeof(GUID)) == 0;
        }

        bool in_dinput(void* fn)
        {
            HMODULE ours = nullptr;
            HMODULE di   = GetModuleHandleW(L"dinput8.dll");
            if (!di || !fn)
            {
                return false;
            }
            if (!GetModuleHandleExW(
                    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<LPCWSTR>(fn),
                    &ours))
            {
                return false;
            }
            return ours == di;
        }

        void remember(void** slot, void* original)
        {
            for (int i = 0; i < g_npatches; i++)
            {
                if (g_patches[i].slot == slot)
                {
                    return;
                }
            }
            if (g_npatches >= 8)
            {
                return;
            }
            g_patches[g_npatches].slot     = slot;
            g_patches[g_npatches].original = original;
            g_npatches++;
        }

        bool patch_slot(void** slot, void* hook)
        {
            DWORD old = 0;
            if (!slot || !hook)
            {
                return false;
            }
            if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &old))
            {
                return false;
            }
            void* cur = *slot;
            if (cur == hook)
            {
                VirtualProtect(slot, sizeof(void*), old, &old);
                return true;
            }
            if (!in_dinput(cur))
            {
                VirtualProtect(slot, sizeof(void*), old, &old);
                return false;
            }
            remember(slot, cur);
            *slot = hook;
            VirtualProtect(slot, sizeof(void*), old, &old);
            FlushInstructionCache(GetCurrentProcess(), slot, sizeof(void*));
            return true;
        }

        void release_obj(void* obj)
        {
            if (!obj)
            {
                return;
            }
            void** vt  = *reinterpret_cast<void***>(obj);
            RelFn  rel = reinterpret_cast<RelFn>(vt[VT_RELEASE]);
            if (in_dinput(reinterpret_cast<void*>(rel)))
            {
                rel(obj);
            }
        }

        SetFn orig_set(void* self)
        {
            void* vt = *reinterpret_cast<void**>(self);
            if (vt == g_set_vt[1])
            {
                return g_set_orig[1];
            }
            return g_set_orig[0];
        }

        GdsFn orig_gds(void* self)
        {
            void* vt = *reinterpret_cast<void**>(self);
            if (vt == g_gds_vt[1])
            {
                return g_gds_orig[1];
            }
            return g_gds_orig[0];
        }

        CreateFxFn orig_create(void* self)
        {
            void* vt = *reinterpret_cast<void**>(self);
            if (vt == g_ce_vt[1])
            {
                return g_ce_orig[1];
            }
            return g_ce_orig[0];
        }

        int fx_kind(const GUID* g)
        {
            if (!g || std::memcmp(reinterpret_cast<const char*>(g) + 4, reinterpret_cast<const char*>(&k_constant_force) + 4, sizeof(GUID) - 4) != 0)
            {
                return FFB_FX_OTHER;
            }
            uint32_t n = static_cast<uint32_t>(g->Data1 - k_constant_force.Data1);
            return n < FFB_FX_KINDS ? static_cast<int>(n) : FFB_FX_OTHER;
        }

        int fx_kind_of(void* fx)
        {
            if (!fx)
            {
                return FFB_FX_OTHER;
            }
            GetGuidFn get_guid = reinterpret_cast<GetGuidFn>((*reinterpret_cast<void***>(fx))[VT_GET_EFFECT_GUID]);
            GUID      guid;
            if (!in_dinput(reinterpret_cast<void*>(get_guid)) || FAILED(get_guid(fx, &guid)))
            {
                return FFB_FX_OTHER;
            }
            return fx_kind(&guid);
        }

        bool fx_condition(int kind)
        {
            return kind >= FFB_FX_SPRING && kind <= FFB_FX_FRICTION;
        }

        SavedEffect* find_or_add_saved_fx(void* fx, int kind)
        {
            if (!fx || kind == FFB_FX_CONSTANT)
            {
                return nullptr;
            }
            for (int i = 0; i < g_nsaved_fx; i++)
            {
                if (g_saved_fx[i].fx == fx)
                {
                    return &g_saved_fx[i];
                }
            }
            if (g_nsaved_fx >= 16)
            {
                return nullptr;
            }
            SavedEffect* s = &g_saved_fx[g_nsaved_fx++];
            ZeroMemory(s, sizeof(*s));
            s->fx   = fx;
            s->kind = kind;
            return s;
        }

        void save_fx_params(SavedEffect* s, const DIEFFECT* eff, DWORD flags)
        {
            if (!s || !eff || eff->dwSize != sizeof(DIEFFECT))
            {
                return;
            }
            if (flags & DIEP_GAIN)
            {
                s->gain     = eff->dwGain;
                s->has_gain = true;
            }
            if ((flags & DIEP_TYPESPECIFICPARAMS) && fx_condition(s->kind) && eff->lpvTypeSpecificParams
                && eff->cbTypeSpecificParams >= sizeof(DICONDITION))
            {
                DWORD cb = eff->cbTypeSpecificParams;
                if (cb > sizeof(s->cond))
                {
                    cb = sizeof(s->cond);
                }
                std::memcpy(s->cond, eff->lpvTypeSpecificParams, cb);
                s->cb_cond  = cb;
                s->has_cond = true;
            }
        }

        void mute_single_fx(SavedEffect* s)
        {
            if (!s || !s->fx)
            {
                return;
            }
            SetFn orig = orig_set(s->fx);
            if (!orig)
            {
                return;
            }
            DIEFFECT    eff;
            DICONDITION zero_cond[2];
            ZeroMemory(&eff, sizeof(eff));
            ZeroMemory(zero_cond, sizeof(zero_cond));
            eff.dwSize  = sizeof(eff);
            eff.dwGain  = 0;
            DWORD flags = DIEP_GAIN | DIEP_NORESTART;
            if (fx_condition(s->kind) && s->has_cond && s->cb_cond >= sizeof(DICONDITION))
            {
                eff.cbTypeSpecificParams  = s->cb_cond;
                eff.lpvTypeSpecificParams = zero_cond;
                flags |= DIEP_TYPESPECIFICPARAMS;
            }
            orig(s->fx, &eff, flags);
            s->muted = true;
        }

        void restore_single_fx(SavedEffect* s)
        {
            if (!s || !s->fx || !s->muted)
            {
                return;
            }
            SetFn orig = orig_set(s->fx);
            if (!orig)
            {
                return;
            }
            DIEFFECT eff;
            ZeroMemory(&eff, sizeof(eff));
            eff.dwSize  = sizeof(eff);
            DWORD flags = DIEP_NORESTART;
            if (s->has_gain)
            {
                eff.dwGain = s->gain;
                flags |= DIEP_GAIN;
            }
            else
            {
                eff.dwGain = DI_FFNOMINALMAX;
                flags |= DIEP_GAIN;
            }
            if (fx_condition(s->kind) && s->has_cond && s->cb_cond >= sizeof(DICONDITION))
            {
                eff.cbTypeSpecificParams  = s->cb_cond;
                eff.lpvTypeSpecificParams = s->cond;
                flags |= DIEP_TYPESPECIFICPARAMS;
            }
            orig(s->fx, &eff, flags);
            s->muted = false;
        }

        void update_other_effects_mute_state(bool should_mute)
        {
            for (int i = 0; i < g_nsaved_fx; i++)
            {
                SavedEffect* s = &g_saved_fx[i];
                if (should_mute && !s->muted)
                {
                    mute_single_fx(s);
                }
                else if (!should_mute && s->muted)
                {
                    restore_single_fx(s);
                }
            }
        }

        void record_existing_fx(void* fx, int kind)
        {
            if (!fx || kind == FFB_FX_CONSTANT)
            {
                return;
            }
            SavedEffect* s = find_or_add_saved_fx(fx, kind);
            if (!s)
            {
                return;
            }
            GetParamsFn get = reinterpret_cast<GetParamsFn>((*reinterpret_cast<void***>(fx))[VT_GET_PARAMS]);
            if (!in_dinput(reinterpret_cast<void*>(get)))
            {
                return;
            }
            DICONDITION cond[2];
            DIEFFECT    eff;
            DWORD       flags = DIEP_GAIN;
            ZeroMemory(&eff, sizeof(eff));
            ZeroMemory(cond, sizeof(cond));
            eff.dwSize = sizeof(eff);
            if (fx_condition(kind))
            {
                flags |= DIEP_TYPESPECIFICPARAMS;
                eff.cbTypeSpecificParams  = sizeof(cond);
                eff.lpvTypeSpecificParams = cond;
            }
            if (SUCCEEDED(get(fx, &eff, flags)))
            {
                save_fx_params(s, &eff, flags);
            }
        }

        double tick_dt()
        {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            if (g_qpc.QuadPart == 0 || g_qpf.QuadPart == 0)
            {
                g_qpc = now;
                return 0.0;
            }
            double dt = static_cast<double>(now.QuadPart - g_qpc.QuadPart) / static_cast<double>(g_qpf.QuadPart);
            g_qpc     = now;
            if (dt < 0.0)
            {
                dt = 0.0;
            }
            return dt;
        }

        bool steer_fresh()
        {
            if (!InterlockedOr(&g_steer_ok, 0) || g_steer_at.QuadPart == 0 || g_qpf.QuadPart == 0)
            {
                return false;
            }
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            return static_cast<double>(now.QuadPart - g_steer_at.QuadPart) / static_cast<double>(g_qpf.QuadPart) <= FFB_STEER_STALE_S;
        }

        bool cf_fresh()
        {
            if (!InterlockedOr(&g_seen_cf, 0) || g_cf_at.QuadPart == 0 || g_qpf.QuadPart == 0)
            {
                return false;
            }
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            return static_cast<double>(now.QuadPart - g_cf_at.QuadPart) / static_cast<double>(g_qpf.QuadPart) <= 0.5;
        }

        double target_age(uint32_t stamp)
        {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            if (stamp != g_stamp_last || g_stamp_at.QuadPart == 0)
            {
                g_stamp_last = stamp;
                g_stamp_at   = now;
                return 0.0;
            }
            return g_qpf.QuadPart ? static_cast<double>(now.QuadPart - g_stamp_at.QuadPart) / static_cast<double>(g_qpf.QuadPart) : 0.0;
        }

        double steer_still()
        {
            if (g_steer_moved_at.QuadPart == 0 || g_qpf.QuadPart == 0)
            {
                return 0.0;
            }
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            return static_cast<double>(now.QuadPart - g_steer_moved_at.QuadPart) / static_cast<double>(g_qpf.QuadPart);
        }

        double compute_servo_force(bool active, double dt)
        {
            static double smooth_target = 0.0;
            static bool   smooth_init   = false;
            static int    last_err_sign = 0;
            static double last_flip_clk = -10.0;
            static double osc_level     = 0.0;

            double raw = 0.0;
            int    on  = active && steer_fresh() ? 1 : 0;
            if (on)
            {
                int dir = static_cast<int>(InterlockedOr(&g_servo_dir, 0));
                if (dir == 0)
                {
                    dir = 1;
                }

                double still      = steer_still();
                double iv         = static_cast<double>(g_steer_interval);
                double raw_target = static_cast<double>(g_target);

                // Smooth the discrete backend target monotonically at the high FFB update rate (120-500 Hz).
                // A 1st-order exponential filter is strictly monotonic (never overshoots or snaps back) and its
                // analytical derivative (raw_target - smooth_target) / tau gives a clean, lag-free target velocity.
                if (!smooth_init || std::fabs(raw_target - smooth_target) > 0.5)
                {
                    smooth_target = raw_target;
                    smooth_init   = true;
                }
                else if (dt > 0.0)
                {
                    smooth_target += (raw_target - smooth_target) * (1.0 - std::exp(-dt / TARGET_SMOOTH_TAU_S));
                }

                double smooth_rate = ffb_clamp((raw_target - smooth_target) / TARGET_SMOOTH_TAU_S, 1.5);
                double err         = smooth_target - static_cast<double>(g_steer);
                double err_abs     = std::fabs(err);

                // Decay measured wheel rate smoothly between 60 Hz telemetry frames so a single encoder step
                // doesn't hold a constant derivative braking spike across multiple FFB frames.
                double wheel_rate = static_cast<double>(g_steer_rate);
                if (iv > 0.0 && still > iv)
                {
                    wheel_rate *= std::exp(-(still - iv) / FFB_STEER_RATE_TAU_S);
                }

                // Detect ~1-5 Hz limit-cycle oscillations around a quasi-stationary target (e.g. G923 rocking left-right
                // when straight). Normal driving/turning has zero osc_level, so output power and DD responsiveness are untouched.
                if (dt > 0.0)
                {
                    osc_level *= std::exp(-dt / 1.2);
                }
                int err_sign = (err > 0.0025) ? 1 : ((err < -0.0025) ? -1 : 0);
                if (err_sign != 0 && err_sign != last_err_sign)
                {
                    if (last_err_sign != 0 && std::fabs(smooth_rate) < 0.12)
                    {
                        double since_flip = g_servo.clock - last_flip_clk;
                        if (since_flip >= 0.06 && since_flip <= 0.85)
                        {
                            osc_level += 0.45;
                            if (osc_level > 1.0)
                            {
                                osc_level = 1.0;
                            }
                            g_servo.integ *= 0.25;
                        }
                    }
                    last_err_sign = err_sign;
                    last_flip_clk = g_servo.clock;
                }

                // When moving away from or overshooting a stationary target (err * wheel_rate > 0), blend in fast
                // rate (6 ms tau) on top of the 40 ms rate to eliminate the phase lag that fuels ~2 Hz limit cycles.
                double fast_rate = static_cast<double>(g_steer_rate_fast);
                if (iv > 0.0 && still > iv)
                {
                    fast_rate *= std::exp(-(still - iv) / FFB_STEER_RATE_TAU_S);
                }
                if (std::fabs(smooth_rate) < 0.08 && err * fast_rate > 0.0)
                {
                    double lead_blend = 0.35 + 0.45 * osc_level;
                    wheel_rate        = wheel_rate * (1.0 - lead_blend) + fast_rate * lead_blend;
                }

                // Smoothly transition KP from 30000 inside the tiny center deadzone (|err| <= 0.012, preventing G29/G923 gear chatter)
                // up to 45000 outside (|err| >= 0.04, giving continuous, non-pulsing return force when off-center).
                double kp_blend = (err_abs - 0.012) / (0.040 - 0.012);
                kp_blend        = kp_blend < 0.0 ? 0.0 : (kp_blend > 1.0 ? 1.0 : kp_blend);
                double kp       = DEFAULT_SERVO_KP_NEAR + (DEFAULT_SERVO_KP_FAR - DEFAULT_SERVO_KP_NEAR) * kp_blend;

                // If oscillation is detected, temporarily boost derivative damping (up to 2.4x) and soften near-center KP/stiction
                // only inside the small oscillation zone (|err| < 0.06), leaving full output power intact for real turns.
                double kd          = DEFAULT_SERVO_KD * (1.0 + 1.4 * osc_level);
                double ki          = DEFAULT_SERVO_KI * (1.0 - 0.8 * osc_level);
                double ff_friction = DEFAULT_FF_FRICTION * (1.0 - 0.85 * osc_level);
                if (osc_level > 0.0 && err_abs < 0.06)
                {
                    kp *= (1.0 - 0.35 * osc_level);
                }

                FfbServoIn in = {};
                in.target       = smooth_target;
                in.rate         = smooth_rate;
                in.kp           = kp;
                in.kd           = kd;
                in.ki           = ki;
                in.cap          = DEFAULT_SERVO_CAP;
                in.dir          = dir;
                in.only         = 0;
                in.damp_first   = 0;
                in.accel        = 0.0;
                in.ff_inertia   = 0.0;
                in.ff_damping   = DEFAULT_FF_DAMPING;
                in.ff_friction  = ff_friction;
                in.ff_breakaway = 0.0;
                in.stuck        = 0.0;
                in.settle       = 0;
                in.still_s      = still;
                in.interval     = iv;
                in.step         = static_cast<double>(g_settle_step);
                in.d_lag        = 0.0;
                in.i_move       = FFB_I_MOVE_RATE;
                in.ff_rate      = smooth_rate;
                in.ff_accel     = 0.0;

                raw = ffb_servo_pid(
                    &g_servo,
                    &in,
                    smooth_target,
                    static_cast<double>(g_steer),
                    wheel_rate,
                    dt
                );
            }
            else
            {
                smooth_init   = false;
                last_err_sign = 0;
                osc_level     = 0.0;
                g_servo.ff    = 0.0;
            }
            return ffb_servo_step(&g_servo, raw, on, static_cast<double>(g_steer), dt);
        }

        HRESULT hook_set_body(SetFn orig, void* self, const DIEFFECT* eff, DWORD flags)
        {
            if (InterlockedIncrement(&g_depth) != 1)
            {
                InterlockedDecrement(&g_depth);
                return orig(self, eff, flags);
            }

            bool is_cf = eff && eff->dwSize == sizeof(DIEFFECT) && (flags & DIEP_TYPESPECIFICPARAMS)
                         && eff->cbTypeSpecificParams == sizeof(DICONSTANTFORCE) && eff->lpvTypeSpecificParams != nullptr;

            bool engaged = InterlockedOr(&g_engaged, 0) != 0 && InterlockedOr(&g_driving, 0) != 0;

            if (!is_cf)
            {
                if (eff && eff->dwSize == sizeof(DIEFFECT))
                {
                    int kind = fx_kind_of(self);
                    if (kind != FFB_FX_CONSTANT)
                    {
                        SavedEffect* s = find_or_add_saved_fx(self, kind);
                        if (s)
                        {
                            save_fx_params(s, eff, flags);
                            if (engaged || g_servo.gain > 0.0)
                            {
                                mute_single_fx(s);
                                InterlockedDecrement(&g_depth);
                                return DI_OK;
                            }
                        }
                    }
                }
                InterlockedDecrement(&g_depth);
                return orig(self, eff, flags);
            }

            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            g_cf_at = now;
            InterlockedExchange(&g_seen_cf, 1);

            LONG game_mag = static_cast<const DICONSTANTFORCE*>(eff->lpvTypeSpecificParams)->lMagnitude;
            if (InterlockedOr(&g_driving, 0) && !g_have_mag)
            {
                g_first_mag = game_mag;
                g_have_mag  = 1;
            }

            double dt    = tick_dt();
            double servo = compute_servo_force(engaged, dt);
            double fade  = ffb_ease(g_servo.gain);

            bool servo_active = g_servo.gain > 0.0;
            InterlockedExchange(&g_servoing, servo_active ? 1 : 0);

            // Mute or restore all other effects (spring, damper, inertia, friction) when servo engages/disengages
            update_other_effects_mute_state(servo_active);

            if (engaged)
            {
                float* veh_steer = g_vehicle_steer_ptr;
                if (veh_steer != nullptr)
                {
                    *veh_steer = g_target;
                }
            }

            if (!servo_active && servo == 0.0)
            {
                InterlockedDecrement(&g_depth);
                return orig(self, eff, flags);
            }

            // Fade out the game's constant force completely as the servo fades in, leaving only the servo torque
            double blended = static_cast<double>(game_mag) * (1.0 - fade) + servo;
            DIEFFECT        copy = *eff;
            DICONSTANTFORCE cf;
            cf.lMagnitude              = ffb_clamp_mag(ffb_round(blended));
            copy.lpvTypeSpecificParams = &cf;

            HRESULT hr = orig(self, &copy, flags);
            InterlockedDecrement(&g_depth);
            return hr;
        }

        HRESULT WINAPI hook_set(void* self, const DIEFFECT* eff, DWORD flags)
        {
            SetFn orig = orig_set(self);
            if (!orig)
            {
                return E_FAIL;
            }
            if (!g_live || g_dead)
            {
                return orig(self, eff, flags);
            }
            __try
            {
                return hook_set_body(orig, self, eff, flags);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                g_dead  = 1;
                g_depth = 0;
            }
            return orig(self, eff, flags);
        }

        void hook_effect(void* fx)
        {
            if (!fx)
            {
                return;
            }
            void** vt = *reinterpret_cast<void***>(fx);
            for (int i = 0; i < 2; i++)
            {
                if (g_set_vt[i] == vt)
                {
                    return;
                }
            }
            for (int i = 0; i < 2; i++)
            {
                if (!g_set_vt[i])
                {
                    void** slot = &vt[VT_SET_PARAMS];
                    void*  cur  = *slot;
                    if (!in_dinput(cur))
                    {
                        return;
                    }
                    g_set_orig[i] = reinterpret_cast<SetFn>(cur);
                    g_set_vt[i]   = vt;
                    MemoryBarrier();
                    patch_slot(slot, reinterpret_cast<void*>(hook_set));
                    return;
                }
            }
        }

        BOOL CALLBACK on_game_effect(LPDIRECTINPUTEFFECT fx, LPVOID ctx)
        {
            (void)ctx;
            if (!fx)
            {
                return DIENUM_CONTINUE;
            }
            int kind = fx_kind_of(fx);
            hook_effect(fx);
            record_existing_fx(fx, kind);
            return DIENUM_CONTINUE;
        }

        bool skipped(void* obj)
        {
            for (int i = 0; i < g_nskip; i++)
            {
                if (g_skip[i] == obj)
                {
                    return true;
                }
            }
            return false;
        }

        void note_skip(void* obj)
        {
            if (g_nskip < 8 && !skipped(obj))
            {
                g_skip[g_nskip++] = obj;
            }
        }

        void try_enum_game(void* dev)
        {
            if (!dev || g_game_enum_done || skipped(dev))
            {
                return;
            }
            GetCapsFn get_caps = reinterpret_cast<GetCapsFn>((*reinterpret_cast<void***>(dev))[VT_GET_CAPS]);
            if (!in_dinput(reinterpret_cast<void*>(get_caps)))
            {
                note_skip(dev);
                return;
            }
            DIDEVCAPS caps;
            caps.dwSize = sizeof(caps);
            if (FAILED(get_caps(dev, &caps)) || !(caps.dwFlags & DIDC_FORCEFEEDBACK))
            {
                note_skip(dev);
                return;
            }
            g_game_enum_done = 1;
            EnumFxFn enum_fx = reinterpret_cast<EnumFxFn>((*reinterpret_cast<void***>(dev))[VT_ENUM_EFFECTS]);
            if (in_dinput(reinterpret_cast<void*>(enum_fx)))
            {
                enum_fx(dev, on_game_effect, nullptr, 0);
            }
        }

        HRESULT WINAPI hook_gds(void* self, DWORD cb, void* data)
        {
            GdsFn orig = orig_gds(self);
            if (!orig)
            {
                return E_FAIL;
            }
            HRESULT hr = orig(self, cb, data);
            if (g_live && !g_dead)
            {
                if (InterlockedOr(&g_engaged, 0) != 0 && InterlockedOr(&g_driving, 0) != 0)
                {
                    float* veh_steer = g_vehicle_steer_ptr;
                    if (veh_steer != nullptr)
                    {
                        __try
                        {
                            *veh_steer = g_target;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                        }
                    }
                }
                if (!g_game_enum_done && InterlockedCompareExchange(&g_busy, 1, 0) == 0)
                {
                    __try
                    {
                        try_enum_game(self);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        g_dead = 1;
                    }
                    InterlockedExchange(&g_busy, 0);
                }
            }
            return hr;
        }

        HRESULT WINAPI hook_create(void* self, const GUID* guid, const DIEFFECT* eff, void** out, void* unk)
        {
            CreateFxFn orig = orig_create(self);
            if (!orig)
            {
                return E_FAIL;
            }
            HRESULT hr = orig(self, guid, eff, out, unk);
            if (g_live && !g_dead && SUCCEEDED(hr) && out && *out && guid)
            {
                __try
                {
                    hook_effect(*out);
                    int kind = fx_kind(guid);
                    if (kind != FFB_FX_CONSTANT)
                    {
                        SavedEffect* s = find_or_add_saved_fx(*out, kind);
                        if (s)
                        {
                            save_fx_params(s, eff, DIEP_TYPESPECIFICPARAMS | DIEP_GAIN);
                            if (InterlockedOr(&g_servoing, 0))
                            {
                                mute_single_fx(s);
                            }
                        }
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    g_dead = 1;
                }
            }
            return hr;
        }

        void patch_device_methods(void* dev)
        {
            void** vt = *reinterpret_cast<void***>(dev);
            for (int i = 0; i < 2; i++)
            {
                if (g_gds_vt[i] == vt)
                {
                    return;
                }
            }
            for (int i = 0; i < 2; i++)
            {
                if (!g_gds_vt[i])
                {
                    void* gds    = vt[VT_GET_STATE];
                    void* create = vt[VT_CREATE_EFFECT];
                    if (!in_dinput(gds) || !in_dinput(create))
                    {
                        return;
                    }
                    g_gds_orig[i] = reinterpret_cast<GdsFn>(gds);
                    g_gds_vt[i]   = vt;
                    g_ce_orig[i]  = reinterpret_cast<CreateFxFn>(create);
                    g_ce_vt[i]    = vt;
                    MemoryBarrier();
                    patch_slot(&vt[VT_GET_STATE], reinterpret_cast<void*>(hook_gds));
                    patch_slot(&vt[VT_CREATE_EFFECT], reinterpret_cast<void*>(hook_create));
                    return;
                }
            }
        }

        void prime_effect_vtable(void* dev)
        {
            SetFmtFn   set_fmt = reinterpret_cast<SetFmtFn>((*reinterpret_cast<void***>(dev))[VT_SET_FORMAT]);
            CreateFxFn create  = reinterpret_cast<CreateFxFn>((*reinterpret_cast<void***>(dev))[VT_CREATE_EFFECT]);
            if (!create)
            {
                log_raw("[ets2la_plugin][FfbProcessor] create-effect slot missing");
                return;
            }
            if (in_dinput(reinterpret_cast<void*>(set_fmt)))
            {
                set_fmt(dev, &c_dfDIJoystick);
            }
            DICONSTANTFORCE cf;
            DWORD           axis = DIJOFS_X;
            LONG            dir  = 0;
            DIEFFECT        eff;
            void*           fx = nullptr;
            ZeroMemory(&eff, sizeof(eff));
            cf.lMagnitude             = 0;
            eff.dwSize                = sizeof(eff);
            eff.dwFlags               = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
            eff.dwDuration            = INFINITE;
            eff.dwGain                = DI_FFNOMINALMAX;
            eff.dwTriggerButton       = DIEB_NOTRIGGER;
            eff.cAxes                 = 1;
            eff.rgdwAxes              = &axis;
            eff.rglDirection          = &dir;
            eff.cbTypeSpecificParams  = sizeof(cf);
            eff.lpvTypeSpecificParams = &cf;
            HRESULT hr                = create(dev, &k_constant_force, &eff, &fx, nullptr);
            if (FAILED(hr) || !fx)
            {
                log_hr("own constant force", hr);
                return;
            }
            hook_effect(fx);
            release_obj(fx);
        }

        BOOL WINAPI on_enum_dev(LPCDIDEVICEINSTANCEW inst, LPVOID ctx)
        {
            (void)ctx;
            if (!inst || g_ndev >= 8)
            {
                return DIENUM_CONTINUE;
            }
            g_devs[g_ndev] = inst->guidInstance;
            g_ndev++;
            char utf[160];
            char buf[240];
            if (WideCharToMultiByte(CP_UTF8, 0, inst->tszProductName, -1, utf, static_cast<int>(sizeof(utf)), nullptr, nullptr) > 0)
            {
                std::snprintf(buf, sizeof(buf), "[ets2la_plugin][FfbProcessor] saw controller: %s", utf);
                log_raw(buf);
            }
            return DIENUM_CONTINUE;
        }

        void open_and_patch(IDirectInput8W* di, const GUID* guid)
        {
            IDirectInputDevice8W* dev = nullptr;
            if (!di || !guid)
            {
                return;
            }
            HRESULT hr = di->CreateDevice(*guid, &dev, nullptr);
            if (FAILED(hr) || !dev)
            {
                log_hr("CreateDevice", hr);
                return;
            }
            prime_effect_vtable(dev);
            patch_device_methods(dev);
            release_obj(dev);
        }

        void install_hook()
        {
            IDirectInput8W* diw = nullptr;
            HRESULT         hr  = DirectInput8Create(GetModuleHandleW(nullptr), 0x0800, k_iid_di8_w, reinterpret_cast<void**>(&diw), nullptr);
            if (FAILED(hr) || !diw)
            {
                log_hr("DirectInput8Create", hr);
                return;
            }
            hr = diw->EnumDevices(DI8DEVCLASS_GAMECTRL, on_enum_dev, nullptr, DIEDFL_ATTACHEDONLY);
            if (g_ndev == 0)
            {
                hr = diw->EnumDevices(DI8DEVCLASS_ALL, on_enum_dev, nullptr, DIEDFL_ATTACHEDONLY);
            }
            for (int i = 0; i < g_ndev; i++)
            {
                open_and_patch(diw, &g_devs[i]);
            }
            release_obj(diw);
            if (g_ndev == 0)
            {
                log_raw("[ets2la_plugin][FfbProcessor] no controller enumerated");
            }
        }

        void unpatch_all()
        {
            update_other_effects_mute_state(false);
            g_live = 0;
            MemoryBarrier();
            for (int i = 0; i < g_npatches; i++)
            {
                DWORD old = 0;
                if (!g_patches[i].slot)
                {
                    continue;
                }
                if (VirtualProtect(g_patches[i].slot, sizeof(void*), PAGE_READWRITE, &old))
                {
                    *g_patches[i].slot = g_patches[i].original;
                    VirtualProtect(g_patches[i].slot, sizeof(void*), old, &old);
                }
            }
        }

        SCSAPI_VOID telemetry_on_steering(const scs_string_t name, const scs_u32_t index, const scs_value_t* const value, const scs_context_t context)
        {
            (void)name;
            (void)index;
            if (!value || value->type != SCS_VALUE_TYPE_float || !context)
            {
                return;
            }
            static_cast<FfbProcessor*>(context)->on_steering_input(value->value_float.value);
        }

        SCSAPI_VOID telemetry_on_drive_state(const scs_event_t event, const void* const event_info, const scs_context_t context)
        {
            (void)event_info;
            if (!context)
            {
                return;
            }
            static_cast<FfbProcessor*>(context)->on_drive_state(event == SCS_TELEMETRY_EVENT_started);
        }
    } // namespace

    FfbProcessor::FfbProcessor(scs_log_t scs_log) : scs_log_(scs_log)
    {
        g_log = scs_log;
    }

    FfbProcessor::~FfbProcessor()
    {
        destroy();
    }

    bool FfbProcessor::init(const scs_telemetry_init_params_v101_t* init_params)
    {
        g_live      = 1;
        g_dead      = 0;
        g_driving   = 1; // Default to 1 in case plugin loads while already driving; paused/started events refine it
        g_servo_dir = 1;
        QueryPerformanceFrequency(&g_qpf);
        ffb_servo_init(&g_servo);

        __try
        {
            install_hook();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            g_dead = 1;
        }

        if (init_params && init_params->register_for_event)
        {
            init_params->register_for_event(SCS_TELEMETRY_EVENT_paused, telemetry_on_drive_state, this);
            init_params->register_for_event(SCS_TELEMETRY_EVENT_started, telemetry_on_drive_state, this);
        }

        if (init_params && init_params->register_for_channel)
        {
            if (init_params->register_for_channel(
                    SCS_TELEMETRY_TRUCK_CHANNEL_input_steering,
                    SCS_U32_NIL,
                    SCS_VALUE_TYPE_float,
                    SCS_TELEMETRY_CHANNEL_FLAG_each_frame,
                    telemetry_on_steering,
                    this) == SCS_RESULT_ok)
            {
                InterlockedExchange(&g_steer_ok, 1);
            }
            else
            {
                this->warning("truck.input.steering channel unavailable, FFB servo disabled.");
            }
        }

        this->info("FFB processor initialized.");
        return true;
    }

    void FfbProcessor::destroy()
    {
        unpatch_all();
        g_log = nullptr;
    }

    bool FfbProcessor::is_available() const
    {
        if (!g_live || g_dead || !g_set_orig[0])
        {
            return false;
        }
        return cf_fresh() && steer_fresh();
    }

    void FfbProcessor::set_steering_target(float target_angle, bool engaged)
    {
        if (!engaged || !(target_angle == target_angle))
        {
            InterlockedExchange(&g_engaged, 0);
            return;
        }

        g_target       = target_angle;
        g_target_rate  = 0.0f;
        g_target_accel = 0.0f;
        g_target_stamp++;
        InterlockedExchange(&g_engaged, 1);
    }

    void FfbProcessor::set_vehicle_steering_ptr(float* ptr)
    {
        InterlockedExchangePointer(reinterpret_cast<PVOID volatile*>(&g_vehicle_steer_ptr), ptr);
    }

    void FfbProcessor::on_steering_input(float v)
    {
        static LARGE_INTEGER last = {};
        static float         prev = 0.0f;

        if (!(v == v))
        {
            return;
        }

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double dt = (last.QuadPart && g_qpf.QuadPart)
                        ? static_cast<double>(now.QuadPart - last.QuadPart) / static_cast<double>(g_qpf.QuadPart)
                        : 0.0;

        if (dt > 0.0 && dt < 0.2)
        {
            double raw        = static_cast<double>(v - prev) / dt;
            g_steer_rate      = static_cast<float>(static_cast<double>(g_steer_rate) + (raw - static_cast<double>(g_steer_rate)) * (1.0 - std::exp(-dt / FFB_STEER_RATE_TAU_S)));
            g_steer_rate_fast = static_cast<float>(static_cast<double>(g_steer_rate_fast) + (raw - static_cast<double>(g_steer_rate_fast)) * (1.0 - std::exp(-dt / FFB_RATE_FAST_TAU_S)));
            g_steer_interval  = g_steer_interval > 0.0f
                                    ? static_cast<float>(static_cast<double>(g_steer_interval) + (dt - static_cast<double>(g_steer_interval)) * 0.05)
                                    : static_cast<float>(dt);
        }
        else
        {
            g_steer_rate      = 0.0f;
            g_steer_rate_fast = 0.0f;
        }

        if (v != prev || g_steer_moved_at.QuadPart == 0)
        {
            g_steer_moved_at = now;
        }
        if (last.QuadPart && v != prev)
        {
            g_settle_step = static_cast<float>(ffb_settle_step(static_cast<double>(g_settle_step), std::fabs(static_cast<double>(v) - static_cast<double>(prev))));
        }

        last       = now;
        prev       = v;
        g_steer    = v;
        g_steer_at = now;
        InterlockedIncrement(&g_steer_count);
    }

    void FfbProcessor::on_drive_state(bool driving)
    {
        InterlockedExchange(&g_driving, driving ? 1 : 0);
    }

    void FfbProcessor::tick()
    {
        static bool logged_hook  = false;
        static bool logged_mag   = false;
        static bool logged_dead  = false;
        static int  logged_servo = 0;

        if (g_dead && !logged_dead)
        {
            logged_dead = true;
            unpatch_all();
            this->error("DirectInput hook faulted; FFB disabled, falling back to memory steering.");
        }
        if (!logged_hook && g_set_orig[0])
        {
            logged_hook = true;
            this->info("Constant force effect hooked.");
        }
        if (!logged_mag && g_have_mag)
        {
            logged_mag = true;
            this->info("Game FFB force active (initial sample: {}), FFB steering available.", static_cast<long>(g_first_mag));
        }
        int servo = g_servoing ? 1 : 0;
        if (servo != logged_servo)
        {
            logged_servo = servo;
            this->info(servo ? "FFB steering servo engaged (other game effects muted)." : "FFB steering servo disengaged (game effects restored).");
        }
    }
}
