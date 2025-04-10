using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Networking;

[System.Serializable]
public class MyRect
{
    public float x;
    public float y;
    public float w;
    public float h;
}

public class PostCameraView : MonoBehaviour
{
    [Header("Image Capture Settings")]
    public int height = 320;
    public float k = 0.25f;
    
    // Capture interval in seconds (e.g., 0.1 for 10 captures per second)
    [Header("Capture Interval Settings")]
    public float captureInterval = 0.1f;

    [Header("Drone Movement Settings")]
    public float moveForce = 10f;
    // You can assign your drone’s Rigidbody in the Inspector.
    // If left unassigned the script will try to get the Rigidbody attached to the same GameObject.
    public Rigidbody controlledRigidBody = null;
    
    private Vector3 mousePos;
    private bool restartTracker = false; 
    public GUIStyle style = new GUIStyle();
    
    MyRect coords;

    void Start()
    {
        style.normal.textColor = Color.red;
        
        // Automatically get the Rigidbody if not assigned.
        if (controlledRigidBody == null)
        {
            controlledRigidBody = GetComponent<Rigidbody>();
        }
        
        // Start continuously capturing and sending images.
        StartCoroutine(CaptureAndSendCoroutine());
    }

    void OnGUI()
    {
        // Display the drone’s current height
        GUI.Label(new Rect(10, 10, 150, 20), "Height = " + transform.position.y, style);

        // If a region has been returned from the server then draw the overlay
        if (coords != null)
        {
            Color overlayColor = Color.green;
            overlayColor.a = 0.5f;
            Color oldColor = GUI.color;
            GUI.color = overlayColor;
            GUI.DrawTexture(new Rect(coords.x * Screen.width,
                                     coords.y * Screen.height,
                                     coords.w * Screen.width,
                                     coords.h * Screen.height), Texture2D.whiteTexture);
            GUI.color = oldColor;
        }
    }

    void Update()
    {
        // --- Input for image capture ---
        // When the left mouse button is pressed, record its normalized position 
        if (Input.GetKeyDown(KeyCode.Mouse0))
        {
            mousePos = Input.mousePosition;
            mousePos.x /= Screen.width;
            mousePos.y /= Screen.height;
            restartTracker = true;
        }
        
        // --- (Optional) Additional input could be added here ---
    }

    IEnumerator CaptureAndSendCoroutine()
    {
        while (true)
        {
            // Capture and send the image.
            yield return RenderNextFrame();
            
            // Wait for the defined capture interval before capturing the next image.
            yield return new WaitForSeconds(captureInterval);
        }
    }

    IEnumerator RenderNextFrame()
    {
        // Wait for all rendering to complete
        yield return new WaitForEndOfFrame();

        // Capture the full screen image
        Texture2D screenTexture = new Texture2D(Screen.width, Screen.height, TextureFormat.RGB24, false);
        Rect fullRect = new Rect(0, 0, Screen.width, Screen.height);
        screenTexture.ReadPixels(fullRect, 0, 0);
        screenTexture.Apply();

        // Resize the captured image to the desired height
        Texture2D resizedTexture = Resize(screenTexture, Screen.width * height / Screen.height, height);
        Destroy(screenTexture);

        // Convert the resized texture to a JPG byte array
        byte[] imageData = resizedTexture.EncodeToJPG();
        Destroy(resizedTexture);

        // Create a form to send the image and additional data
        WWWForm form = new WWWForm();
        form.AddBinaryData("file", imageData);
        form.AddField("x", mousePos.x.ToString());
        // Adjust the y-coordinate so that it is relative to the top of the screen
        form.AddField("y", (1.0f - mousePos.y).ToString());
        form.AddField("restart", (restartTracker ? 1 : 0).ToString());
        restartTracker = false;

        UnityWebRequest www = UnityWebRequest.Post("http://localhost:20000", form);
        DownloadHandlerBuffer buffer = new DownloadHandlerBuffer();
        www.downloadHandler = buffer;
        yield return www.SendWebRequest();

        if (www.result != UnityWebRequest.Result.Success)
        {
            Debug.Log(www.error);
        }
        else
        {
            Debug.Log(buffer.text);

            // Parse the returned JSON data for display coordinates
            string jsonString = buffer.text;
            coords = JsonUtility.FromJson<MyRect>(jsonString);
            Debug.Log("Parsed x: " + coords.x + ", y: " + coords.y +
                      ", w: " + coords.w + ", h: " + coords.h);
        }
    }

    Texture2D Resize(Texture2D originalTexture, int targetWidth, int targetHeight)
    {
        RenderTexture rt = new RenderTexture(targetWidth, targetHeight, 24);
        RenderTexture.active = rt;
        Graphics.Blit(originalTexture, rt);
        Texture2D resultTexture = new Texture2D(targetWidth, targetHeight);
        resultTexture.ReadPixels(new Rect(0, 0, targetWidth, targetHeight), 0, 0);
        resultTexture.Apply();
        RenderTexture.active = null;
        rt.Release();
        return resultTexture;
    }
}
