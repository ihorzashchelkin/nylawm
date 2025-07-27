#include <csignal>
#include <iostream>
#include <print>
#include <span>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <epoxy/gl.h>
#include <epoxy/glx.h>

#include <X11/Xlib-xcb.h>
#include <X11/Xlib.h>
#include <xcb/composite.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xproto.h>

#include "internal.hpp"

namespace cirnowm {

void GLAPIENTRY
GLDebugMessageCallback(GLenum aSource,
                       GLenum aType,
                       GLuint aId,
                       GLenum aSeverity,
                       GLsizei aLength,
                       const GLchar* aMessage,
                       const void* aUserParam)
{
  std::println(std::cerr,
               "GL CALLBACK: {} type = {:#x}, severity = {:#x}, message = {}",
               (aType == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
               aType,
               aSeverity,
               aMessage);
}

WindowManager::WindowManager(std::span<const Keybind> aKeybinds)
  : mFlags{}
  , mDisplay{}
  , mEwmh{}
  , mCompositorWindow{}
{
  mDisplay = XOpenDisplay(nullptr);
  if (!mDisplay) {
    std::println(std::cerr, "could not open display");
    return;
  }

  mEwmh = { .connection = XGetXCBConnection(mDisplay) };
  XSetEventQueueOwner(mDisplay, XCBOwnsEventQueue);

  mCompositorWindow = xcb_generate_id(mEwmh.connection);

  if (!xcb_ewmh_init_atoms_replies(
        &mEwmh, xcb_ewmh_init_atoms(mEwmh.connection, &mEwmh), nullptr)) {
    std::println(std::cerr, "could not intern ewmh atoms");
    return;
  }

  mScreenNumber = DefaultScreen(mDisplay);
  mScreen = xcb_aux_get_screen(mEwmh.connection, mScreenNumber);
  if (!mScreen) {
    std::println(std::cerr, "could not get screen {}", mScreenNumber);
    return;
  }

  if (xcb_errors_context_new(mEwmh.connection, &mErrorContext)) {
    std::println(std::cerr, "could not create error context");
    return;
  }

  static const uint32_t kRootEventMask =
    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
    XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_EXPOSURE |
    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
    XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_KEY_PRESS |
    XCB_EVENT_MASK_KEY_RELEASE;
  if (xcb_request_check(
        mEwmh.connection,
        xcb_change_window_attributes_checked(mEwmh.connection,
                                             mScreen->root,
                                             XCB_CW_EVENT_MASK,
                                             &kRootEventMask))) {

    std::println("could not change root window attributes. is another wm "
                 "already running?");
    return;
  }

  // clang-format off
  static const int kVisualAttribs[] =
  {
    GLX_X_RENDERABLE,         True,
    GLX_DRAWABLE_TYPE,        GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,          GLX_RGBA_BIT,
    GLX_X_VISUAL_TYPE,        GLX_TRUE_COLOR,
    GLX_RED_SIZE,             8,
    GLX_GREEN_SIZE,           8,
    GLX_BLUE_SIZE,            8,
    GLX_ALPHA_SIZE,           8,
    GLX_DEPTH_SIZE,           24,
    GLX_STENCIL_SIZE,         8,
    GLX_DOUBLEBUFFER,         True,
    // GLX_SAMPLE_BUFFERS,    1,
    // GLX_SAMPLES,           4,
    None
  };
  // clang-format on

  int numFbConfigs;
  GLXFBConfig* fbConfigs =
    glXChooseFBConfig(mDisplay, mScreenNumber, kVisualAttribs, &numFbConfigs);
  if (!fbConfigs || !numFbConfigs) {
    std::println(std::cerr, "glXGetFBConfigs failed");
    return;
  }

  GLXFBConfig fbConfig =
    *std::max_element(fbConfigs, fbConfigs + numFbConfigs, [&](auto a, auto b) {
      int sa, sb;
      glXGetFBConfigAttrib(mDisplay, a, GLX_SAMPLES, &sa);
      glXGetFBConfigAttrib(mDisplay, b, GLX_SAMPLES, &sb);
      return sa < sb;
    });
  XFree(fbConfigs);

  glXGetFBConfigAttrib(mDisplay, fbConfig, GLX_VISUAL_ID, &mVisualId);

  mGlxContext = glXCreateNewContext(mDisplay, fbConfig, GLX_RGBA_TYPE, 0, True);
  if (!mGlxContext) {
    std::println("glXCreateNewContext failed");
    return;
  }

  xcb_colormap_t colormap = xcb_generate_id(mEwmh.connection);
  xcb_create_colormap(mEwmh.connection,
                      XCB_COLORMAP_ALLOC_NONE,
                      colormap,
                      mScreen->root,
                      mVisualId);

  if (xcb_request_check(mEwmh.connection,
                        xcb_create_window_checked(
                          mEwmh.connection,
                          XCB_COPY_FROM_PARENT,
                          mCompositorWindow,
                          mScreen->root,
                          0,
                          0,
                          mScreen->width_in_pixels,
                          mScreen->height_in_pixels,
                          0,
                          XCB_WINDOW_CLASS_INPUT_OUTPUT,
                          mVisualId,
                          XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT |
                            XCB_CW_EVENT_MASK | XCB_CW_COLORMAP,
                          (uint32_t[]){ 0,
                                        1,
                                        (XCB_EVENT_MASK_EXPOSURE |
                                         XCB_EVENT_MASK_STRUCTURE_NOTIFY),
                                        colormap }))) {
    std::println(std::cerr, "could not create compositor window");
    return;
  }

  if (xcb_request_check(
        mEwmh.connection,
        xcb_map_window_checked(mEwmh.connection, mCompositorWindow))) {
    std::println(std::cerr, "could not map compositor window");
    return;
  }

  mGlxWindow = glXCreateWindow(mDisplay, fbConfig, mCompositorWindow, 0);
  if (!mGlxWindow) {
    std::println(std::cerr, "glXCreateWindow failed");
    return;
  }

  if (!glXMakeContextCurrent(mDisplay, mGlxWindow, mGlxWindow, mGlxContext)) {
    std::println(std::cerr, "glXMakeContextCurrent failed");
    return;
  }

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(GLDebugMessageCallback, nullptr);

  xcb_key_symbols_t* syms = xcb_key_symbols_alloc(mEwmh.connection);
  for (const auto& keybind : aKeybinds) {
    xcb_keycode_t* keycodes = xcb_key_symbols_get_keycode(syms, keybind.mKeysyms);
    if (!keycodes) {
      std::println("could not get keycodes for keysym={}", keybind.mKeysyms);
      continue;
    }

    for (xcb_keycode_t* p = keycodes; *p != XCB_NO_SYMBOL; ++p) {
      mKeybinds.emplace_back(
        Keybind::Resolved{ keybind.mMods, *p, keybind.mActions });
    }
    free(keycodes);
  }
  free(syms);

  GrabKeys();

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = SIG_IGN;
  sigaction(SIGCHLD, &sa, nullptr);

  while (waitpid(-1, nullptr, WNOHANG) > 0)
    ;

  mFlags.set(Flag_Init);
}

void
WindowManager::GrabKeys()
{
  for (const auto& keybind : mKeybinds)
    xcb_grab_key(mEwmh.connection,
                 0,
                 mScreen->root,
                 keybind.mMods,
                 keybind.mKeycode,
                 XCB_GRAB_MODE_ASYNC,
                 XCB_GRAB_MODE_ASYNC);

  xcb_flush(mEwmh.connection);
}

void
WindowManager::Run()
{
  mFlags.set(Flag_Running);

  while (IsRunning()) {
    xcb_generic_event_t* event = xcb_wait_for_event(mEwmh.connection);
    if (!event) {
      std::println(std::cerr, "lost connection to X server");
      mFlags.reset(Flag_Running);
      break;
    }

    HandleEvent(event);
    free(event);

    // TODO:
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glXSwapBuffers(mDisplay, mGlxWindow);
  }
}

void
WindowManager::Spawn(const char* const aCommand[])
{
  if (fork() != 0)
    return;

  close(xcb_get_file_descriptor(mEwmh.connection));

  int dev_null_fd = open("/dev/null", O_RDONLY);
  dup2(dev_null_fd, STDIN_FILENO);
  close(dev_null_fd);

  dev_null_fd = open("/dev/null", O_WRONLY);
  dup2(dev_null_fd, STDOUT_FILENO);
  dup2(dev_null_fd, STDERR_FILENO);
  close(dev_null_fd);

  setsid();

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = SIG_DFL;
  sigaction(SIGCHLD, &sa, nullptr);

  execvp(aCommand[0], const_cast<char**>(aCommand));
  std::println("execvp failed");
  std::exit(EXIT_FAILURE);
}
}

#if 0

void
controller::HandleClientMessage(const xcb_client_message_event_t* Event)
{
  std::cout << "ClientMessage from " << Event->window << std::endl;

  client* Client = GetClient(Event->window);
  auto _ = Client;

  if (Event->type == Conn._NET_WM_STATE) {
    const uint32_t& verb = Event->data.data32[0];
    const uint32_t& arg1 = Event->data.data32[1];
    // const uint32_t& arg2 = Event->data.data32[2];

    if (arg1 == Conn._NET_WM_STATE_HIDDEN && verb == XCB_EWMH_WM_STATE_REMOVE) {
    }

#if 0
            if (Client)
            {
                Client->Flags |= managed_client::FlagMapped;
            }
            else
            {
                // TODO: what here?
                xcb_map_window(XConn(), Event->window);
            }
#endif

    // uint32_t stack_vals[] = { XCB_STACK_MODE_ABOVE };
    // xcb_configure_window(xconn(),
    //     event->window,
    //     XCB_CONFIG_WINDOW_STACK_MODE,
    //     stack_vals);
    //

    // xcb_map_window(XConn(), event->window);

    // xcb_set_input_focus(xconn(),
    //     XCB_INPUT_FOCUS_POINTER_ROOT,
    //     event->window,
    //     XCB_CURRENT_TIME);

    // xcb_change_property(xconn(),
    //     XCB_PROP_MODE_REPLACE,
    //     root(),
    //     m_ewmh._NET_ACTIVE_WINDOW,
    //     XCB_ATOM_WINDOW,
    //     32,
    //     1,
    //     &event->window);
  }

#if 0
        switch (event->data.data32[0]) {
            case XCB_EWMH_WM_STATE_REMOVE: {
                xcb_get_property_cookie_t cookie = xcb_get_property(xconn(), 0, event->window, m_ewmh._NET_WM_STATE, XCB_ATOM_ATOM, 0, std::numeric_limits<uint32_t>::max());
                xcb_get_property_reply_t* reply = xcb_get_property_reply(xconn(), cookie, nullptr);

                if (reply) {
                    int len = xcb_get_property_value_length(reply);
                    if (!len) {
                    }

                    void* value = xcb_get_property_value(reply);

                    xcb_change_property(xconn(), XCB_PROP_MODE_REPLACE, event->window, m_ewmh._NET_WM_STATE, XCB_ATOM_ATOM, 32, 0, nullptr);
                }

                break;
            }

            case XCB_EWMH_WM_STATE_ADD: {
                break;
            }

            case XCB_EWMH_WM_STATE_TOGGLE: {
                break;
            }
        }
#endif
}

void
controller::HandleError(const xcb_generic_error_t* Error)
{
  utils::PrintXCBError(std::cerr, ErrorContext, Error);
}

client*
controller::GetClient(xcb_window_t Window)
{
  return Clients.contains(Window) ? &Clients.at(Window) : nullptr;
}

client&
controller::Manage(xcb_window_t Win)
{
  if (Clients.contains(Win))
    return Clients.at(Win);

  return Clients
    .emplace(
      std::pair{ Win, client{ .Pixmap = 0, .DesktopIndex = CurDesktopIndex } })
    .first->second;
}

void
controller::HandleConfigureRequest(const xcb_configure_request_event_t* Event)
{
  std::println("ConfigureRequest from {}", Event->window);

#if 0
    {
        xcb_get_window_attributes_cookie_t AtrrCookie = xcb_get_window_attributes(XConn(), Event->window);
        xcb_get_window_attributes_reply_t* AttrReply  = xcb_get_window_attributes_reply(XConn(), AtrrCookie, nullptr);

        xcb_get_property_cookie_t PropCookie = xcb_get_property(XConn(),
            0,
            Event->window,
            Conn._NET_WM_NAME,
            Conn.UTF8_STRING,
            0,
            std::numeric_limits<uint32_t>::max());

        xcb_get_property_reply_t* PropReply = xcb_get_property_reply(XConn(), PropCookie, nullptr);
        int                       len       = xcb_get_property_value_length(PropReply);
        const std::string_view    Name { (char*)xcb_get_property_value(PropReply), size_t(len) };

        std::println("Name: {}, Width: {}, Height: {}, Window: {}, OverrideRedirect: {}", Name, Event->width, Event->height, Event->window, AttrReply->override_redirect);

        free(AttrReply);
        free(PropReply);
    }
#endif

  if (!GetClient(Event->window)) {
  }
}

void
controller::HandleConfigureNotify(const xcb_configure_notify_event_t* Event)
{
  auto* Client = GetClient(Event->window);
  if (Client) {
    if (Client->Pixmap)
      xcb_free_pixmap(GetConn(), Client->Pixmap);
    Client->Pixmap = xcb_generate_id(GetConn());
    xcb_composite_name_window_pixmap(GetConn(), Event->window, Client->Pixmap);
    xcb_flush(GetConn());

    std::println("HandleConfigureNotify composite step");
  }
}

void
controller::HandleDestroyNotify(const xcb_destroy_notify_event_t* Event)
{
  std::println("DestroyNotify from {}", Event->window);
  Clients.erase(Event->window);
}

void
controller::HandleEntryNotify(const xcb_enter_notify_event_t* Event)
{
  std::println("EnterNotify");
}

void
controller::HandleExpose(const xcb_expose_event_t* event)
{
  std::cout << "Expose from " << event->window << std::endl;
}

void
controller::HandleFocusIn(const xcb_focus_in_event_t* event)
{
  std::cout << "FocusIn" << std::endl;
}

void
controller::HandleKeyPress(const xcb_key_press_event_t* event)
{
  auto& keycode = event->detail;
  auto& modifiers = event->state;

  for (const auto& resolved_bind : ResolvedKeybinds) {
    if (resolved_bind.KeyCode == keycode && resolved_bind.Mods == modifiers) {
      resolved_bind.Handler(this);
      break;
    }
  }
}

void
controller::HandleResizeRequest(const xcb_resize_request_event_t* Event)
{
  if (Config.DebugLog)
    std::println("ResizeRequest from {}", Event->window);
}

void
controller::HandleUnmapNotify(const xcb_unmap_notify_event_t* Event)
{
  if (Config.DebugLog)
    std::println("UnmapNotify from {}", Event->window);

  auto* Client = GetClient(Event->window);
  if (Client)
    Client->Flags &= ~(client::FlagMapped);
}


