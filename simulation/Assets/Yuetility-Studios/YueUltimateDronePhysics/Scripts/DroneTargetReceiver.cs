using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using UnityEngine;

public class DroneTargetReceiver : MonoBehaviour
{
    [Tooltip("Port to listen for incoming target messages.")]
    public int port = 11000;

    public DroneSimulatorInterop simulation;

    private TcpListener listener;
    private Thread listenerThread;
    private volatile bool running = false;

    private Queue<string> receivedMessages = new Queue<string>();
    private readonly object queueLock = new object();

    void Start()
    {
        if (simulation == null)
        {
            Debug.LogError("DroneTargetReceiver: simulation reference is not assigned.");
            return;
        }

        running = true;
        listenerThread = new Thread(ListenForTargets);
        listenerThread.IsBackground = true;
        listenerThread.Start();
        Debug.Log($"DroneTargetReceiver: Listening for target data on port {port}.");
    }

    void OnDestroy()
    {
        running = false;
        if (listener != null)
            listener.Stop();
        if (listenerThread != null && listenerThread.IsAlive)
            listenerThread.Abort();
    }

    private void ListenForTargets()
    {
        try
        {
            listener = new TcpListener(IPAddress.Any, port);
            listener.Start();
            while (running)
            {
                if (listener.Pending())
                {
                    using (TcpClient client = listener.AcceptTcpClient())
                    using (NetworkStream stream = client.GetStream())
                    {
                        byte[] buffer = new byte[1024];
                        int bytesRead = stream.Read(buffer, 0, buffer.Length);
                        if (bytesRead > 0)
                        {
                            string message = Encoding.UTF8.GetString(buffer, 0, bytesRead);
                            Debug.Log("DroneTargetReceiver: Received message: " + message);
                            lock (queueLock)
                            {
                                receivedMessages.Enqueue(message);
                            }
                        }
                    }
                }
                Thread.Sleep(10); // Prevent busy waiting.
            }
        }
        catch (SocketException ex)
        {
            Debug.LogError("DroneTargetReceiver: SocketException: " + ex);
        }
        catch (Exception ex)
        {
            Debug.LogError("DroneTargetReceiver: Exception: " + ex);
        }
    }

    void Update()
    {
        lock (queueLock)
        {
            while (receivedMessages.Count > 0)
            {
                string msg = receivedMessages.Dequeue();
                ProcessMessage(msg);
            }
        }
    }

    private void ProcessMessage(string message)
    {
        simulation.AddTargetFromMessage(message);
    }
}
