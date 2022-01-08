#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <chrono>
#include <ConsoleApi2.h>
#include <ConsoleApi3.h>
#include <conio.h>
#include <stdio.h>
#include <vector>
#pragma comment (lib, "Kernel32.lib")

std::chrono::steady_clock::time_point gametimer;

void startgame();
void gameloop();
void render();
void resetgame();
void initaliseobjects();

static HANDLE outputhandle = GetStdHandle(STD_OUTPUT_HANDLE);
static HWND consolehandle = GetConsoleWindow();

static COORD cursorreset = { (SHORT)0, (SHORT)0 };

static int previousscore = 0;
static int score = 0;

struct objects {
    int x;
    int y;
};
std::vector<objects*> squares;
std::vector<objects*> foods;
int scene[16][33];
int scenedoublebuffer[16][33];

bool doublebufferflip = 0;
static int lastmovekey = 0;



int main()
{
    std::ios_base::sync_with_stdio(false);

    //center window and resize
    RECT screensize;
    GetWindowRect(GetDesktopWindow(), &screensize);

    int consolesizeX = 300;
    int consolesizeY = 300;

    _COORD coord;
    coord.X = consolesizeX + 1;
    coord.Y = consolesizeY + 1;

  
    SetConsoleScreenBufferSize(outputhandle, coord);
    SetWindowPos(consolehandle, 0, 0, 0, consolesizeX, consolesizeY, SWP_SHOWWINDOW | SWP_NOMOVE);
    MoveWindow(consolehandle, (screensize.right / 2) - consolesizeX, screensize.bottom / 2 - consolesizeY, consolesizeX, consolesizeY, TRUE);
    
    //stop resize
    SetWindowLong(consolehandle, GWL_STYLE, GetWindowLong(consolehandle, GWL_STYLE) & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX);

    //change console font and size
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 00;                 
    cfi.dwFontSize.Y = 30;                  
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Arial");
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);

    //hide cursor
    CONSOLE_CURSOR_INFO cursorinfo;
    cursorinfo.bVisible = 0;
    cursorinfo.dwSize = 1;
    SetConsoleCursorInfo(outputhandle, &cursorinfo);

    //exit program if screen not set right size
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    size_t width = csbi.srWindow.Right - csbi.srWindow.Left;
    size_t height = csbi.srWindow.Bottom - csbi.srWindow.Top;
    if (csbi.srWindow.Right - csbi.srWindow.Left != 32)
        return 0;
    if (csbi.srWindow.Bottom - csbi.srWindow.Top != 15)
        return 0;

    startgame();

}

void startgame() {

    resetgame();
    SetConsoleCursorPosition(outputhandle, cursorreset);
    std::wcout << "Press wasd keys to start the game\n";
    std::wcout << "\nPrevious score: " << previousscore;
    while (true)
    {
        for (int i = 8; i <= 256; i++)
        {
            if (GetAsyncKeyState(i) & 0x7FFF)
            {
                resetgame();

                initaliseobjects();
                gameloop();
            }
        }
    }
}

