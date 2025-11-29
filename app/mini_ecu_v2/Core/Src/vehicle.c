/**
 * @file    vehicle.c
 * @brief   Implementation of the "true physical" vehicle dynamics model.
 *
 * This is NOT a sensor layer. These values represent the ground truth:
 * the real temperature, real speed, real RPM. In future phases, a
 * virtual sensor layer will sample these values and convert them into
 * ADC counts, timer pulses, etc.
 *
 * WHY THIS MODULE EXISTS:
 * -----------------------
 * This keeps the ECU design clean and realistic:
 *   App Logic <— Virtual Sensors <— Physical Model (this file)
 *
 * This separation allows:
 *   - fault/edge-case injection
 *   - unit tests for physical behavior
 *   - SIL-style simulation
 *   - better logging and crash dump interpretation
 */

#include "vehicle.h"

/* -------------------------------------------------------------
 * Helper: float clamp
 *
 * Used to keep quantities within realistic physical ranges.
 * This prevents faults or test injections from pushing the
 * physics engine into absurd or undefined states.
 * ------------------------------------------------------------- */
static float clamp_f(float v, float min, float max)
{
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

void Vehicle_Init(VehicleState_t *vs)
{
    if (!vs) return;

    vs->speed_kph        = 0.0f;
    vs->engine_rpm       = 800;
    vs->coolant_temp_c   = 30.0f;  /* Cold engine start */
}

/* -------------------------------------------------------------
 * Physics Model Explanation
 * -------------------------------------------------------------
 *
 * SPEED:
 *   - Natural decay proportional to current speed (rolling resistance).
 *   - Hard-clamped between 0 and 200 km/h.
 *
 * RPM:
 *   - Follows speed using a simple linear map:
 *         target_rpm = 800 + (speed * 50)
 *     (50 RPM per km/h gives 5800 RPM at 100 km/h, realistic enough)
 *
 *   - First-order lag to simulate engine inertia:
 *         rpm += (target_rpm - rpm) * 0.3
 *
 *   - Clamped between 600 and 6000 rpm.
 *
 * COOLANT TEMP:
 *   - Gradually rises toward ~90°C at rate proportional to engine load.
 *   - Slowly cools if the engine is below operating RPM and stationary.
 -------------------------------------------------------------- */
void Vehicle_Update(VehicleState_t *vs, float dt_s)
{
    if (!vs || dt_s <= 0.0f) return;

    /* --- 1. SPEED DECAY ------------------------------------ */
    /* Very simple model: coast down by 2% per update. */
    vs->speed_kph *= 0.98f;
    vs->speed_kph = clamp_f(vs->speed_kph, 0.0f, 200.0f);

    /* --- 2. RPM FOLLOWS SPEED ------------------------------ */
    float target_rpm = 800.0f + (vs->speed_kph * 50.0f);

    /* Engine inertia modeled as 30% convergence per update. */
    vs->engine_rpm += (uint16_t)((target_rpm - vs->engine_rpm) * 0.3f);

    vs->engine_rpm = (uint16_t)clamp_f(vs->engine_rpm, 600.0f, 6000.0f);

    /* --- 3. COOLANT TEMPERATURE ---------------------------- */
    float warmup_target = 90.0f;  /* Typical operating temp */

    if (vs->engine_rpm > 1000)
    {
        /* Warm up faster when engine is loaded */
        vs->coolant_temp_c += (warmup_target - vs->coolant_temp_c) * 0.05f;
    }
    else
    {
        /* Slight cooling when idling */
        vs->coolant_temp_c -= 0.01f;
    }

    vs->coolant_temp_c = clamp_f(vs->coolant_temp_c, 20.0f, 110.0f);
}

void Vehicle_SetTargetSpeed(VehicleState_t *vs, float speed_kph)
{
    if (!vs) return;

    vs->speed_kph = clamp_f(speed_kph, 0.0f, 200.0f);
}

void Vehicle_Force(VehicleState_t *vs, float speed, uint16_t rpm, float temp_c)
{
    if (!vs) return;

    /* Direct injection — no physics, no filtering */
    vs->speed_kph      = clamp_f(speed,        0.0f, 200.0f);
    vs->engine_rpm     = (uint16_t)clamp_f(rpm, 600.0f, 6000.0f);
    vs->coolant_temp_c = clamp_f(temp_c,       20.0f, 110.0f);
}
