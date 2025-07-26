#include <cstdint>
#include <iostream>
#include <print>

#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>

#include <GL/glx.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xproto.h>

#include "wm.hpp"

x11_manager::x11_manager()
  : m_Display(XOpenDisplay(nullptr))
{
  if (!m_Display)
    std::println(std::cerr, "could not open display");

#if 0
    if (XInitThreads() == 0)
    {
        std::println(std::cerr, "x11_manager: XInitThreads failed");
        XCloseDisplay(m_Display);
        m_Display = nullptr;
    }
#endif

  m_XCBConn = XGetXCBConnection(m_Display);
  if (!m_XCBConn) {
    std::println(std::cerr, "could not get XCB connection from display");
    XCloseDisplay(m_Display);
    m_Display = nullptr;
  }

  XSetEventQueueOwner(m_Display, XCBOwnsEventQueue);

  m_ScreenNumber = DefaultScreen(m_Display);
  m_Screen = xcb_aux_get_screen(m_XCBConn, m_ScreenNumber);
}

uint32_t
x11_manager::GenerateId()
{
  return xcb_generate_id(m_XCBConn);
}

x11_manager::~x11_manager()
{
  if (m_Display)
    XCloseDisplay(m_Display);
}

glx_window_manager::glx_window_manager(x11_manager& Conn)
  : m_X{ Conn }
  , m_Window{ m_X.GenerateId() }
{

  // clang-format off
  static constexpr int VISUAL_ATTRIBS[] =
  {
    GLX_X_RENDERABLE,           True,
    GLX_DRAWABLE_TYPE,          GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,            GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE,          GLX_TRUE_COLOR,
    GLX_RED_SIZE,               8,
    GLX_GREEN_SIZE,             8,
    GLX_BLUE_SIZE,              8,
    GLX_ALPHA_SIZE,             8,
    GLX_DEPTH_SIZE,             24,
    GLX_STENCIL_SIZE,           8,
    GLX_DOUBLEBUFFER,           True,
    // GLX_SAMPLE_BUFFERS,      1,
    // GLX_SAMPLES,             4,
    None
  };
  // clang-format on

  int NumFBConfigs;
  GLXFBConfig* FBConfigs = glXChooseFBConfig(
    m_X.GetDisplay(), m_X.GetScreenNumber(), VisualAttribs, &NumFBConfigs);
  if (!FBConfigs || !NumFBConfigs) {
    std::println("glXGetFBConfigs failed");
    return;
  }

  GLXFBConfig FBConfig =
    *std::max_element(FBConfigs, FBConfigs + NumFBConfigs, [&](auto a, auto b) {
      int sa, sb;
      glXGetFBConfigAttrib(m_X.GetDisplay(), a, GLX_SAMPLES, &sa);
      glXGetFBConfigAttrib(m_X.GetDisplay(), b, GLX_SAMPLES, &sb);
      return sa < sb;
    });

  glXGetFBConfigAttrib(m_X.GetDisplay(), FBConfig, GLX_VISUAL_ID, &m_VisualId);

  m_GLXContext =
    glXCreateNewContext(m_X.GetDisplay(), FBConfig, GLX_RGBA_TYPE, 0, True);
  if (!m_GLXContext) {
    std::println("glXCreateNewContext failed");
    return;
  }

  xcb_colormap_t ColorMap = m_X.GenerateId();
  xcb_create_colormap(m_X.GetConnection(),
                      XCB_COLORMAP_ALLOC_NONE,
                      ColorMap,
                      m_X.GetRoot(),
                      m_VisualId);

  xcb_create_window(
    m_X.GetConnection(),
    XCB_COPY_FROM_PARENT,
    m_Window,
    m_X.GetRoot(),
    0,
    0,
    150,
    150,
    0,
    XCB_WINDOW_CLASS_INPUT_OUTPUT,
    m_VisualId,
    XCB_CW_EVENT_MASK | XCB_CW_COLORMAP,
    (uint32_t[]){
      XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS, ColorMap, 0 });

  xcb_map_window(m_X.GetConnection(), m_Window);

  m_GLXWindow = glXCreateWindow(m_X.GetDisplay(), FBConfig, m_Window, 0);
  glXMakeContextCurrent(
    m_X.GetDisplay(), m_GLXWindow, m_GLXWindow, m_GLXContext);
}

glx_window_manager::~glx_window_manager()
{
  if (m_X && m_GLXContext)
    glXDestroyContext(m_X.GetDisplay(), m_GLXContext);
}