    int NumMappedClientsInWorkspace = 0;
    for (auto& [Win, Client] : Clients) {
      if (Client.DesktopIndex == CurDesktopIndex &&
          (Client.Flags & client::FlagMapped)) {
        ++NumMappedClientsInWorkspace;
        continue;
      }
    }

    if (NumMappedClientsInWorkspace > 0) {
      int i = -1;

      int NumRows = std::round(std::sqrt(NumMappedClientsInWorkspace));
      int NumCols = (NumMappedClientsInWorkspace / NumRows) +
                    ((NumMappedClientsInWorkspace % NumRows > 0) ? 1 : 0);

      uint16_t Width = Screen->width_in_pixels / NumCols;
      uint16_t Height = Screen->height_in_pixels / NumRows;

      for (auto& [Win, Client] : Clients) {
        if (Client.DesktopIndex == CurDesktopIndex) {
          // if (Client.Flags & client::FlagMapped)
          {
            Client.Width = Width;
            Client.Height = Height;

            i++;
            Client.X = (i % NumCols) * Width;
            Client.Y = (i / NumCols) * Height;

            if (Client.X != Client.CurrentX || Client.Y != Client.CurrentY ||
                Client.Width != Client.CurrentWidth ||
                Client.Height != Client.CurrentHeight) {
              uint32_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
                              XCB_CONFIG_WINDOW_WIDTH |
                              XCB_CONFIG_WINDOW_HEIGHT;
              uint32_t values[]{
                Client.X, Client.Y, Client.Width, Client.Height
              };
              xcb_configure_window(GetConn(), Win, mask, values);

              Client.CurrentX = Client.X;
              Client.CurrentY = Client.Y;
              Client.CurrentWidth = Client.Width;
              Client.CurrentHeight = Client.Height;
            }

            if (MouseX >= Client.X && MouseX <= Client.X + Client.Width &&
                MouseX >= Client.Y && MouseY <= Client.Y + Client.Height)
              Desktops[CurDesktopIndex].ActiveWin = Win;
          }

#if 0
                    xcb_composite_name_window_pixmap(GetConn(), Win, Client.Pixmap);
                    xcb_copy_area(GetConn(),
                        Client.Pixmap,
                        GetCurDesktop().Pixmap,
                        GraphicsContext,
                        0, 0,
                        Client.X, Client.Y,
                        Client.Width, Client.Height);
#endif
        }

#endif
