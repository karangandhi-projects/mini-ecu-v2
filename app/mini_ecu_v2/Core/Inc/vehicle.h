/**
 * @file    vehicle.h
 * @brief   Virtual "true physical" vehicle model for the Mini ECU project.
 *
 * This module represents the *physical world* as the ECU would experience it.
 * It simulates:
 *   - vehicle speed (km/h)
 *   - engine RPM
 *   - coolant temperature (°C)
 *
 * The rest of the ECU *must not* read or change these values directly.
 * Instead, virtual sensors (added later) will "measure" these values and
 * provide ADC/pulse/timer readings just like real hardware.
 *
 * The purpose of this module:
 *   1. Be a plant model for SIL/HIL-style testing.
 *   2. Provide predictable and tunable behavior for virtual sensor inputs.
 *   3. Allow test harnesses to inject special conditions (faults, edges).
 */

#ifndef VEHICLE_H
#define VEHICLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Represents the "true" underlying physical state of the simulated vehicle.
 *
 * These values should never be accessed directly by application logic.
 * They represent physics-level truth, not what sensors actually report.
 */
typedef struct
{
    float    speed_kph;        /**< Physical vehicle speed in km/h. */
    uint16_t engine_rpm;       /**< Physical crankshaft RPM (0–6000 typical). */
    float    coolant_temp_c;   /**< Physical coolant temperature in °C. */
} VehicleState_t;

/**
 * @brief Initialize the vehicle state to realistic "engine on, idle" values.
 *
 * Default startup assumption (similar to idle in a parked real vehicle):
 *   - speed        = 0 km/h
 *   - engine_rpm   = 800 RPM
 *   - coolant_temp = 30 °C (cold engine)
 *
 * @param[out] vs Pointer to a VehicleState_t instance to initialize.
 */
void Vehicle_Init(VehicleState_t *vs);

/**
 * @brief Advance the simulated vehicle's physics by a time step.
 *
 * This function implements a *very* simple dynamic model:
 *
 * Speed Behavior:
 *   - If no target is being enforced, speed naturally decays (coast-down).
 *
 * RPM Behavior:
 *   - RPM follows speed with a first-order lag (imitates engine inertia).
 *   - RPM is clamped between safe limits (600 → 6000 rpm).
 *
 * Coolant Temperature Behavior:
 *   - Temperature gradually warms toward ~90°C when engine is active.
 *   - It cools slightly when stationary or idling for long.
 *
 * These values form the "ground truth" that virtual sensors will sample.
 *
 * @param[in,out] vs   Vehicle state to update.
 * @param[in]     dt_s Time step in seconds (e.g., 0.1 for 10Hz updates).
 */
void Vehicle_Update(VehicleState_t *vs, float dt_s);

/**
 * @brief Directly set the vehicle's physical speed (km/h).
 *
 * This is primarily used by CLI commands or test harnesses
 * to inject external conditions into the physical world model.
 *
 * Speed is clamped to [0, 200] km/h.
 *
 * @param[in,out] vs Pointer to the vehicle state.
 * @param[in]     speed_kph Desired physical speed.
 */
void Vehicle_SetTargetSpeed(VehicleState_t *vs, float speed_kph);

/**
 * @brief Forcefully override all physical quantities.
 *
 * This bypasses all physics and is used strictly for:
 *   - Fault injection
 *   - Unit testing
 *   - Crash injection scenarios
 *   - Sensor plausibility testing
 *
 * The values are applied directly without filtering.
 *
 * @param[in,out] vs      Vehicle state to manipulate.
 * @param[in]     speed   New physical speed (km/h).
 * @param[in]     rpm     New engine RPM.
 * @param[in]     temp_c  New coolant temperature (°C).
 */
void Vehicle_Force(VehicleState_t *vs, float speed, uint16_t rpm, float temp_c);

#ifdef __cplusplus
}
#endif

#endif /* VEHICLE_H */
