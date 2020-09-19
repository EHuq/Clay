#pragma once
#pragma comment(lib, "winmm.lib")

#ifndef UNICODE
#error Turn UNICODE on for this system
#endif

struct object {
    vector<tri> tris;
};
enum COLOUR
{
  FG_BLACK = 0x0000,
  FG_DARK_BLUE = 0x0001,
  FG_DARK_GREEN = 0x0002,
  FG_DARK_CYAN = 0x0003,
  FG_DARK_RED = 0x0004,
  FG_DARK_MAGENTA = 0x0005,
  FG_DARK_YELLOW = 0x0006,
  FG_GREY = 0x0007, // Thanks MS :-/
  FG_DARK_GREY = 0x0008,
  FG_BLUE = 0x0009,
  FG_GREEN = 0x000A,
  FG_CYAN = 0x000B,
  FG_RED = 0x000C,
  FG_MAGENTA = 0x000D,
  FG_YELLOW = 0x000E,
  FG_WHITE = 0x000F,
  BG_BLACK = 0x0000,
  BG_DARK_BLUE = 0x0010,
  BG_DARK_GREEN = 0x0020,
  BG_DARK_CYAN = 0x0030,
  BG_DARK_RED = 0x0040,
  BG_DARK_MAGENTA = 0x0050,
  BG_DARK_YELLOW = 0x0060,
  BG_GREY = 0x0070,
  BG_DARK_GREY = 0x0080,
  BG_BLUE = 0x0090,
  BG_GREEN = 0x00A0,
  BG_CYAN = 0x00B0,
  BG_RED = 0x00C0,
  BG_MAGENTA = 0x00D0,
  BG_YELLOW = 0x00E0,
  BG_WHITE = 0x00F0,
};

enum PIXEL_TYPE
{
  PIXEL_SOLID = 0x2588,
  PIXEL_THREEQUARTERS = 0x2593,
  PIXEL_HALF = 0x2592,
  PIXEL_QUARTER = 0x2591,
};



class clayGameEngine
{
public:
  clayGameEngine()
  {


    m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    m_hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);

    memset(m_keyNewState, 0, 256 * sizeof(short));
    memset(m_keyOldState, 0, 256 * sizeof(short));
    memset(m_keys, 0, 256 * sizeof(sKeyState));
    m_mousePosX = 0;
    m_mousePosY = 0;

    m_sAppName = L"Default";
  }
  
  int ConstructConsole(int width, int height, int fontw, int fonth)
  {
    if (m_hConsole == INVALID_HANDLE_VALUE)
      return Error(L"Bad Handle");

    c.setScreenWidth(width);
    c.setScreenHeight(height);

    // Change console visual size to a minimum so ScreenBuffer can shrink
    // below the actual visual size
    m_rectWindow = {0, 0, 1, 1};
    SetConsoleWindowInfo(m_hConsole, TRUE, &m_rectWindow);

    // Set the size of the screen buffer
    COORD coord = {(short)c.ScreenWidth(), (short)c.ScreenHeight()};
    if (!SetConsoleScreenBufferSize(m_hConsole, coord))
      Error(L"SetConsoleScreenBufferSize");

    // Assign screen buffer to the console
    if (!SetConsoleActiveScreenBuffer(m_hConsole))
      return Error(L"SetConsoleActiveScreenBuffer");

    // Set the font size now that the screen buffer has been assigned to the console
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = fontw;
    cfi.dwFontSize.Y = fonth;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas");
    if (!SetCurrentConsoleFontEx(m_hConsole, false, &cfi))
      return Error(L"SetCurrentConsoleFontEx");

    // Get screen buffer info and check the maximum allowed window size. Return
    // error if exceeded, so user knows their dimensions/fontsize are too large
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(m_hConsole, &csbi))
      return Error(L"GetConsoleScreenBufferInfo");
    if (c.ScreenHeight() > csbi.dwMaximumWindowSize.Y)
    {
      return Error(L"Screen Height / Font Height Too Big");
    }
    if (c.ScreenWidth() > csbi.dwMaximumWindowSize.X)
      return Error(L"Screen Width / Font Width Too Big");

    // Set Physical Console Window Size
    m_rectWindow = {0, 0, (short)c.ScreenWidth() - 1, (short)c.ScreenHeight() - 1};
    if (!SetConsoleWindowInfo(m_hConsole, TRUE, &m_rectWindow))
      return Error(L"SetConsoleWindowInfo");

    // Set flags to allow mouse input
    if (!SetConsoleMode(m_hConsoleIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT))
      return Error(L"SetConsoleMode");

    // Allocate memory for screen buffer
    m_bufScreen = new CHAR_INFO[c.ScreenWidth() * c.ScreenHeight()];
    memset(m_bufScreen, 0, sizeof(CHAR_INFO) * c.ScreenWidth() * c.ScreenHeight());

    //SetConsoleCtrlHandler((PHANDLER_ROUTINE)CloseHandler, TRUE);
    return 1;
  }

  void Draw(int x, int y, short ca = 0x2588, short col = 0x000F)
  {
      if (x >= 0 && x < c.ScreenWidth() && y >= 0 && y < c.ScreenHeight())
      {
          m_bufScreen[y * c.ScreenWidth() + x].Char.UnicodeChar = ca;
          m_bufScreen[y * c.ScreenWidth() + x].Attributes = col;
      }
  }

  void Fill(int x1, int y1, int x2, int y2, short c = 0x2588, short col = 0x000F)
  {
      Clip(x1, y1);
      Clip(x2, y2);
      for (int x = x1; x < x2; x++)
          for (int y = y1; y < y2; y++)
              Draw(x, y, c, col);
  }

  void Clip(int& x, int& y)
  {
      if (x < 0)
          x = 0;
      if (x >= c.ScreenWidth())
          x = c.ScreenWidth();
      if (y < 0)
          y = 0;
      if (y >= c.ScreenHeight())
          y = c.ScreenHeight();
  }

  void Start()
  {
    // Start the thread
    m_bAtomActive = true;
    thread t = thread(&clayGameEngine::GameThread, this);

    // Wait for thread to be exited
    t.join();
  }

  virtual bool OnUserCreate() = 0;

  virtual bool OnUserUpdate(float fElapsedTime) = 0;


  // 

