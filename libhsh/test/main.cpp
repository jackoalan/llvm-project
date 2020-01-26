#define VMA_IMPLEMENTATION
#include <hsh/hsh.h>
#include <string_view>
using namespace std::literals;

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

constexpr std::string_view AppName = "Hello World"sv;

struct MyInstanceCreateInfo : vk::InstanceCreateInfo {
  struct MyApplicationInfo : vk::ApplicationInfo {
    constexpr MyApplicationInfo()
        : vk::ApplicationInfo(AppName.data(), 0, "test-engine", 0,
                              VK_API_VERSION_1_1) {}
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

  bool enableExtension(std::string_view Name, bool Error = true) {
    for (const auto &E : Extensions) {
      if (!Name.compare(E.extensionName)) {
        EnabledExtensions.push_back(Name.data());
        return true;
      }
    }
    if (Error)
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

  explicit MyDeviceCreateInfo(vk::PhysicalDevice PD, uint32_t &QFIdxOut, bool &HasExtMemoryBudget)
  : vk::DeviceCreateInfo({}, 1, &QueueCreateInfo, 0, nullptr, 0, nullptr, &EnabledFeatures) {
    Layers = PD.enumerateDeviceLayerProperties().value;
    Extensions = PD.enumerateDeviceExtensionProperties().value;

    for (auto WL : WantedLayers)
      VULKAN_HPP_ASSERT(enableLayer(WL));

    for (auto WE : WantedExtensions)
      VULKAN_HPP_ASSERT(enableExtension(WE));

    HasExtMemoryBudget = enableExtension("VK_EXT_memory_budget"sv, false);

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
  explicit MySwapchainCreateInfo(vk::PhysicalDevice PD, vk::SurfaceKHR Surface, vk::Format &FmtOut)
  : vk::SwapchainCreateInfoKHR({}, Surface) {
    auto Capabilities = PD.getSurfaceCapabilitiesKHR(Surface).value;
    setMinImageCount(std::max(2u, Capabilities.minImageCount));
    vk::SurfaceFormatKHR UseFormat;
    for (auto &Format : PD.getSurfaceFormatsKHR(Surface).value) {
      if (Format.format == vk::Format::eB8G8R8A8Unorm) {
        FmtOut = vk::Format::eB8G8R8A8Unorm;
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

struct MyPresentInfo : vk::PresentInfoKHR {
  vk::Semaphore Semaphore_;
  vk::SwapchainKHR Swapchain_;
  uint32_t ImageIndex_;
  MyPresentInfo(vk::Semaphore Semaphore, vk::SwapchainKHR Swapchain, uint32_t ImageIndex) noexcept
  : vk::PresentInfoKHR(1, &Semaphore_, 1, &Swapchain_, &ImageIndex_), Semaphore_(Semaphore),
    Swapchain_(Swapchain), ImageIndex_(ImageIndex) {}
};

struct MyDescriptorSetLayoutCreateInfo : vk::DescriptorSetLayoutCreateInfo {
  std::array<vk::DescriptorSetLayoutBinding, hsh::detail::MaxUniforms +
                                                 hsh::detail::MaxImages +
                                                 hsh::detail::MaxSamplers>
      Bindings;
  template <std::size_t... USeq, std::size_t... ISeq, std::size_t... SSeq>
  constexpr MyDescriptorSetLayoutCreateInfo(
      std::index_sequence<USeq...>, std::index_sequence<ISeq...>,
      std::index_sequence<SSeq...>) noexcept
      : vk::DescriptorSetLayoutCreateInfo({}, Bindings.size(), Bindings.data()),
        Bindings{vk::DescriptorSetLayoutBinding(
                     USeq, vk::DescriptorType::eUniformBufferDynamic, 1,
                     vk::ShaderStageFlagBits::eAllGraphics)...,
                 vk::DescriptorSetLayoutBinding(
                     hsh::detail::MaxUniforms + ISeq,
                     vk::DescriptorType::eSampledImage, 1,
                     vk::ShaderStageFlagBits::eAllGraphics)...,
                 vk::DescriptorSetLayoutBinding(
                     hsh::detail::MaxUniforms + hsh::detail::MaxImages + SSeq,
                     vk::DescriptorType::eSampler, 1,
                     vk::ShaderStageFlagBits::eAllGraphics)...} {}
  constexpr MyDescriptorSetLayoutCreateInfo() noexcept
      : MyDescriptorSetLayoutCreateInfo(
            std::make_index_sequence<hsh::detail::MaxUniforms>(),
            std::make_index_sequence<hsh::detail::MaxImages>(),
            std::make_index_sequence<hsh::detail::MaxSamplers>()) {}
};

struct MyPipelineLayoutCreateInfo : vk::PipelineLayoutCreateInfo {
  std::array<vk::DescriptorSetLayout, 1> Layouts;
  constexpr MyPipelineLayoutCreateInfo(vk::DescriptorSetLayout layout) noexcept
  : vk::PipelineLayoutCreateInfo({}, Layouts.size(), Layouts.data()), Layouts{layout} {}
};

struct MyRenderPassCreateInfo : vk::RenderPassCreateInfo {
  std::array<vk::AttachmentDescription, 2> Attachments;
  std::array<vk::SubpassDescription, 1> Subpasses;
  vk::AttachmentReference ColorRef{0, vk::ImageLayout::eColorAttachmentOptimal};
  vk::AttachmentReference DepthRef{1, vk::ImageLayout::eDepthAttachmentOptimal};
  constexpr MyRenderPassCreateInfo(
      vk::Format colorFormat, vk::SampleCountFlagBits colorSamples,
      vk::SampleCountFlagBits depthSamples) noexcept
      : vk::RenderPassCreateInfo({}, Attachments.size(), Attachments.data(),
                                 Subpasses.size(), Subpasses.data()),
        Attachments{
            vk::AttachmentDescription(
                {}, colorFormat, colorSamples, vk::AttachmentLoadOp::eLoad,
                vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
                vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eColorAttachmentOptimal),
            vk::AttachmentDescription(
                {}, vk::Format::eD32Sfloat, depthSamples,
                vk::AttachmentLoadOp::eLoad, vk::AttachmentStoreOp::eStore,
                vk::AttachmentLoadOp::eDontCare,
                vk::AttachmentStoreOp::eDontCare,
                vk::ImageLayout::eDepthAttachmentOptimal,
                vk::ImageLayout::eDepthAttachmentOptimal)},
        Subpasses{vk::SubpassDescription({}, vk::PipelineBindPoint::eGraphics,
                                         {}, {}, 1, &ColorRef, {}, &DepthRef)} {
  }
};

struct MyCommandPoolCreateInfo : vk::CommandPoolCreateInfo {
  constexpr MyCommandPoolCreateInfo(uint32_t qfIdx)
      : vk::CommandPoolCreateInfo(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer, qfIdx) {}
};

struct MyCommandBufferAllocateInfo : vk::CommandBufferAllocateInfo {
  constexpr MyCommandBufferAllocateInfo(vk::CommandPool cmdPool)
      : vk::CommandBufferAllocateInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 2) {}
};

struct MyCommandBufferBeginInfo : vk::CommandBufferBeginInfo {
  constexpr MyCommandBufferBeginInfo()
      : vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit) {}
};

struct MyVmaAllocatorCreateInfo : VmaAllocatorCreateInfo {
  VmaVulkanFunctions Funcs;
  MyVmaAllocatorCreateInfo(VkInstance Instance, VkPhysicalDevice PhysDev,
                           VkDevice Device, bool HasExtMemoryBudget)
      : VmaAllocatorCreateInfo{
            VmaAllocatorCreateFlagBits(
                VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT |
                (HasExtMemoryBudget ? VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT
                                    : 0)),
            PhysDev,
            Device,
            0,
            nullptr,
            nullptr,
            0,
            nullptr,
            &Funcs,
            nullptr,
            Instance,
            VK_API_VERSION_1_1} {
#define COPY_FUNC(funcName) \
    Funcs.funcName = VULKAN_HPP_DEFAULT_DISPATCHER.funcName;
    COPY_FUNC(vkGetPhysicalDeviceProperties);
    COPY_FUNC(vkGetPhysicalDeviceMemoryProperties);
    COPY_FUNC(vkAllocateMemory);
    COPY_FUNC(vkFreeMemory);
    COPY_FUNC(vkMapMemory);
    COPY_FUNC(vkUnmapMemory);
    COPY_FUNC(vkFlushMappedMemoryRanges);
    COPY_FUNC(vkInvalidateMappedMemoryRanges);
    COPY_FUNC(vkBindBufferMemory);
    COPY_FUNC(vkBindImageMemory);
    COPY_FUNC(vkGetBufferMemoryRequirements);
    COPY_FUNC(vkGetImageMemoryRequirements);
    COPY_FUNC(vkCreateBuffer);
    COPY_FUNC(vkDestroyBuffer);
    COPY_FUNC(vkCreateImage);
    COPY_FUNC(vkDestroyImage);
    COPY_FUNC(vkCmdCopyBuffer);
#if VMA_DEDICATED_ALLOCATION || VMA_VULKAN_VERSION >= 1001000
    COPY_FUNC(vkGetBufferMemoryRequirements2KHR);
    COPY_FUNC(vkGetImageMemoryRequirements2KHR);
#endif
#if VMA_BIND_MEMORY2 || VMA_VULKAN_VERSION >= 1001000
    COPY_FUNC(vkBindBufferMemory2KHR);
    COPY_FUNC(vkBindImageMemory2KHR);
#endif
#if VMA_MEMORY_BUDGET
    COPY_FUNC(vkGetPhysicalDeviceMemoryProperties2KHR);
#endif
#undef COPY_FUNC
  }
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
  hsh::detail::vulkan::Globals.Instance = Instance.get();

  auto Messenger = Instance->createDebugUtilsMessengerEXTUnique(MyDebugUtilsMessengerCreateInfo()).value;

  auto PhysDevices = Instance->enumeratePhysicalDevices().value;
  for (auto PD : PhysDevices) {
    uint32_t QFIdx = 0;
    bool HasExtMemoryBudget = false;
    auto Device = PD.createDeviceUnique(MyDeviceCreateInfo(PD, QFIdx, HasExtMemoryBudget)).value;
    VULKAN_HPP_DEFAULT_DISPATCHER.init(*Device);
    hsh::detail::vulkan::Globals.Device = Device.get();

    auto VmaAllocator =
    vk::createVmaAllocatorUnique(MyVmaAllocatorCreateInfo(Instance.get(), PD, Device.get(),
                                                          HasExtMemoryBudget)).value;
    hsh::detail::vulkan::Globals.Allocator = VmaAllocator.get();

    auto Surface = Instance->createXcbSurfaceKHR(MyXcbSurfaceCreateInfo(Window)).value;
    vk::Format ColorFormat;
    auto Swapchain = Device->createSwapchainKHRUnique(MySwapchainCreateInfo(PD, Surface, ColorFormat)).value;
    auto SwapchainImages = Device->getSwapchainImagesKHR(*Swapchain).value;
    auto DescriptorSetLayout = Device->createDescriptorSetLayoutUnique(MyDescriptorSetLayoutCreateInfo()).value;
    hsh::detail::vulkan::Globals.setDescriptorSetLayout(DescriptorSetLayout.get());
    auto PipelineLayout = Device->createPipelineLayoutUnique(MyPipelineLayoutCreateInfo(DescriptorSetLayout.get())).value;
    hsh::detail::vulkan::Globals.PipelineLayout = PipelineLayout.get();
    hsh::detail::vulkan::DescriptorPoolChain DescriptorPoolChain;
    hsh::detail::vulkan::Globals.DescriptorPoolChain = &DescriptorPoolChain;
    auto DescriptorUpdateTemplate = Device->createDescriptorUpdateTemplateUnique(hsh::detail::vulkan::DescriptorUpdateTemplateCreateInfo()).value;
    hsh::detail::vulkan::Globals.DescriptorUpdateTemplate = DescriptorUpdateTemplate.get();
    auto RenderPass = Device->createRenderPassUnique(MyRenderPassCreateInfo(ColorFormat, vk::SampleCountFlagBits::e1, vk::SampleCountFlagBits::e1)).value;
    hsh::detail::vulkan::Globals.RenderPass = RenderPass.get();
    auto Queue = Device->getQueue(QFIdx, 0);
    auto CommandPool = Device->createCommandPoolUnique(MyCommandPoolCreateInfo(QFIdx)).value;
    auto CommandBuffers = Device->allocateCommandBuffersUnique(MyCommandBufferAllocateInfo(CommandPool.get())).value;
    auto PresentCompleteSem = Device->createSemaphoreUnique({}).value;

    uint32_t Frame = 0;
    Connection.runloop([&]() {
      uint32_t Index;
      auto Res = Device->acquireNextImageKHR(*Swapchain, 500000000, *PresentCompleteSem, {}, &Index);

      uint32_t CurBufferIdx = Frame & 1u;
      vk::CommandBuffer CurBuffer = CommandBuffers[CurBufferIdx].get();
      hsh::detail::vulkan::Globals.DynamicBufferIndex = CurBufferIdx;
      hsh::detail::vulkan::Globals.DynamicBufferMask = CurBufferIdx ? ~VkDeviceSize(0) : 0;
      hsh::detail::vulkan::Globals.Cmd = CurBuffer;
      CurBuffer.begin(MyCommandBufferBeginInfo());
      CurBuffer.beginRenderPass(vk::RenderPassBeginInfo(RenderPass.get(),))

      std::cerr << "SC " << vk::to_string(Res) << " " << Index << " " << Frame++ << "\n";

      CurBuffer.endRenderPass();
      CurBuffer.end();
      Queue.presentKHR(MyPresentInfo(*PresentCompleteSem, *Swapchain, Index));
      return true;
    });

    break;
  }

  return 0;
}
