using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

public class DroneSimulatorInterop : MonoBehaviour
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Vector3Interop
    {
        public float x;
        public float y;
        public float z;

        public override string ToString() => $"({x:F2}, {y:F2}, {z:F2})";
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct DroneStateInterop
    {
        public Vector3Interop pos;
        public Vector3Interop vel;
    }

#if UNITY_STANDALONE_OSX || UNITY_EDITOR_OSX
    const string DLL_NAME = "DroneDynamicsDLL";
#elif UNITY_STANDALONE_WIN
    const string DLL_NAME = "DroneDynamicsDLL.dll";
#else
    const string DLL_NAME = "DroneDynamicsDLL";
#endif

    [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
    public static extern void SimulateStep(
        ref DroneStateInterop currentState,
        ref Vector3Interop target,
        float dt,
        float thrust,
        float mass,
        float drag,
        float kp,
        float kd,
        out DroneStateInterop newState);

    [Header("DLL Simulation & Controller Parameters")]
    public float thrust = 100f;    // Maximum thrust [N] passed to the DLL.
    public float mass = 1f;        // Drone mass [kg].
    public float drag = 0.1f;      // Linear drag coefficient.
    public float kp = 10f;         // Proportional gain for the DLL simulation.
    public float kd = 2f;          // Damping factor for the DLL simulation.
    public float dt = 0.02f;       // Simulation timestep [seconds].

    [Header("Autopilot Force Parameters")]
    public float acceleration = 10f;    // Correction acceleration used to drive error correction.
    public float dampingFactor = 2f;    // Damping factor used with the Rigidbody’s velocity.
    public float tolerance = 0.5f;      // Tolerance (in world units) for reaching a target.

    private DroneStateInterop currentState;

    public List<Vector3Interop> targets = new List<Vector3Interop>();
    private int currentTargetIndex = 0;

    private bool returningToHome = false;

    private Vector3Interop initialPos;

    private Rigidbody rb;

    void Start()
    {
        rb = GetComponent<Rigidbody>();
        if (rb == null)
        {
            Debug.LogError("Rigidbody not found! Please add a Rigidbody to the drone GameObject.");
            enabled = false;
            return;
        }

        rb.freezeRotation = true;

        Vector3 pos = transform.position;
        currentState.pos = new Vector3Interop { x = pos.x, y = pos.y, z = pos.z };
        currentState.vel = new Vector3Interop { x = 0f, y = 0f, z = 0f };

        initialPos = currentState.pos;
    }

    void FixedUpdate()
    {
        Vector3Interop target;

        if (returningToHome)
        {
            target = initialPos;
        }
        else if (targets.Count > 0)
        {
            if (currentTargetIndex < targets.Count)
            {
                target = targets[currentTargetIndex];
                Vector3 unityTarget = new Vector3(target.x, target.y, target.z);
                if (Vector3.Distance(transform.position, unityTarget) < tolerance)
                {
                    Debug.Log($"Target {currentTargetIndex} reached: {unityTarget}");
                    currentTargetIndex++;
                }
            }
            else
            {
                target = targets[targets.Count - 1];
            }
        }
        else
        {
            Vector3 pos = transform.position;
            target = new Vector3Interop { x = pos.x, y = pos.y, z = pos.z };
        }

        DroneStateInterop simulatedState;
        SimulateStep(ref currentState, ref target, dt, thrust, mass, drag, kp, kd, out simulatedState);
        currentState = simulatedState;

        Vector3 desiredPos = new Vector3(simulatedState.pos.x, simulatedState.pos.y, simulatedState.pos.z);
        Vector3 error = desiredPos - transform.position;

        Vector3 desiredForce = error.normalized * acceleration;
        desiredForce -= rb.linearVelocity * dampingFactor;

        Vector3 gravityCompensation = -Physics.gravity;

        rb.AddForce(gravityCompensation + desiredForce, ForceMode.Acceleration);

        transform.rotation = Quaternion.identity;
        rb.angularVelocity = Vector3.zero;
        if (returningToHome)
        {
            Vector3 unityHome = new Vector3(initialPos.x, initialPos.y, initialPos.z);
            if (Vector3.Distance(transform.position, unityHome) < tolerance)
            {
                Debug.Log("Returned to starting position. Drone disarmed.");
                enabled = false;
            }
        }
    }

    public void AddTargetFromMessage(string message)
    {
        if (string.IsNullOrEmpty(message))
            return;

        string trimmed = message.Trim();
        if (trimmed.ToLower() == "home")
        {
            targets.Clear();
            currentTargetIndex = 0;
            returningToHome = true;
            Debug.Log("Received 'home' command. Returning to starting position.");
        }
        else
        {
            char[] separators = new char[] { ',', ';', ' ' };
            string[] tokens = trimmed.Split(separators, System.StringSplitOptions.RemoveEmptyEntries);
            if (tokens.Length >= 3)
            {
                if (float.TryParse(tokens[0], out float x) &&
                    float.TryParse(tokens[1], out float y) &&
                    float.TryParse(tokens[2], out float z))
                {
                    Vector3Interop newTarget = new Vector3Interop { x = x, y = y, z = z };
                    targets.Add(newTarget);
                    returningToHome = false;
                    Debug.Log("Added new target: " + newTarget.ToString());
                }
                else
                {
                    Debug.LogWarning("Unable to parse target coordinates from message: " + message);
                }
            }
            else
            {
                Debug.LogWarning("Invalid target format received: " + message);
            }
        }
    }
}
