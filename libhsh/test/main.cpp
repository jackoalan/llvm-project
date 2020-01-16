#include <hsh.h>
#include <string_view>
using namespace std::literals;

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

constexpr std::string_view AppName = "Hello World"sv;

struct MyInstanceCreateInfo : vk::InstanceCreateInfo {
  struct MyApplicationInfo : vk::ApplicationInfo {
    constexpr MyApplicationInfo() : vk::ApplicationInfo(AppName.data(), 0, "test-engine") {}
  } AppInfo;
  std::vector<vk::LayerProperties> Layers;
  std::vector<vk::ExtensionProperties> Extensions;
  std::vector<const char*> EnabledLayers;
  std::vector<const char*> EnabledExtensions;

  bool enableLayer(std::string_view Name) {
    for (const auto &L : Layers) {
      if (!Name.compare(L.layerName)) {
        EnabledLayers.push_back(Name.data());
        return true;
      }
    }
    std::cerr << "Unable to find '" << Name << " instance layer\n";
    return false;
  }

  bool enableExtension(std::string_view Name) {
    for (const auto &E : Extensions) {
      if (!Name.compare(E.extensionName)) {
        EnabledExtensions.push_back(Name.data());
        return true;
      }
    }
    std::cerr << "Unable to find '" << Name << " instance extension\n";
    return false;
  }

  static constexpr std::string_view WantedLayers[] = {
      "VK_LAYER_LUNARG_standard_validation"sv,
  };

  static constexpr std::string_view WantedExtensions[] = {
      "VK_KHR_surface"sv,
      "VK_KHR_xcb_surface"sv,
      "VK_EXT_debug_utils"sv
  };

  MyInstanceCreateInfo() : vk::InstanceCreateInfo({}, &AppInfo) {
    Layers = vk::enumerateInstanceLayerProperties().value;
    Extensions = vk::enumerateInstanceExtensionProperties().value;

    for (auto WL : WantedLayers)
      VULKAN_HPP_ASSERT(enableLayer(WL));

    for (auto WE : WantedExtensions)
      VULKAN_HPP_ASSERT(enableExtension(WE));

    setEnabledLayerCount(EnabledLayers.size());
    setPpEnabledLayerNames(EnabledLayers.data());
    setEnabledExtensionCount(EnabledExtensions.size());
    setPpEnabledExtensionNames(EnabledExtensions.data());
  }
};

struct MyDeviceCreateInfo : vk::DeviceCreateInfo {
  float QueuePriority = 1.f;
  vk::DeviceQueueCreateInfo QueueCreateInfo{{}, 0, 1, &QueuePriority};
  std::vector<vk::LayerProperties> Layers;
  std::vector<vk::ExtensionProperties> Extensions;
  std::vector<const char*> EnabledLayers;
  std::vector<const char*> EnabledExtensions;
  vk::PhysicalDeviceFeatures EnabledFeatures;

  bool enableLayer(std::string_view Name) {
    for (const auto &L : Layers) {
      if (!Name.compare(L.layerName)) {
        EnabledLayers.push_back(Name.data());
        return true;
      }
    }
    std::cerr << "Unable to find '" << Name << " device layer\n";
    return false;
  }

  bool enableExtension(std::string_view Name) {
    for (const auto &E : Extensions) {
      if (!Name.compare(E.extensionName)) {
        EnabledExtensions.push_back(Name.data());
        return true;
      }
    }
    std::cerr << "Unable to find '" << Name << " device extension\n";
    return false;
  }

  static constexpr std::string_view WantedLayers[] = {
      "VK_LAYER_LUNARG_standard_validation"sv,
  };

  static constexpr std::string_view WantedExtensions[] = {
      "VK_KHR_swapchain"sv,
      "VK_KHR_get_memory_requirements2"sv,
      "VK_KHR_dedicated_allocation"sv
  };