void gameloop() {
    while (1)
    {
        gametimer = std::chrono::high_resolution_clock::now();

        //reset the scene array
        static COORD cursorcoord = { (SHORT)0, (SHORT)0 };
        for (size_t ii = 0; ii < 16; ii++)
        {
            for (size_t i = 0; i < 33; i++)
            {
                scene[ii][i] = (int)0;
            }
        }

        //game input - w = 1 = up, a = 2 = left, s = 3 = down, d = 4 = right
        int lastmove = 0;

        if (squares.size() > 1) {
            if (GetAsyncKeyState(87) && lastmovekey != 3) lastmovekey = 1;
            else if (GetAsyncKeyState(65) && lastmovekey != 4) lastmovekey = 2;
            else if (GetAsyncKeyState(83) && lastmovekey != 1) lastmovekey = 3;
            else if (GetAsyncKeyState(68) && lastmovekey != 2) lastmovekey = 4;
        }
        else {
            if (GetAsyncKeyState(87)) lastmovekey = 1;
            else if (GetAsyncKeyState(65)) lastmovekey = 2;
            else if (GetAsyncKeyState(83)) lastmovekey = 3;
            else if (GetAsyncKeyState(68)) lastmovekey = 4;
        }

        //alow lastmove to change in the update snake loop but always set it back to lastmovekey when loop reran
        lastmove = lastmovekey;

        //update snakes body
        int count = 0;
        for (auto* i : squares) { count++; }
        int i;
        for (i = 0; i < count; i++)
        {
            switch (lastmove)
            {
            case 1://up
                if (squares[i] == squares[0])
                {

                    squares[0]->y -= 1;
                    break;
                }
                if (squares[i]->x == squares[i - 1]->x)
                {
                    squares[i]->y -= 1;
                    lastmove = 1;
                    break;
                }
                if (squares[i]->x > squares[i - 1]->x)
                {
                    squares[i]->x -= 1;
                    lastmove = 2;
                    break;
                }
                if (squares[i]->x < squares[i - 1]->x)
                {
                    squares[i]->x += 1;
                    lastmove = 4;
                    break;
                }

                break;
            case 2://left ->
                if (squares[i] == squares[0])
                {

                    squares[0]->x -= 1;
                    break;
                }
                if (squares[i]->y == squares[i - 1]->y)
                {
                    squares[i]->x -= 1;
                    lastmove = 2;
                    break;
                }
                if (squares[i]->y > squares[i - 1]->y)
                {
                    squares[i]->y -= 1;
                    lastmove = 1;
                    break;
                }
                if (squares[i]->y < squares[i - 1]->y)
                {
                    squares[i]->y += 1;
                    lastmove = 3;
                    break;
                }

                break;
            case 3://down
                if (squares[i] == squares[0])
                {

                    squares[0]->y += 1;
                    break;
                }
                if (squares[i]->x == squares[i - 1]->x)
                {
                    squares[i]->y += 1;
                    lastmove = 3;
                    break;
                }
                if (squares[i]->x > squares[i - 1]->x)
                {
                    squares[i]->x -= 1;
                    lastmove = 2;
                    break;
                }
                if (squares[i]->x < squares[i - 1]->x)
                {
                    squares[i]->x += 1;
                    lastmove = 4;
                    break;
                }

                break;
            case 4://right
                if (squares[i] == squares[0])
                {

                    squares[0]->x += 1;
                    break;
                }
                if (squares[i]->y == squares[i - 1]->y)
                {
                    squares[i]->x += 1;
                    lastmove = 4;
                    break;
                }
                if (squares[i]->y > squares[i - 1]->y)
                {
                    squares[i]->y -= 1;
                    lastmove = 1;
                    break;
                }
                if (squares[i]->y < squares[i - 1]->y)
                {
                    squares[i]->y += 1;
                    lastmove = 3;
                    break;
                }

                break;

            }
        }

        //if eat food
        if (squares[0]->x == foods[0]->x && squares[0]->y == foods[0]->y)
        {
            score++;//game input - w = 1 = up, a = 2 = left, s = 3 = down, d = 4 = right
            //determine where to initaly place new snake body
            objects* firstsquare = new objects;
            switch (lastmove)
            {
            case 1:
                firstsquare->x = squares[i - 1]->x;
                firstsquare->y = squares[i - 1]->y + 1;
                break;

            case 2:
                firstsquare->x = squares[i - 1]->x + 1;
                firstsquare->y = squares[i - 1]->y;
                break;

            case 3:
                firstsquare->x = squares[i - 1]->x;
                firstsquare->y = squares[i - 1]->y - 1;
                break;

            case 4:
                firstsquare->x = squares[i - 1]->x - 1;
                firstsquare->y = squares[i - 1]->y;
                break;
            }
            squares.push_back(firstsquare);

            //generate new food
            bool freespace = true;
            do {
                freespace = true;
                foods[0]->x = (rand() % 32);
                foods[0]->y = (rand() % 15);
                for (auto* i : squares)
                {
                    if (foods[0]->x == i->x && foods[0]->y == i->y)
                        freespace = false;
                }
            } while (!freespace);


        }



        //snake hits wall
        if (squares[0]->x > 32 || squares[0]->y <= -1 || squares[0]->x <= -1 || squares[0]->y > 15)
        {
            startgame();
        }


        
        for (auto* i : squares)
        {
            //snake hits body
            if (squares[0] != i && squares[0]->y == i->y && squares[0]->x == i->x)
                startgame();

            //add objects to scene array
            //set snake
            if (i == squares[0])
                scene[i->y][i->x] = (int)2;
            else
                scene[i->y][i->x] = (int)1;
        }
        for (auto* i : foods)
        {
            //set food
            scene[i->y][i->x] = (int)2;
        }

        render();

#if (DEBUG)
        int difference = ((int)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - gametimer).count());
        char a[20];
        sprintf(a, "%d", difference);
        std::string ae = (std::string)a + "\n";
        LPTSTR lstring = new TCHAR[ae.size() + 1];
        strcpy(lstring, ae.c_str());
        OutputDebugString(lstring);
#endif
        while ((int)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - gametimer).count() <= 166000000) {}
        
    }
}

void render() 
{

    static COORD cursorcoord = { (SHORT)0, (SHORT)0 };
    

    for (size_t ii = 0; ii < 16; ii++)
    {
        for (size_t i = 0; i < 33; i++)
        {
            //check if something changed compared to previous scene
            if (scenedoublebuffer[ii][i] != scene[ii][i])
            {
                std::cout.flush();
                cursorcoord = { (SHORT)i, (SHORT)ii };
                SetConsoleCursorPosition(outputhandle, cursorcoord);

                switch (scene[ii][i])
                {
                    case 0:
                        //std::cout << " ";
                        printf(" ");
                        //write changes from current scene to previous scene
                        scenedoublebuffer[ii][i] = 0;
                        break;

                    case 1:
                        //std::cout << (char)219u;
                        printf("%c", (char)219u);
                        scenedoublebuffer[ii][i] = 1;
                        break;

                    case 2:
                        //std::cout << (char)176u;
                        printf("%c", (char)176u);
                        scenedoublebuffer[ii][i] = 2;
                        break;
                }
                SetConsoleCursorPosition(outputhandle, cursorreset);
            }
        }
    }
}

void resetgame() {
    //delete all objects in the 2 arrays and clear the arrays
    previousscore = score;
    static COORD cursorcoord = { (SHORT)0, (SHORT)0 };
    //fill screen and arrays with walls
    for (size_t ii = 0; ii < 16; ii++)
    {
        for (size_t i = 0; i < 33; i++)
        {
            std::cout.flush();
            cursorcoord = { (SHORT)i, (SHORT)ii };
            SetConsoleCursorPosition(outputhandle, cursorcoord);
            std::cout << " ";

            scene[ii][i] = (int)0;
            scenedoublebuffer[ii][i] = (int)0;
        }
    }

    for (auto* i : squares)
    {
        delete i;
    }
    for (auto* i : foods)
    {
        delete i;
    }

    squares.clear();
    foods.clear();
    
    score = 0;

    return;
}


void initaliseobjects()
{
    objects* firstsquare = new objects;
    firstsquare->x = 16;
    firstsquare->y = 8;
    squares.push_back(firstsquare);

    objects* firstfood = new objects;
    firstfood->x = (rand() % 32);
    firstfood->y = (rand() % 15);
    foods.push_back(firstfood);
}