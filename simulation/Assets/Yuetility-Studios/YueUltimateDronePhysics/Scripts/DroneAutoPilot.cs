using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class DroneAutoPilot : MonoBehaviour
{
    [Header("Flight Parameters")]
    [Tooltip("Desired altitude for takeoff (in meters)")]
    public float takeoffAltitude = 10f;
    
    [Tooltip("Side length (in meters) of the square path")]
    public float squareSideLength = 10f;
    
    [Tooltip("Acceleration factor for moving towards the target")]
    public float acceleration = 10f;
    
    [Tooltip("Damping factor to smooth the movement")]
    public float dampingFactor = 2f;
    
    [Tooltip("Distance tolerance when reaching a waypoint")]
    public float waypointTolerance = 1f;
    
    private Rigidbody rb;
    private Vector3 initialPosition;
    private Vector3 targetPosition;
    
    private enum FlightPhase { Takeoff, FlySquare, Hover }
    private FlightPhase currentPhase = FlightPhase.Takeoff;
    
    void Start()
    {
        rb = GetComponent<Rigidbody>();
        initialPosition = transform.position;
        targetPosition = new Vector3(initialPosition.x, takeoffAltitude, initialPosition.z);
        
        var dronePhysics = GetComponent<YueUltimateDronePhysics.YueDronePhysics>();
        if (dronePhysics != null)
        {
            dronePhysics.enabled = false;
        }
        var inputModule = GetComponent<YueUltimateDronePhysics.YueInputModule>();
        if (inputModule != null)
        {
            inputModule.enabled = false;
        }
        
        StartCoroutine(ExecuteFlightPlan());
    }
    
    IEnumerator ExecuteFlightPlan()
    {
        currentPhase = FlightPhase.Takeoff;
        while (Mathf.Abs(transform.position.y - takeoffAltitude) > 0.5f)
        {
            yield return new WaitForFixedUpdate();
        }
        
        currentPhase = FlightPhase.FlySquare;
        
        Vector3 startPosition = new Vector3(initialPosition.x, takeoffAltitude, initialPosition.z);
        List<Vector3> waypoints = new List<Vector3>
        {
            startPosition + new Vector3(0, 0, squareSideLength),                   // Move forward
            startPosition + new Vector3(squareSideLength, 0, squareSideLength),        // Turn right
            startPosition + new Vector3(squareSideLength, 0, 0),                     // Turn right again
            startPosition                                                          // Return to start position
        };
        
        foreach (Vector3 wp in waypoints)
        {
            targetPosition = wp;
            while (Vector3.Distance(transform.position, targetPosition) > waypointTolerance)
            {
                yield return new WaitForFixedUpdate();
            }
        }
        
        currentPhase = FlightPhase.Hover;
        targetPosition = transform.position;
    }
    
    void FixedUpdate()
    {
        Vector3 error = targetPosition - transform.position;
        Vector3 desiredForce = error.normalized * acceleration;
        desiredForce -= rb.linearVelocity * dampingFactor;
        rb.AddForce(desiredForce, ForceMode.Acceleration);
    }
}