  explicit MyDeviceCreateInfo(vk::PhysicalDevice PD, uint32_t &QFIdxOut)
  : vk::DeviceCreateInfo({}, 1, &QueueCreateInfo, 0, nullptr, 0, nullptr, &EnabledFeatures) {
    Layers = PD.enumerateDeviceLayerProperties().value;
    Extensions = PD.enumerateDeviceExtensionProperties().value;

    for (auto WL : WantedLayers)
      VULKAN_HPP_ASSERT(enableLayer(WL));

    for (auto WE : WantedExtensions)
      VULKAN_HPP_ASSERT(enableExtension(WE));

    setEnabledLayerCount(EnabledLayers.size());
    setPpEnabledLayerNames(EnabledLayers.data());
    setEnabledExtensionCount(EnabledExtensions.size());
    setPpEnabledExtensionNames(EnabledExtensions.data());

    uint32_t QFIdx = 0;
    bool FoundQF = false;
    for (const auto &QF : PD.getQueueFamilyProperties()) {
      if (QF.queueFlags & vk::QueueFlagBits::eGraphics) {
        FoundQF = true;
        break;
      }
      ++QFIdx;
    }
    VULKAN_HPP_ASSERT(FoundQF);
    QFIdxOut = QFIdx;
    QueueCreateInfo.setQueueFamilyIndex(QFIdx);

    auto Features = PD.getFeatures();
    EnabledFeatures.geometryShader = Features.geometryShader;
    EnabledFeatures.tessellationShader = Features.tessellationShader;
    EnabledFeatures.samplerAnisotropy = Features.samplerAnisotropy;
    EnabledFeatures.textureCompressionBC = Features.textureCompressionBC;
  }
};

struct MyDebugUtilsMessengerCreateInfo : vk::DebugUtilsMessengerCreateInfoEXT {
  static VkBool32
  Callback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
           vk::DebugUtilsMessageTypeFlagBitsEXT messageTypes,
           const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
           void *pUserData) {
    std::cerr << to_string(messageSeverity) << " " << to_string(messageTypes)
              << " " << pCallbackData->pMessage << "\n";
    return VK_FALSE;
  }

  static constexpr vk::DebugUtilsMessageSeverityFlagsEXT WantedFlags =
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
      vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
  static constexpr vk::DebugUtilsMessageTypeFlagsEXT WantedTypes =
      vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
      vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
      vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

  MyDebugUtilsMessengerCreateInfo()
      : vk::DebugUtilsMessengerCreateInfoEXT(
            {}, WantedFlags, WantedTypes,
            PFN_vkDebugUtilsMessengerCallbackEXT(&Callback)) {}
};

struct XcbConnection;
struct XcbWindow {
  XcbConnection &Connection;
  xcb_window_t Window;
  explicit XcbWindow(XcbConnection &Connection);
  ~XcbWindow();
};

struct XcbConnection {
  xcb_connection_t *Connection;
  xcb_atom_t wmDeleteWin, wmProtocols;

  operator xcb_connection_t *() const { return Connection; }

