#include "listener.hpp"


const uint16_t POSE_PORT = 20001;

// ------------------------------------------------------------------
// Допоміжна функція — відкрити TCP-слухач
int listenOn(uint16_t port)
{
    int s = socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in a{}; a.sin_family=AF_INET;
    a.sin_port=htons(port); a.sin_addr.s_addr=INADDR_ANY;
    bind(s,(sockaddr*)&a,sizeof(a));
    listen(s,4);
    std::cout<<"[SERV] listening "<<port<<"\n";
    return s;
}

// ------------------------------------------------------------------
// Потік: позиції дрона (TCP 20001)
void poseThread()
{
    std::cout << "[POSE] Starting pose thread...\n";
    int srv = listenOn(POSE_PORT);
    for(;;)
    {
        int cli = accept(srv,nullptr,nullptr);
        std::thread([cli]{
            char buf[12];
            while(recv(cli,buf,12,MSG_WAITALL)==12)
            {
                float x,y,z;
                memcpy(&x,buf,4); memcpy(&y,buf+4,4); memcpy(&z,buf+8,4);
                std::cout<<"POSE "<<x<<" "<<y<<" "<<z<<"\n";
            }
            CLOSESOCK(cli);
        }).detach();
    }
}


void mainThread()
{
    const char* serverIP = "127.0.0.1";
    const int port = 11000;

    std::string input;
    std::vector<std::string> messages = {
            "100,100,100",
            "200,200,200",
            "300,300,300",
            "400,400,400",
            "500,500,500",
            "600,600,600",
            "700,700,700",
            "800,800,800",
    };

    for (const auto& message : messages) {
        int sock;
        while (true) {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                std::cerr << "Error creating socket." << std::endl;
                continue;
            }

            struct sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(port);
            serverAddr.sin_addr.s_addr = inet_addr(serverIP);

            if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
//                std::cerr << "Error connecting to server." << std::endl;
    #ifdef _WIN32
                closesocket(sock);
    #else
                close(sock);
    #endif
                continue;
            }
            break;
        }

        int bytesSent = send(sock, message.c_str(), message.size(), 0);
        if (bytesSent < 0) {
            std::cerr << "Error sending data." << std::endl;
        } else {
            std::cout << "Sent: " << message << std::endl;
        }

#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }

#ifdef _WIN32
    WSACleanup();
#endif
}


void imageThread() {
    std::string watchDir = "/Users/ksushakretsula/Documents/Year_3/Semester_2/CPP/FINAL_PROJECT/drove_navigation/received_images";
    std::set<std::string> processed;
    std::cout << "Watching folder: " << watchDir << std::endl;

    while (true) {
        std::vector<fs::path> images;

        for (const auto& entry : fs::directory_iterator(watchDir)) {
            if (!entry.is_regular_file()) continue;
            const fs::path& path = entry.path();

            if (processed.find(path.filename().string()) == processed.end()) {
                images.push_back(path);
            }
        }

        // Sort files like image0001.jpg, image0002.jpg, ...
        std::sort(images.begin(), images.end(), [](const fs::path& a, const fs::path& b) {
            return a.filename().string() < b.filename().string();
        });

        for (auto& img : images) {
            processImage(img, ModelType::MiDaS);
            processed.insert(img.filename().string());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
