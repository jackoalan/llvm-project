#define HSH_IMPLEMENTATION

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <hsh/hsh.h>
#include <iostream>
#include <string_view>
using namespace std::literals;

#include "test-input.h"
#include "urde-test.h"

constexpr std::string_view AppName = "Hello Triangle"sv;

#if !HSH_PROFILE_MODE

#if _WIN32

#define NOMINMAX
#include <Windows.h>

struct WsiConnection;
struct WsiWindow {
  WsiConnection &Connection;
  HWND Window;
  explicit WsiWindow(WsiConnection &Connection);
  ~WsiWindow();
};

struct WsiConnection {
  HINSTANCE Instance;
  bool Running = true;

  operator HINSTANCE() const { return Instance; }

  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam) {
    if (msg == WM_DESTROY)
      PostQuitMessage(0);
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }

  WsiConnection() {
    Instance = GetModuleHandle(nullptr);
    assert(Instance);

    WNDCLASSEX Class{};

    Class.cbSize = sizeof(Class);
    Class.style = 0;
    Class.lpfnWndProc = &WindowProc;
    Class.cbClsExtra = 0;
    Class.cbWndExtra = 0;
    Class.hInstance = Instance;
    Class.hIcon = nullptr;
    Class.hCursor = nullptr;
    Class.hbrBackground = HBRUSH(GetStockObject(BLACK_BRUSH));
    Class.lpszMenuName = nullptr;
    Class.lpszClassName = TEXT("TestappWndClass");
    Class.hIconSm = nullptr;

    ATOM ClassAtom = RegisterClassEx(&Class);
    assert(ClassAtom);
  }

  WsiWindow makeWindow() { return WsiWindow(*this); }

  void _handleMsg(const MSG& Msg) {
    switch (Msg.message) {
    case WM_QUIT:
      Running = false;
      break;
    default:
      TranslateMessage(&Msg);
      DispatchMessage(&Msg);
      break;
    }
  }

  void runloop(const std::function<bool()> &IdleFunc) {
    while (Running) {
      MSG Msg;
      while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
        _handleMsg(Msg);
      if (!IdleFunc()) {
        Running = false;
        break;
      }
    }
  }

  void dispatchLatestEvents() {
    MSG Msg;
    while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
      _handleMsg(Msg);
  }
};

WsiWindow::WsiWindow(WsiConnection &Connection) : Connection(Connection) {
  Window = CreateWindow(TEXT("TestappWndClass"), TEXT("HSH Test App"),
                        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 512,
                        512, (HWND) nullptr, (HMENU) nullptr, Connection,
                        (LPVOID) nullptr);
  ShowWindow(Window, SW_SHOW);
}

WsiWindow::~WsiWindow() { DestroyWindow(Window); }

struct PipelineCacheFileManager {
  static std::wstring GetFilename(const uint8_t UUID[VK_UUID_SIZE]) noexcept {
    std::wostringstream FileName;
    // TODO: make this more portable
    wchar_t LocalAppData[MAX_PATH];
    if (GetEnvironmentVariable(TEXT("LOCALAPPDATA"), LocalAppData, MAX_PATH)) {
      FileName << LocalAppData;
      FileName << TEXT("/cache/hsh-test-pipeline-cache-");
      for (int i = 0; i < VK_UUID_SIZE; ++i)
        FileName << std::hex << unsigned(UUID[i]);
      FileName << TEXT(".bin");
    }
    return FileName.str();
  }

  template <typename Func>
  static void ReadPipelineCache(Func F,
                                const uint8_t UUID[VK_UUID_SIZE]) noexcept {
    if (std::FILE *File = ::_wfopen(GetFilename(UUID).c_str(), TEXT("rb"))) {
      std::fseek(File, 0, SEEK_END);
      auto Size = std::ftell(File);
      if (Size != 0) {
        std::fseek(File, 0, SEEK_SET);
        std::unique_ptr<uint8_t[]> Data(new uint8_t[Size]);
        Size = std::fread(Data.get(), 1, Size, File);
        if (Size != 0)
          F(Data.get(), Size);
      }
      std::fclose(File);
    }
  }

