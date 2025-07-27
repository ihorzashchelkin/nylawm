#pragma once

#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include <cstdint>

namespace cirnowm {

class glx_window_manager
{
  x11_manager& m_X;
  xcb_window_t m_Window;
  int m_VisualId;
  uint32_t m_GLXWindow;
  GLXContext m_GLXContext;

public:
  glx_window_manager(x11_manager& X);
  ~glx_window_manager();

private:
};

struct desktop
{
  uint32_t Pixmap;
  xcb_window_t ActiveWin;
};

struct client
{
  uint8_t Flags;
  static inline decltype(Flags) FlagMapped = 1 << 0;

  uint32_t Pixmap;
  uint8_t DesktopIndex;
  uint16_t X;
  uint16_t CurrentX;
  uint16_t Y;
  uint16_t CurrentY;
  uint16_t Width;
  uint16_t CurrentWidth;
  uint16_t Height;
  uint16_t CurrentHeight;
};

struct config
{
  bool DebugLog;
  std::span<const keybind> Bindings;
};

class controller
{
  uint8_t Flags;
  static inline decltype(Flags) FlagRunning = 1 << 0;

  uint8_t CurDesktopIndex;
  uint16_t MouseX;
  uint16_t MouseY;
  xcb_window_t CompositorWindow;
  xcb_window_t OverlayWin;
  uint32_t GraphicsContext;
  std::array<desktop, 16> Desktops;
  std::unordered_map<xcb_window_t, client> Clients;
  xcb_ewmh_connection_t Conn;
  xcb_screen_t* Screen;
  xcb_errors_context_t* ErrorContext;
  const config& Config;
  std::vector<keybind::resolved> ResolvedKeybinds;

public:
  explicit controller(const config& Config)
    : Flags{}
    , CurDesktopIndex{}
    , MouseX{ std::numeric_limits<decltype(MouseX)>::max() }
    , MouseY{ std::numeric_limits<decltype(MouseY)>::max() }
    , GraphicsContext{}
    , Desktops{}
    , Clients{}
    , Conn{}
    , Screen{}
    , ErrorContext{}
    , Config{ Config }
    , ResolvedKeybinds{}
  {
  }
  ~controller();

  bool TryInit();
  void Run();

  void Quit() { Flags &= ~(FlagRunning); }
  void Spawn(const char* const command[]);
  void Kill();
  void SwitchToNextWorkspace();
  void SwitchToPrevWorkspace();

private:
  xcb_connection_t* GetConn() { return Conn.connection; }
  void SetConn(xcb_connection_t* Conn) { this->Conn.connection = Conn; }
  xcb_window_t GetRoot() { return Screen->root; }
  desktop& GetCurDesktop() { return Desktops[CurDesktopIndex]; }

  client* GetClient(xcb_window_t Window);
  client& Manage(xcb_window_t Window);
  void Prepare();
  void PrepareSpawn();
  void TryResolveKeyBinds();
  void GrabKeys();

};

} // namespace cirnowm
