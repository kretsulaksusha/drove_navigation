#ifdef _WIN32
  #define DLL_EXPORT extern "C" __declspec(dllexport)
#else
  #define DLL_EXPORT extern "C"
#endif

#include <cmath>

struct Vector3 {
    float x;
    float y;
    float z;
};

struct DroneState {
    Vector3 pos;  // Position in meters.
    Vector3 vel;  // Velocity in m/s.
};

// Helper functions for Vector3 math
inline Vector3 add(const Vector3 &a, const Vector3 &b) {
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}

inline Vector3 subtract(const Vector3 &a, const Vector3 &b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}

inline Vector3 multiply(const Vector3 &a, float s) {
    return { a.x * s, a.y * s, a.z * s };
}

inline Vector3 divide(const Vector3 &a, float s) {
    return { a.x / s, a.y / s, a.z / s };
}

inline float magnitude(const Vector3 &a) {
    return std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}

inline Vector3 normalize(const Vector3 &a) {
    float mag = magnitude(a);
    if (mag > 0)
        return divide(a, mag);
    return { 0, 0, 0 };
}

// Helper to add two DroneState instances.
inline DroneState addState(const DroneState &a, const DroneState &b) {
    DroneState result;
    result.pos = add(a.pos, b.pos);
    result.vel = add(a.vel, b.vel);
    return result;
}

// Helper to multiply the components of DroneState by a scalar.
inline DroneState multiplyState(const DroneState &state, float s) {
    DroneState result;
    result.pos = multiply(state.pos, s);
    result.vel = multiply(state.vel, s);
    return result;
}

//-------------------------
// Compute the derivatives of the state.
// Here we use a simple PD controller for translation in 3D.
// Parameters:
//   - kp: Proportional gain.
//   - kd: Derivative gain.
//   - drag: Additional drag coefficient.
//   - mass: Drone’s mass (kg).
//   - thrust: Maximum allowed force (N) [used to clamp the control input].
DroneState computeDerivatives(const DroneState &state, const Vector3 &target,
                              float kp, float kd, float drag, float mass, float thrust) {
    DroneState ds;
    ds.pos = state.vel;

    Vector3 error = subtract(target, state.pos);
    // PD control: force = kp * error - (kd + drag) * velocity.
    Vector3 control = subtract(multiply(error, kp), multiply(state.vel, (kd + drag)));

    float forceMag = magnitude(control);
    if (forceMag > thrust) {
        control = multiply(normalize(control), thrust);
    }
    ds.vel = divide(control, mass);
    return ds;
}

//-------------------------
// Exposed function: SimulateStep
// Uses RK4 integration to update the drone state over one time step dt.
// Inputs:
//   currentState - pointer to current DroneState (position and velocity)
//   target       - pointer to the target position vector
//   dt           - time step (seconds)
//   thrust       - maximum thrust [N] (from physicsConfig.thrust)
//   mass         - mass [kg] (from physicsConfig.mass)
//   drag         - linear drag (from physicsConfig.drag)
//   kp           - proportional gain (e.g., for position control or altitude hold)
//   kd           - derivative gain (e.g., for position control or altitude hold)
// Output:
//   newState - pointer where the updated DroneState will be stored.
DLL_EXPORT void SimulateStep(DroneState* currentState, const Vector3* target, float dt,
                             float thrust, float mass, float drag, float kp, float kd,
                             DroneState* newState)
{
    DroneState s = *currentState;

    // RK4 integration using our derivative function.
    DroneState k1 = computeDerivatives(s, *target, kp, kd, drag, mass, thrust);
    k1 = multiplyState(k1, dt);

    DroneState temp;
    temp.pos = add(s.pos, multiply(k1.pos, 0.5f));
    temp.vel = add(s.vel, multiply(k1.vel, 0.5f));
    DroneState k2 = computeDerivatives(temp, *target, kp, kd, drag, mass, thrust);
    k2 = multiplyState(k2, dt);

    temp.pos = add(s.pos, multiply(k2.pos, 0.5f));
    temp.vel = add(s.vel, multiply(k2.vel, 0.5f));
    DroneState k3 = computeDerivatives(temp, *target, kp, kd, drag, mass, thrust);
    k3 = multiplyState(k3, dt);

    temp.pos = add(s.pos, k3.pos);
    temp.vel = add(s.vel, k3.vel);
    DroneState k4 = computeDerivatives(temp, *target, kp, kd, drag, mass, thrust);
    k4 = multiplyState(k4, dt);

    Vector3 sumPos = add(add(k1.pos, multiply(k2.pos, 2.0f)),
                         add(multiply(k3.pos, 2.0f), k4.pos));
    Vector3 sumVel = add(add(k1.vel, multiply(k2.vel, 2.0f)),
                         add(multiply(k3.vel, 2.0f), k4.vel));

    Vector3 incPos = multiply(sumPos, 1.0f / 6.0f);
    Vector3 incVel = multiply(sumVel, 1.0f / 6.0f);

    newState->pos = add(s.pos, incPos);
    newState->vel = add(s.vel, incVel);
}