  template <typename Func>
  static void WritePipelineCache(Func F,
                                 const uint8_t UUID[VK_UUID_SIZE]) noexcept {
    if (std::FILE *File = ::_wfopen(GetFilename(UUID).c_str(), TEXT("wb"))) {
      F([File](const uint8_t *Data, std::size_t Size) {
        std::fwrite(Data, 1, Size, File);
      });
      std::fclose(File);
    }
  }
};

#elif defined(__APPLE__)

#include <AppKit/AppKit.h>
#include <QuartzCore/CoreAnimation.h>

@interface MetalView : NSView
- (id)initWithFrame:(NSRect)frameRect;
- (CALayer *)makeBackingLayer;
@end

@implementation MetalView
- (id)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  self.wantsLayer = YES;
  return self;
}
- (CALayer *)makeBackingLayer {
  CAMetalLayer *Layer = [CAMetalLayer layer];
  Layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
  return Layer;
}
- (BOOL)layer:(CALayer *)layer
    shouldInheritContentsScale:(CGFloat)newScale
                    fromWindow:(NSWindow *)window {
  return YES;
}
@end

@interface CocoaWindow : NSWindow
- (id)init;
@end

@implementation CocoaWindow
- (id)init {
  NSRect ContentRect = NSMakeRect(0, 0, 512, 512);
  self = [super
      initWithContentRect:ContentRect
                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                          NSWindowStyleMaskMiniaturizable |
                          NSWindowStyleMaskResizable /*|
                          NSWindowStyleMaskFullSizeContentView*/
                  backing:NSBackingStoreBuffered
                    defer:NO];
  self.contentView = [[MetalView alloc] initWithFrame:ContentRect];
  return self;
}
@end

struct WsiConnection;
struct WsiWindow {
  WsiConnection &Connection;
  CocoaWindow *Window;
  explicit WsiWindow(WsiConnection &Connection);
  ~WsiWindow();
};

@interface Application : NSApplication <NSApplicationDelegate>
@end