private:
  void GameThread()
  {
      if (!OnUserCreate()) {
          m_bAtomActive = false;
      }

    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();

    while (m_bAtomActive)
    {
      // Handle Timing
      tp2 = chrono::system_clock::now();
      chrono::duration<float> elapsedTime = tp2 - tp1;
      tp1 = tp2;
      float fElapsedTime = elapsedTime.count();

      // Handle Keyboard Input
      for (int i = 0; i < 256; i++)
      {
        m_keyNewState[i] = GetAsyncKeyState(i);

        m_keys[i].bPressed = false;
        m_keys[i].bReleased = false;

        if (m_keyNewState[i] != m_keyOldState[i])
        {
          if (m_keyNewState[i] & 0x8000)
          {
            m_keys[i].bPressed = !m_keys[i].bHeld;
            m_keys[i].bHeld = true;
          }
          else
          {
            m_keys[i].bReleased = true;
            m_keys[i].bHeld = false;
          }
        }

        m_keyOldState[i] = m_keyNewState[i];
      }

      // Handle Mouse Input - Check for window events
      INPUT_RECORD inBuf[32];
      DWORD events = 0;
      GetNumberOfConsoleInputEvents(m_hConsoleIn, &events);
      if (events > 0)
        ReadConsoleInput(m_hConsoleIn, inBuf, events, &events);

      // Handle events - we only care about mouse clicks and movement
      // for now
      for (DWORD i = 0; i < events; i++)
      {
        switch (inBuf[i].EventType)
        {
        case FOCUS_EVENT:
        {
          m_bConsoleInFocus = inBuf[i].Event.FocusEvent.bSetFocus;
        }
        break;

        case MOUSE_EVENT:
        {
          switch (inBuf[i].Event.MouseEvent.dwEventFlags)
          {
          case MOUSE_MOVED:
          {
            m_mousePosX = inBuf[i].Event.MouseEvent.dwMousePosition.X;
            m_mousePosY = inBuf[i].Event.MouseEvent.dwMousePosition.Y;
          }
          break;

          case 0:
          {
            for (int m = 0; m < 5; m++)
              m_mouseNewState[m] = (inBuf[i].Event.MouseEvent.dwButtonState & (1 << m)) > 0;
          }
          break;

          default:
            break;
          }
        }
        break;

        default:
          break;
        }
      }

      for (int m = 0; m < 5; m++)
      {
        m_mouse[m].bPressed = false;
        m_mouse[m].bReleased = false;

        if (m_mouseNewState[m] != m_mouseOldState[m])
        {
          if (m_mouseNewState[m])
          {
            m_mouse[m].bPressed = true;
            m_mouse[m].bHeld = true;
          }
          else
          {
            m_mouse[m].bReleased = true;
            m_mouse[m].bHeld = false;
          }
        }

        m_mouseOldState[m] = m_mouseNewState[m];
      }

      // Handle Frame Update
      if (!OnUserUpdate(fElapsedTime))
        m_bAtomActive = false;

      // Update Title & Present Screen Buffer
      wchar_t s[256];
      swprintf_s(s, 256,  m_sAppName.c_str(), 1.0f / fElapsedTime);
      SetConsoleTitle(s);
      WriteConsoleOutput(m_hConsole, m_bufScreen, {(short)c.ScreenWidth(), (short)c.ScreenHeight() }, {0, 0}, &m_rectWindow);
    }
  }