  XcbConnection() {
    Connection = xcb_connect(nullptr, nullptr);
    VULKAN_HPP_ASSERT(Connection);

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

  XcbWindow makeWindow() {
    return XcbWindow(*this);
  }

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

XcbWindow::XcbWindow(XcbConnection &Connection) : Connection(Connection) {
  const struct xcb_setup_t *Setup = xcb_get_setup(Connection);
  xcb_screen_iterator_t Screen = xcb_setup_roots_iterator(Setup);
  VULKAN_HPP_ASSERT(Screen.rem);

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
                      Connection.wmProtocols, 4, 32, 1, &Connection.wmDeleteWin);

  xcb_map_window(Connection, Window);
  xcb_flush(Connection);
}

XcbWindow::~XcbWindow() {
  xcb_destroy_window(Connection, Window);
}

struct MyXcbSurfaceCreateInfo : vk::XcbSurfaceCreateInfoKHR {
  MyXcbSurfaceCreateInfo(const XcbWindow &Window)
      : vk::XcbSurfaceCreateInfoKHR({}, Window.Connection, Window.Window) {}
};

struct MySwapchainCreateInfo : vk::SwapchainCreateInfoKHR {
  explicit MySwapchainCreateInfo(vk::PhysicalDevice PD, vk::SurfaceKHR Surface)
  : vk::SwapchainCreateInfoKHR({}, Surface) {
    auto Capabilities = PD.getSurfaceCapabilitiesKHR(Surface).value;
    setMinImageCount(std::max(2u, Capabilities.minImageCount));
    vk::SurfaceFormatKHR UseFormat;
    for (auto &Format : PD.getSurfaceFormatsKHR(Surface).value) {
      if (Format.format == vk::Format::eB8G8R8A8Unorm) {
        UseFormat = Format;
        break;
      }
    }
    VULKAN_HPP_ASSERT(UseFormat.format != vk::Format::eUndefined);
    setImageFormat(UseFormat.format).setImageColorSpace(UseFormat.colorSpace);
    setImageExtent(Capabilities.currentExtent);
    setImageArrayLayers(1);
    constexpr auto WantedUsage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment;
    VULKAN_HPP_ASSERT((Capabilities.supportedUsageFlags & WantedUsage) == WantedUsage);
    setImageUsage(WantedUsage);
    setPresentMode(vk::PresentModeKHR::eFifo);
  }
};

struct MyDescriptorPoolCreateInfo : vk::DescriptorPoolCreateInfo {
  MyDescriptorPoolCreateInfo() : vk::DescriptorPoolCreateInfo({}, ) {

  }
};

struct MyPresentInfo : vk::PresentInfoKHR {
  vk::Semaphore Semaphore_;
  vk::SwapchainKHR Swapchain_;
  uint32_t ImageIndex_;
  MyPresentInfo(vk::Semaphore Semaphore, vk::SwapchainKHR Swapchain, uint32_t ImageIndex)
  : vk::PresentInfoKHR(1, &Semaphore_, 1, &Swapchain_, &ImageIndex_), Semaphore_(Semaphore),
    Swapchain_(Swapchain), ImageIndex_(ImageIndex) {}
};

int main(int argc, char** argv) {
  XcbConnection Connection;
  XcbWindow Window = Connection.makeWindow();

  vk::DynamicLoader Loader;
  VULKAN_HPP_ASSERT(Loader.success());
  auto GetInstanceProcAddr = Loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
  VULKAN_HPP_ASSERT(GetInstanceProcAddr);
  VULKAN_HPP_DEFAULT_DISPATCHER.init(GetInstanceProcAddr);

  auto Instance = vk::createInstanceUnique(MyInstanceCreateInfo()).value;
  VULKAN_HPP_DEFAULT_DISPATCHER.init(*Instance);

  auto Messenger = Instance->createDebugUtilsMessengerEXTUnique(MyDebugUtilsMessengerCreateInfo()).value;

  auto PhysDevices = Instance->enumeratePhysicalDevices().value;
  for (auto PD : PhysDevices) {
    uint32_t QFIdx = 0;
    auto Device = PD.createDeviceUnique(MyDeviceCreateInfo(PD, QFIdx)).value;
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*Device);

    auto Surface = Instance->createXcbSurfaceKHR(MyXcbSurfaceCreateInfo(Window)).value;
    auto Swapchain = Device->createSwapchainKHRUnique(MySwapchainCreateInfo(PD, Surface)).value;
    auto SwapchainImages = Device->getSwapchainImagesKHR(*Swapchain);
    auto Queue = Device->getQueue(QFIdx, 0);
    Device->createDescriptorPool()
    auto PresentCompleteSem = Device->createSemaphoreUnique({}).value;

    uint32_t Frame = 0;
    Connection.runloop([&]() {
      uint32_t Index;
      auto Res = Device->acquireNextImageKHR(*Swapchain, 500000000, *PresentCompleteSem, {}, &Index);
      std::cerr << "SC " << vk::to_string(Res) << " " << Index << " " << Frame++ << "\n";
      Queue.presentKHR(MyPresentInfo(*PresentCompleteSem, *Swapchain, Index));
      return true;
    });

    break;
  }

  return 0;
}