@implementation Application
- (id)init {
  self = [super init];
  self.activationPolicy = NSApplicationActivationPolicyRegular;
  self.delegate = self;

  NSMenu *Menu = [NSMenu new];
  NSMenuItem *MenuItem = [NSMenuItem new];
  [Menu addItem:MenuItem];
  NSMenu *AppMenu = [NSMenu new];
  NSString *AppName = [[NSProcessInfo processInfo] processName];
  NSString *QuitTitle = [@"Quit " stringByAppendingString:AppName];
  NSMenuItem *QuitMenuItem =
      [[NSMenuItem alloc] initWithTitle:QuitTitle
                                 action:@selector(terminate:)
                          keyEquivalent:@"q"];
  [AppMenu addItem:QuitMenuItem];
  [MenuItem setSubmenu:AppMenu];
  self.mainMenu = Menu;

  return self;
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:
    (NSApplication *)sender {
  return YES;
}
- (void)terminate:(id)sender {
  /* Use -stop in place of -terminate so -run returns */
  [self stop:sender];
}
@end

struct WsiConnection {
  Application *App;

  operator NSApplication *() const { return App; }

  WsiConnection() { App = [Application sharedApplication]; }

  WsiWindow makeWindow() { return WsiWindow(*this); }

  const std::function<bool()> *IdleFuncPtr = nullptr;

  void idle() {
    if (!(*IdleFuncPtr)())
      [App stop:nil];
    if (App.isRunning)
      dispatch_async_f(dispatch_get_main_queue(), this,
                       dispatch_function_t(_idle));
  }
  static void _idle(WsiConnection *Self) { Self->idle(); }

  void runloop(const std::function<bool()> &IdleFunc) {
    IdleFuncPtr = &IdleFunc;
    dispatch_async_f(dispatch_get_main_queue(), this,
                     dispatch_function_t(_idle));
    [App run];
    IdleFuncPtr = nullptr;
  }

  void dispatchLatestEvents() {
    while (NSEvent *Event = [App nextEventMatchingMask:NSEventMaskAny
                                             untilDate:nullptr
                                                inMode:NSDefaultRunLoopMode
                                               dequeue:YES])
      [App sendEvent:Event];
  }
};

WsiWindow::WsiWindow(WsiConnection &Connection) : Connection(Connection) {
  Window = [CocoaWindow new];
  [Window makeKeyAndOrderFront:nil];
}

WsiWindow::~WsiWindow() { [Window close]; }

struct PipelineCacheFileManager {
  static NSURL *GetFilename() noexcept {
    std::ostringstream FileName;
    // TODO: make this more portable
    if (const char *home = std::getenv("HOME")) {
      FileName << home;
      FileName << "/Library/Caches/hsh-test-pipeline-cache.bin";
    }
    return [NSURL fileURLWithPath:@(FileName.str().c_str())];
  }

  template <typename Func> static void ReadPipelineCache(Func F) noexcept {
    F(GetFilename());
  }

  template <typename Func> static void WritePipelineCache(Func F) noexcept {
    F(GetFilename());
  }
};

#else

struct WsiConnection;
struct WsiWindow {
  WsiConnection &Connection;
  xcb_window_t Window;
  explicit WsiWindow(WsiConnection &Connection);
  ~WsiWindow();
};

struct WsiConnection {
  xcb_connection_t *Connection;
  xcb_atom_t wmDeleteWin, wmProtocols;

  operator xcb_connection_t *() const { return Connection; }

  WsiConnection() {
    Connection = xcb_connect(nullptr, nullptr);
    assert(Connection);

    xcb_intern_atom_cookie_t wmDeleteCookie = xcb_intern_atom(
        Connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    xcb_intern_atom_cookie_t wmProtocolsCookie =
        xcb_intern_atom(Connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");

    xcb_intern_atom_reply_t *wmDeleteReply =
        xcb_intern_atom_reply(Connection, wmDeleteCookie, nullptr);
    xcb_intern_atom_reply_t *wmProtocolsReply =
        xcb_intern_atom_reply(Connection, wmProtocolsCookie, nullptr);

    wmDeleteWin = wmDeleteReply->atom;
    wmProtocols = wmProtocolsReply->atom;
  }

  WsiWindow makeWindow() { return WsiWindow(*this); }

  void runloop(const std::function<bool()> &IdleFunc) {
    bool Running = true;
    while (Running) {
      xcb_generic_event_t *event = xcb_poll_for_event(Connection);
      if (!event) {
        if (!IdleFunc())
          break;
        continue;
      }

      switch (event->response_type & ~0x80u) {
      case XCB_CLIENT_MESSAGE: {
        auto *cm = (xcb_client_message_event_t *)event;

        if (cm->data.data32[0] == wmDeleteWin)
          Running = false;

        break;
      }
      default:
        break;
      }

      free(event);
    }
  }
};

WsiWindow::WsiWindow(WsiConnection &Connection) : Connection(Connection) {
  const struct xcb_setup_t *Setup = xcb_get_setup(Connection);
  xcb_screen_iterator_t Screen = xcb_setup_roots_iterator(Setup);
  assert(Screen.rem);

  Window = xcb_generate_id(Connection);
  uint32_t EventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  uint32_t ValueList[] = {Screen.data->black_pixel, 0};
  xcb_create_window(Connection, XCB_COPY_FROM_PARENT, Window, Screen.data->root,
                    0, 0, 512, 512, 0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
                    Screen.data->root_visual, EventMask, ValueList);
  xcb_change_property(Connection, XCB_PROP_MODE_REPLACE, Window,
                      XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, AppName.size(),
                      AppName.data());
  xcb_change_property(Connection, XCB_PROP_MODE_REPLACE, Window,
                      Connection.wmProtocols, 4, 32, 1,
                      &Connection.wmDeleteWin);

  xcb_map_window(Connection, Window);
  xcb_flush(Connection);
}

WsiWindow::~WsiWindow() { xcb_destroy_window(Connection, Window); }

struct PipelineCacheFileManager {
  static std::string GetFilename(const uint8_t UUID[VK_UUID_SIZE]) noexcept {
    std::ostringstream FileName;
    // TODO: make this more portable
    if (const char *home = std::getenv("HOME")) {
      FileName << home;
      FileName << "/.cache/hsh-test-pipeline-cache-";
      for (int i = 0; i < VK_UUID_SIZE; ++i)
        FileName << std::hex << unsigned(UUID[i]);
      FileName << ".bin";
    }
    return FileName.str();
  }

  template <typename Func>
  static void ReadPipelineCache(Func F,
                                const uint8_t UUID[VK_UUID_SIZE]) noexcept {
    if (std::FILE *File = std::fopen(GetFilename(UUID).c_str(), "rb")) {
      std::fseek(File, 0, SEEK_END);
      auto Size = std::ftell(File);
      if (Size != 0) {
        std::fseek(File, 0, SEEK_SET);
        std::unique_ptr<uint8_t[]> Data(new uint8_t[Size]);
        Size = std::fread(Data.get(), 1, Size, File);
        if (Size != 0)
          F(Data.get(), Size);
      }
      std::fclose(File);
    }
  }

  template <typename Func>
  static void WritePipelineCache(Func F,
                                 const uint8_t UUID[VK_UUID_SIZE]) noexcept {
    if (std::FILE *File = std::fopen(GetFilename(UUID).c_str(), "wb")) {
      F([File](const uint8_t *Data, std::size_t Size) {
        std::fwrite(Data, 1, Size, File);
      });
      std::fclose(File);
    }
  }
};

#endif

#if _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
  WsiConnection Connection;
  WsiWindow Window = Connection.makeWindow();

#ifndef __APPLE__

  auto Instance = hsh::create_vulkan_instance(
      AppName.data(), 0, "test-engine", 0,
      [](vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
         vk::DebugUtilsMessageTypeFlagBitsEXT messageTypes,
         const vk::DebugUtilsMessengerCallbackDataEXT &pCallbackData) {
        std::cerr << to_string(messageSeverity) << " "
                  << to_string(messageTypes) << " " << pCallbackData.pMessage
                  << "\n";
      });
  if (!Instance)
    return 1;

  Instance.enumerate_vulkan_devices(
      [](const vk::PhysicalDeviceProperties &Props,
         const vk::PhysicalDeviceDriverProperties &DriverProps) {
        std::cerr << "name: " << Props.deviceName
                  << " type: " << vk::to_string(Props.deviceType) << "\n";
        return false;
      });

  auto PhysSurface = Instance.create_phys_surface(Connection, Window.Window);
  if (!PhysSurface) {
    std::cerr << "Unable to create PhysSurface\n";
    return 1;
  }

  auto Device = Instance.enumerate_vulkan_devices(
      [&](const vk::PhysicalDeviceProperties &Props,
          const vk::PhysicalDeviceDriverProperties &DriverProps) {
        return true;
      },
      *PhysSurface);
  if (!Device) {
    std::cerr << "No vulkan devices found\n";
    return 1;
  }

#else

  auto Instance = hsh::create_metal_instance(
      [](NSError *Err) { std::cerr << Err.localizedDescription.UTF8String; });
  if (!Instance)
    return 1;

  Instance.enumerate_metal_devices(
      [](id<MTLDevice> Device, bool NativeForPhysSurface) {
        std::cerr << "name: " << Device.name.UTF8String << "\n";
        return false;
      });

  CAMetalLayer *PhysSurface = (CAMetalLayer *)Window.Window.contentView.layer;

  auto Device = Instance.enumerate_metal_devices(
      [&](id<MTLDevice> Device, bool NativeForPhysSurface) {
        return NativeForPhysSurface;
      },
      PhysSurface);
  if (!Device) {
    std::cerr << "No metal devices found\n";
    return 1;
  }

#endif

  auto Surface = hsh::create_surface(std::move(PhysSurface));
  if (!Surface) {
    std::cerr << "PhysSurface not compatible\n";
    return 1;
  }

  auto RenderTexture = hsh::create_render_texture2d(Surface);

  {
    hsh::ShaderFileMapper SFM;
    if (!SFM.Good) {
      std::cerr << "Unable to map shader data\n";
      return 1;
    }
    PipelineCacheFileManager PCFM;
    Device.build_pipelines(PCFM);
  }

  MyNS::Binding PipelineBind{};
  MyNS::Binding PipelineTemplate1Bind{};
  MyNS::Binding PipelineTemplate2Bind{};
  MyNS::Binding PipelineTemplate3Bind{};

  ModelResources ModRes{};
  hsh::binding ModelBinding;
  hsh::uniform_fifo UFifo;
  hsh::vertex_fifo VFifo;

  std::size_t CurColor = 0;
  constexpr std::array<hsh::float4, 7> Rainbow{{{1.f, 0.f, 0.f, 1.f},
                                                {1.f, 0.5f, 0.f, 1.f},
                                                {1.f, 1.f, 0.f, 1.f},
                                                {0.f, 1.f, 0.f, 1.f},
                                                {0.f, 1.f, 1.f, 1.f},
                                                {0.f, 0.f, 1.f, 1.f},
                                                {0.5f, 0.f, 1.f, 1.f}}};

  Connection.runloop([&]() {
    Device.enter_draw_context([&]() {
      if (!UFifo)
        UFifo = hsh::create_uniform_fifo(
            256 +
            sizeof(
                MyNS::UniformData)); // Maximum alignment by vulkan spec is 256
      if (!VFifo)
        VFifo = hsh::create_vertex_fifo(sizeof(MyNS::MyFormat) * 3 * 2);

      if (Surface.acquire_next_image()) {
        Connection.dispatchLatestEvents();
        RenderTexture.attach();
        hsh::clear_attachments();

        auto UFifoBinding =
            UFifo.map<MyNS::UniformData>([&](MyNS::UniformData &UniData) {
              UniData = MyNS::UniformData{};
              UniData.xf[0][0] = 1.f;
              UniData.xf[1][1] = 1.f;
              UniData.xf[2][2] = 1.f;
              UniData.xf[3][3] = 1.f;
              // UniData.color = Rainbow[CurColor];
              UniData.color = hsh::float4(1.f, 1.f, 1.f, 1.f);
            });
        auto VFifoBinding =
            VFifo.map<MyNS::MyFormat>(3, [&](MyNS::MyFormat *VertData) {
              VertData[0] = MyNS::MyFormat{
                  hsh::float3{-1.f, -1.f, 0.f}, {}, Rainbow[CurColor]};
              VertData[1] =
                  MyNS::MyFormat{hsh::float3{1.f, -1.f, 0.f},
                                 {},
                                 Rainbow[(CurColor + 1) % Rainbow.size()]};
              VertData[2] =
                  MyNS::MyFormat{hsh::float3{1.f, 1.f, 0.f},
                                 {},
                                 Rainbow[(CurColor + 2) % Rainbow.size()]};
            });
        // CurColor = (CurColor + 1) % Rainbow.size();
        MyNS::BindPipeline(PipelineBind, UFifoBinding, VFifoBinding).draw(0, 3);

        RenderTexture.resolve_surface(Surface.get());
      }
    });
    return true;
  });

  return 0;
}

#else

int main(int argc, char **argv) {
  MyNS::BuildPipelineTemplated(false, MyNS::AM_Alpha);
  MyNS::BuildPipelineTemplated(false, MyNS::AM_NoAlpha);
  MyNS::BuildPipelineTemplated(true, MyNS::AM_NoAlpha);

  ModelInfo ModInfo = CreateModelInfo();
  MaterialInfo MatInfo = CreateMaterialInfo();
  ModelResources ModRes = CreateModelResources();
  BindDrawModel(ModInfo, MatInfo, PT_Normal, ModRes);

  return 0;
}

#endif
