#include "processing/ffb/ffb_processor.hpp"

namespace ets2la_plugin
{
    FfbProcessor::FfbProcessor(scs_log_t scs_log) : scs_log_(scs_log)
    {
    }

    FfbProcessor::~FfbProcessor()
    {
        destroy();
    }

    bool FfbProcessor::init(const scs_telemetry_init_params_v101_t* init_params)
    {
        (void)init_params;
        return true;
    }

    void FfbProcessor::destroy()
    {
    }

    void FfbProcessor::tick()
    {
    }

    bool FfbProcessor::is_available() const
    {
        return false;
    }

    void FfbProcessor::set_steering_target(float target_angle, bool engaged)
    {
        (void)target_angle;
        (void)engaged;
    }

    void FfbProcessor::set_vehicle_steering_ptr(float* ptr)
    {
        (void)ptr;
    }

    void FfbProcessor::on_steering_input(float value)
    {
        (void)value;
    }

    void FfbProcessor::on_drive_state(bool driving)
    {
        (void)driving;
    }
}
