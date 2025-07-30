/**
 * THIS FILE CONTAINS ALL CODE I WOULD NORMALLY DELETE
 */

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
                          XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
          uint32_t values[]{ Client.X, Client.Y, Client.Width, Client.Height };
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

#include <cassert>
#include <cstdlib>
#include <ostream>
#include <print>

#include <xcb/xcb_errors.h>
#include <xcb/xproto.h>

    namespace nyla {

    void PrintAtomName(std::ostream& out,
                       xcb_ewmh_connection_t& ewmh,
                       const xcb_atom_t atom)
    {
      if (!atom) {
        std::println(out, "None");
        return;
      }

      auto cookie = xcb_get_atom_name(ewmh.connection, atom);
      auto* reply = xcb_get_atom_name_reply(ewmh.connection, cookie, nullptr);
      out << std::string_view{ xcb_get_atom_name_name(reply),
                               size_t(xcb_get_atom_name_name_length(reply)) };
      free(reply);
    }

    void PrintXCBError(std::ostream& out,
                       xcb_errors_context_t* error_context,
                       const xcb_generic_error_t* error)
    {
      std::println(out,
                   "error(code={}): Bad{}",
                   int(error->error_code),
                   xcb_errors_get_name_for_error(
                     error_context, error->error_code, nullptr));
    }

    void PropertyRemoveAtom(xcb_connection_t* Conn,
                            xcb_window_t Win,
                            xcb_atom_t Property,
                            xcb_atom_t Val)
    {
      auto Cookie = xcb_get_property(Conn,
                                     0,
                                     Win,
                                     Property,
                                     XCB_ATOM_ATOM,
                                     0,
                                     std::numeric_limits<uint32_t>::max());
      auto* Reply = xcb_get_property_reply(Conn, Cookie, nullptr);

      int n = xcb_get_property_value_length(Reply);
      auto OldVals = static_cast<xcb_atom_t*>(xcb_get_property_value(Reply));

      assert(n <= 64);
      xcb_atom_t NewVals[64];

      int c = 0;
      for (int i = 0; i < n; ++i) {
        if (OldVals[i] != Val)
          NewVals[c++] = OldVals[i];
      }

      xcb_change_property(Conn,
                          XCB_PROP_MODE_REPLACE,
                          Win,
                          Property,
                          XCB_ATOM_ATOM,
                          32,
                          c,
                          NewVals);
      xcb_flush(Conn);
      free(Reply);
    }

    void PropertyAddAtom(xcb_connection_t* Conn,
                         xcb_window_t Win,
                         xcb_atom_t Property,
                         xcb_atom_t Val)
    {
      xcb_change_property(
        Conn, XCB_PROP_MODE_APPEND, Win, Property, XCB_ATOM_ATOM, 32, 1, &Val);
      xcb_flush(Conn);
    }

    bool PropertyHasAtom(xcb_connection_t* Conn,
                         xcb_window_t Win,
                         xcb_atom_t Property,
                         xcb_atom_t Val)
    {
      auto Cookie = xcb_get_property(Conn,
                                     0,
                                     Win,
                                     Property,
                                     XCB_ATOM_ATOM,
                                     0,
                                     std::numeric_limits<uint32_t>::max());
      auto* Reply = xcb_get_property_reply(Conn, Cookie, nullptr);

      int n = xcb_get_property_value_length(Reply);
      auto OldVals = static_cast<xcb_atom_t*>(xcb_get_property_value(Reply));

      for (int i = 0; i < n; ++i) {
        if (OldVals[i] == Val) {
          free(Reply);
          return true;
        }
      }

      free(Reply);
      return false;
    }

    void PropertyToggleAtom(xcb_connection_t* Conn,
                            xcb_window_t Win,
                            xcb_atom_t Property,
                            xcb_atom_t Val)
    {
      if (PropertyHasAtom(Conn, Win, Property, Val)) {
        PropertyRemoveAtom(Conn, Win, Property, Val);
      } else {
        PropertyAddAtom(Conn, Win, Property, Val);
      }
    }

    }

    void WindowManager::RenderAll()
    {
      // clang-format off
  float vertices[] = {
      // positions          // texture coords
      -0.5f,  0.5f, 0.0f,   0.0f, 1.0f,   // top left 
       0.5f,  0.5f, 0.0f,   1.0f, 1.0f,   // top right
      -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,   // bottom left
                                                              
       0.5f,  0.5f, 0.0f,   1.0f, 1.0f,   // top right
       0.5f, -0.5f, 0.0f,   1.0f, 0.0f,   // bottom right
      -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,   // bottom left
  };
      // clang-format on

      static GLuint vao = 0;
      if (!vao) {
        glGenVertexArrays(1, &vao);
      }

      glBindVertexArray(vao);

      static GLuint vbo = 0;
      if (!vbo) {
        glGenBuffers(1, &vbo);
      }

      glBindBuffer(GL_ARRAY_BUFFER, vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

      static GLuint vertexShader = 0;
      if (!vertexShader) {
        vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &gVertexShaderSrc, nullptr);
        glCompileShader(vertexShader);

        int success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (success) {
          std::println(std::clog, "vertex shader successfully compiled");
        } else {
          std::println(std::cerr, "vertex shader compilation failed");

          char infoLog[512];
          memset(infoLog, 0, sizeof(infoLog));
          glGetShaderInfoLog(
            vertexShader, sizeof(infoLog) - 1, nullptr, infoLog);
          std::println("{}\n\n{}", gVertexShaderSrc, infoLog);
        }
      }

      static GLuint fragmentShader = 0;
      if (!fragmentShader) {
        fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &gFragmentShaderSrc, nullptr);
        glCompileShader(fragmentShader);

        int success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (success) {
          std::println(std::clog, "fragment shader successfully compiled");
        } else {
          std::println(std::cerr, "fragment shader compilation failed");
        }
      }

      static GLuint shaderProgram = 0;
      if (!shaderProgram) {
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        int success;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (success) {
          std::println(std::clog, "shader program successfully linked");
          glDeleteShader(vertexShader);
          glDeleteShader(fragmentShader);
        } else {
          std::println(std::cerr, "shader program link failed");
        }
      }

      glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
      glEnableVertexAttribArray(0);

      glVertexAttribPointer(1,
                            2,
                            GL_FLOAT,
                            GL_FALSE,
                            5 * sizeof(float),
                            (void*)(3 * sizeof(float)));
      glEnableVertexAttribArray(1);

      // clang-format off
  static const int kPixmapAttribs[] = {
      GLX_TEXTURE_TARGET_EXT, GLX_TEXTURE_2D_EXT,
      GLX_TEXTURE_FORMAT_EXT, GLX_TEXTURE_FORMAT_RGB_EXT,
      /*GLX_MIPMAP_TEXTURE_EXT, True,*/
      None
  };
      // clang-format on

      for (auto& [window, client] : mClients) {
        if (!client.mGLXPixmap)
          continue;

        GLuint texture = 0;
        glGenTextures(1, &texture);

        glBindTexture(GL_TEXTURE_2D, texture);

        client.mGLXPixmap =
          glXCreatePixmap(mDisplay, mFbConfig, client.mXPixmap, kPixmapAttribs);

        glXBindTexImageEXT(mDisplay, client.mGLXPixmap, GLX_FRONT_EXT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(30ms);
      }

      glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      glUseProgram(shaderProgram);
      glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices));

      glXSwapBuffers(mDisplay, mGlxWindow);

      using namespace std::chrono_literals;
      std::this_thread::sleep_for(25ms);
    }
