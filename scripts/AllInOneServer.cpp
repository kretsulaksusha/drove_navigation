/*  AllInOneServer.cpp  */
#include <iostream>
#include <vector>
#include <thread>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <atomic>
#include "httplib.h"                 // завантажити з https://github.com/yhirose/cpp-httplib
#ifdef _WIN32
#include <winsock2.h>
 #pragma comment(lib,"ws2_32.lib")
 using socklen_t = int;
 #define CLOSESOCK closesocket
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#define CLOSESOCK close
#endif
namespace fs = std::filesystem;

// ------------------------------------------------------------------
// Константи
const uint16_t HTTP_PORT = 20000;      // кадри
const uint16_t POSE_PORT = 20001;      // позиції
const uint16_t MAP_PORT  = 13000;      // карта / патчі
const std::string SAVE_DIR = "received_images";

// ------------------------------------------------------------------
// Обслуговування файлових імен
std::atomic<int> fileCounter{1};
std::string nextFilename()
{
    int n = fileCounter.fetch_add(1);
    char buf[32]; std::snprintf(buf,sizeof(buf),"image%04d.jpg",n);
    return buf;
}

// ------------------------------------------------------------------
// Структури карти-кіл
#pragma pack(push,1)
struct Circle { float x,y,r; };
#pragma pack(pop)
std::vector<Circle> circles;


void saveMap(const std::string& binFile,
             const std::string& txtFile)
{
    // 1) Бінарний файл — залишимо, раптом потрібен
    {
        std::ofstream bf(binFile, std::ios::binary | std::ios::trunc);
        uint32_t cnt = static_cast<uint32_t>(circles.size());
        bf.write(reinterpret_cast<char*>(&cnt), 4);
        bf.write(reinterpret_cast<char*>(circles.data()),
                 cnt * sizeof(Circle));
    }

    // 2) Людське TXT/CSV
    std::ofstream tf(txtFile, std::ios::trunc);
    tf << "# x,y,r (метри)\n";
    for (const auto& c : circles)
        tf << c.x << ',' << c.y << ',' << c.r << '\n';
    tf.close();

    std::cout << "[MAP] saved "
              << circles.size() << " circles → "
              << txtFile << '\n';
}

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

// ------------------------------------------------------------------
// Потік: карта / патчі (TCP 13000)
void mapThread()
{
    int srv = listenOn(MAP_PORT);
    for(;;)
    {
        int cli = accept(srv,nullptr,nullptr);
        uint8_t type;
        if(recv(cli,&type,1,MSG_WAITALL)<=0){ CLOSESOCK(cli); continue; }

        if(type==10) // FULL_MAP
        {
            uint32_t count;
            recv(cli,&count,4,MSG_WAITALL);
            circles.resize(count);
            recv(cli,circles.data(),count*sizeof(Circle),MSG_WAITALL);
            std::cout<<"[MAP] received "<<count<<" circles\n";
            saveMap("circle_map.bin", "circle_map.txt");


        }
        else if(type==12) // PATCH (1 коло)
        {
            Circle c; recv(cli,&c,sizeof(c),MSG_WAITALL);
            circles.push_back(c);
            std::cout<<"[MAP] PATCH circle "<<c.x<<","<<c.y<<" r="<<c.r<<"\n";
            saveMap("circle_map.bin", "circle_map.txt");


        }
        CLOSESOCK(cli);
    }
}

// ------------------------------------------------------------------
// ── MAIN ───────────────────────────────────────────────────────────
int main()
{
#ifdef _WIN32
    WSADATA d; WSAStartup(MAKEWORD(2,2),&d);
#endif
    fs::create_directories(SAVE_DIR);

    // 1) TCP-потоки
//    std::thread tPose(poseThread);
    std::thread tMap (mapThread);

    // 2) HTTP-сервер кадрів
    httplib::Server http;
    http.Post("/", [](const httplib::Request& req, httplib::Response& res){
        if(req.files.empty()){ res.status=400; res.set_content("no file","text/plain"); return; }

        const auto& f = req.files.begin()->second;
        std::string path = SAVE_DIR + "/" + nextFilename();
        std::ofstream ofs(path,std::ios::binary);
        ofs.write(f.content.data(), f.content.size());
        ofs.close();

        std::cout<<"[HTTP] saved "<<path<<" ("<<f.content.size()<<" bytes)\n";
        res.set_content(R"({"x":0,"y":0,"w":0,"h":0})","application/json");
    });

    std::cout<<"[HTTP] listening "<<HTTP_PORT<<"\n";
    http.listen("0.0.0.0", HTTP_PORT);

//    tPose.join();
    tMap.join();
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
