/*  GridWaypointClient.cpp   SDL2 + SDL_ttf + TCP  */
/*  компіляція:  
    g++ GridWaypointClient.cpp -std=c++17 \
       $(pkg-config --cflags sdl2 SDL2_ttf) \
       $(pkg-config --libs   sdl2 SDL2_ttf) \
       -pthread -o grid_client                                       */

#include <SDL.h>
#include <SDL_ttf.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <cstdio>         // <─ містить snprintf
#include <cstring>
#include <thread>

const int POSE_PORT = 20002;
int poseSock;  int dGX=0,dGZ=0;   // грід-коорд дрона
int  scale   = 2;   // м/клітинку

/* ─── мережа ──────────────────────────────────────────── */
const char* HOST = "127.0.0.1";
const int   PORT = 11000;

void poseListener()
{
    poseSock = socket(AF_INET,SOCK_DGRAM,0);
    sockaddr_in adr{.sin_family=AF_INET,.sin_port=htons(POSE_PORT),
                    .sin_addr{INADDR_ANY}};
    bind(poseSock,(sockaddr*)&adr,sizeof(adr));

    uint8_t buf[12]; socklen_t alen=sizeof(sockaddr_in);
    while(true){
        if(recvfrom(poseSock,buf,12,0,nullptr,&alen)==12){
            float ax,ay,az; memcpy(&ax,buf,4); memcpy(&az,buf+8,4);
            dGX = lround(ax/scale);  dGZ = lround(az/scale);
        }
    }
}

void sendMsg(const std::string& s)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in adr{.sin_family = AF_INET, .sin_port = htons(PORT)};
    inet_pton(AF_INET, HOST, &adr.sin_addr);
    if (connect(sock, (sockaddr*)&adr, sizeof(adr)) == 0)
        send(sock, s.c_str(), s.size(), 0);
    close(sock);
    printf("sent: %s\n", s.c_str());
}

/* ─── грід ────────────────────────────────────────────── */
const int GRID = 20, PIX = 32;

int  heightM = 10;  // y-рівень

struct Way { int gx, gz; };
std::vector<Way> wp;

inline float WX(int gx) { return gx * scale; }
inline float WZ(int gz) { return gz * scale; }

inline int toPixX(int gx){ return (gx + GRID/2) * PIX; }
inline int toPixZ(int gz){ return (gz + GRID/2) * PIX; }

/* ─── текстова текстура ──────────────────────────────── */
SDL_Texture* textTex(SDL_Renderer* r, TTF_Font* f, const std::string& s)
{
    SDL_Surface* surf = TTF_RenderText_Blended(f, s.c_str(), {255, 255, 255});
    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
    SDL_FreeSurface(surf);
    return tex;
}

/* ─── main ────────────────────────────────────────────── */
int main()
{
    std::thread tPose(poseListener); tPose.detach();
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() < 0) {
        printf("SDL/TTF init error\n"); return 1;
    }

    SDL_Window*  win = SDL_CreateWindow("Grid Client",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        GRID * PIX, GRID * PIX, 0);
    SDL_Renderer* rnd = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font* font = TTF_OpenFont("/System/Library/Fonts/SFNS.ttf", 12);

    auto setTitle = [&]() {
        char buf[128];
        std::snprintf(buf, sizeof(buf),
            "Grid (scale=%d m, height=%d m)  [LMB add, C confirm, ± scale, ↑↓ height, R reset]",
            scale, heightM);
        SDL_SetWindowTitle(win, buf);
    };
    setTitle();

    bool quit = false; SDL_Event e;
    while (!quit)
    {

        while (SDL_PollEvent(&e))

        {
            
            if (e.type == SDL_QUIT) quit = true;

            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int gx = e.button.x / PIX - GRID/2;      // ★ NEW центр-нуль
                int gz = e.button.y / PIX - GRID/2;
                if(gx < -GRID/2 || gx >= GRID/2 ||
                gz < -GRID/2 || gz >= GRID/2) continue;
                
                if (e.button.button == SDL_BUTTON_LEFT) wp.push_back({ gx, gz });

                if (e.button.button == SDL_BUTTON_RIGHT) {
                    char buf[64];
                    std::snprintf(buf, sizeof(buf),
                        "prio:%g,%d,%g", WX(gx), heightM, WZ(gz));
                    sendMsg(buf);
                }
            }

            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                case SDLK_c:
                    if (!wp.empty()) {
                        std::string out = "LIST:";
                        char buf[64];
                        for (size_t i = 0; i < wp.size(); ++i) {
                            std::snprintf(buf, sizeof(buf),
                                "%g,%d,%g", WX(wp[i].gx), heightM, WZ(wp[i].gz));
                            out += buf;
                            if (i + 1 < wp.size()) out += ';';
                        }
                        sendMsg(out); wp.clear();
                    } break;
                case SDLK_h: sendMsg("home"); break;
                case SDLK_r: wp.clear(); break;
                case SDLK_PLUS: case SDLK_EQUALS: ++scale; setTitle(); break;
                case SDLK_MINUS: if (scale > 1) { --scale; setTitle(); } break;
                case SDLK_UP:   ++heightM; setTitle(); break;
                case SDLK_DOWN: if (heightM > 0) { --heightM; setTitle(); } break;
                }
            }
        }

        /* ── рендерінг ─ */
        SDL_SetRenderDrawColor(rnd, 25, 25, 25, 255); SDL_RenderClear(rnd);

        SDL_SetRenderDrawColor(rnd, 50, 50, 50, 255);
        for(int i=-GRID/2;i<=GRID/2;i++)
        {
            int x = toPixX(i);
            int z = toPixZ(i);
            SDL_RenderDrawLine(rnd,x,0,x,GRID*PIX);
            SDL_RenderDrawLine(rnd,0,z,GRID*PIX,z);
        }
        SDL_SetRenderDrawColor(rnd,200,30,30,255);
        SDL_Rect orect{ toPixX(0), toPixZ(0), PIX, PIX };
        SDL_RenderDrawRect(rnd,&orect);
        SDL_SetRenderDrawColor(rnd,255,255,0,255);
        SDL_Rect drect{ toPixX(dGX)+10, toPixZ(dGZ)+10, PIX-20, PIX-20 };
        SDL_RenderFillRect(rnd,&drect);

        SDL_SetRenderDrawColor(rnd, 80, 120, 255, 255);
        for(auto& w:wp)
        {
            SDL_Rect r{ toPixX(w.gx)+8, toPixZ(w.gz)+8, PIX-16, PIX-16 };
            SDL_RenderFillRect(rnd,&r);
            if(font){
                char txt[16]; snprintf(txt,sizeof(txt),"%d,%d",w.gx,w.gz);
                SDL_Texture* t=textTex(rnd,font,txt);
                int tw,th; SDL_QueryTexture(t,nullptr,nullptr,&tw,&th);
                SDL_Rect dst{ toPixX(w.gx)+2, toPixZ(w.gz)+2, tw, th };
                SDL_RenderCopy(rnd,t,nullptr,&dst);
                SDL_DestroyTexture(t);
            }
        }

        SDL_RenderPresent(rnd);
        SDL_Delay(16);
    }

    if (font) TTF_CloseFont(font);
    SDL_DestroyRenderer(rnd); SDL_DestroyWindow(win);
    TTF_Quit(); SDL_Quit();
    return 0;
}