protected:
  int Error(const wchar_t *msg)
  {
    wchar_t buf[256];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
    SetConsoleActiveScreenBuffer(m_hOriginalConsole);
    wprintf(L"ERROR: %s\n\t%s\n", msg, buf);
    return 0;
  }
  /*
			static BOOL CloseHandler(DWORD evt)
			{
				if (evt == CTRL_CLOSE_EVENT)
				{
					m_bAtomActive = false;

					// Wait for thread to be exited
					std::unique_lock<std::mutex> ul(m_muxGame);
					m_cvGameFinished.wait(ul);
				}
				return true;
			}
			*/

protected:
  
  struct sKeyState
  {
    bool bPressed;
    bool bReleased;
    bool bHeld;
  } m_keys[256], m_mouse[5];

  int m_mousePosX;
  int m_mousePosY;

public:
  sKeyState GetKey(int nKeyID) { return m_keys[nKeyID]; }
  int GetMouseX() { return m_mousePosX; }
  int GetMouseY() { return m_mousePosY; }
  sKeyState GetMouse(int nMouseButtonID) { return m_mouse[nMouseButtonID]; }
  bool IsFocused() { return m_bConsoleInFocus; }

protected:
  float fTheta;
  Constants c;
  CHAR_INFO *m_bufScreen;
  std::wstring m_sAppName;
  HANDLE m_hOriginalConsole;
  CONSOLE_SCREEN_BUFFER_INFO m_OriginalConsoleInfo;
  HANDLE m_hConsole;
  HANDLE m_hConsoleIn;
  SMALL_RECT m_rectWindow;
  short m_keyOldState[256] = {0};
  short m_keyNewState[256] = {0};
  bool m_mouseOldState[5] = {0};
  bool m_mouseNewState[5] = {0};
  bool m_bConsoleInFocus = true;
  bool m_bEnableSound = false;
  Vertex vCamera;

  std::atomic<bool> m_bAtomActive;
  std::condition_variable m_cvGameFinished;
  std::mutex m_muxGame;


  // ALL OBJECTS FXNS

  // ========================================================================
  // ========================================================================
  // ========================================================================
  // ========================================================================
  // ========================================================================
  // ========================================================================
  // ========================================================================
  // ========================================================================
  // ========================================================================
  // ========================================================================
  // ========================================================================
  // ========================================================================

  // FOR NOW INHERITANCE BEING FUNKY
    public:

            object LoadFromObjectFile(string filename)
            {
                object newObj;
                ifstream f(filename);

                if (!f.is_open())
                {
                    return newObj;
                }

                //temp storage of all vertices
                vector<Vertex> vertices;

                while (!f.eof())
                {
                    char line[128]; //assume 128 characters max, if it's longer than it doesn't matter to us since we only want the coords
                    f.getline(line, 128);

                    strstream str;
                    str << line;

                    char garbage; //the info we get that we dont need

                    if (line[0] == 'v')
                    {
                        Vertex v;
                        str >> garbage >> v.x >> v.y >> v.z;
                        vertices.push_back(v);
                    }

                    if (line[0] == 'f')
                    {
                        int faces[3];
                        str >> garbage;

                        for (int i = 0; i < 3; i++)
                        {
                            str >> faces[i];
                        }
                        tri t(vertices[faces[0] - 1], vertices[faces[1] - 1], vertices[faces[2] - 1]);
                        newObj.tris.push_back(t);
                    }
                }

                return newObj;
            }

            void RotateZ(float fTheta, object o) {
                vector<tri> toBeRasterized;
                //  vector<unique_ptr<tri>> tbrForSort;
                for (auto& triangle : o.tris) {
                    triangle.RotateZ(fTheta);
                    triangle.ThreeDtoTwoD(vCamera);
                    toBeRasterized.push_back(triangle);
                    //  tbrForSort.push_back(make_unique<tri>(triangle));
                }
                Rasterize(toBeRasterized);
            }

            /*
            static bool sortByMid(const unique_ptr<tri>& a, const unique_ptr<tri>& b) {
                float z1Midpoint = 0.0;
                float z2Midpoint = 0.0;
                for (int i = 0; i < 3; i++)
                {
                    z1Midpoint += a->vertices[i].z;
                    z2Midpoint += b->vertices[i].z;
                }
                z1Midpoint /= 3.0f;
                z2Midpoint /= 3.0f;

                return z1Midpoint > z2Midpoint;
            }
            */

            void Rasterize(vector<tri> toBeRasterized) {
                // sort(tbrForSort.begin(), tbrForSort.end(), sortByMid);

                for (auto& triProjected : toBeRasterized)
                {

                    //just the 3 lines after one another
                    //FillTriangle(triProjected.vertices[0],
                    //             triProjected.vertices[1],
                    //             triProjected.vertices[2],
                    //             triProjected.sym,
                    //             triProjected.col);

                    DrawTriangle(triProjected.vertices[0],
                        triProjected.vertices[1],
                        triProjected.vertices[2],
                        PIXEL_SOLID,
                        FG_DARK_RED);
                }
            }

            void DrawLine(int x0, int y0, int x1, int y1, short c = 0x2588, short col = 0x000F) {
                if (y1 - y0 > 0 && y1 - y0 < x1 - x0) {
                    Bresenham(x0, y0, x1, y1, c, col);
                }
                else {
                    DDA(x0, y0, x1, y1, c, col);
                }
            }

            void Bresenham(int x0, int y0, int x1, int y1, short c = 0x2588, short col = 0x000F) {
                int dx, dy, p, x, y;

                dx = x1 - x0;
                dy = y1 - y0;

                x = x0;
                y = y0;

                p = 2 * dy - dx;

                while (x < x1)
                {
                    if (p >= 0)
                    {
                        Draw(x, y, c, col);
                        y = y + 1;
                        p = p + 2 * dy - 2 * dx;
                    }
                    else
                    {
                        Draw(x, y, c, col);
                        p = p + 2 * dy;
                    }
                    x = x + 1;
                }
            }

            void DDA(int X0, int Y0, int X1, int Y1, short c = 0x2588, short col = 0x000F)
            {
                // calculate dx & dy 
                int dx = X1 - X0;
                int dy = Y1 - Y0;

                // calculate steps required for generating pixels 
                int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

                // calculate increment in x & y for each steps 
                float Xinc = dx / (float)steps;
                float Yinc = dy / (float)steps;

                // Put pixel for each step 
                float X = X0;
                float Y = Y0;
                for (int i = 0; i <= steps; i++)
                {
                    Draw(X, Y, c, col);  // put pixel at (X,Y) 
                    X += Xinc;           // increment in x at each step 
                    Y += Yinc;           // increment in y at each step 
               // for visualization of line- 
                                         // generation step by step 
                }
            }

            void DrawTriangle(Vertex v1, Vertex v2, Vertex v3, short c = 0x2588, short col = 0x000F)
            {
                int x1 = v1.x;
                int y1 = v1.y;
                int x2 = v2.x;
                int y2 = v2.y;
                int x3 = v3.x;
                int y3 = v3.y;
                DrawLine(x1, y1, x2, y2, c, col);
                DrawLine(x2, y2, x3, y3, c, col);
                DrawLine(x3, y3, x1, y1, c, col);
            }

            void fillBottomTriangle(Vertex v1, Vertex v2, Vertex v3) { //start at top, go ccw
              //Need inverse slope to determine the change in x for every step in y
                float slopeL = (v1.x - v2.x) / (v1.y - v2.y);
                float slopeR = (v1.x - v3.x) / (v1.y - v3.y);
                float xCurL = v2.x;
                float xCurR = v3.x;

                for (int scanY = v2.y; scanY <= v1.y; scanY++) {

                    DrawLine((int)xCurL, scanY, (int)xCurR, scanY);
                    xCurL += slopeL;
                    xCurR += slopeR;
                }
            }
};
