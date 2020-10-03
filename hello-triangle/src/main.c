//#define NDEBUG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef NDEBUG
void glfwError(int code, const char * message) {
    printf("Error code %i: %s", code, message);
}
#endif

int main(void) {
#ifdef NDEBUG
    fprintf(stderr, "Debug mode enabled\n");
    glfwSetErrorCallback(glfwError);
#endif

    // Initialize GLFW and create window
    glfwInit();
    if (!glfwVulkanSupported())
        return 1;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "hello-triangle", glfwGetPrimaryMonitor(), NULL);

    // Get extension info
    uint32_t extensionCount;
    const char** extensionNames;
    extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);

    // Create vkInstance
    VkInstanceCreateInfo instCreateInfo = {0};
    instCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instCreateInfo.enabledExtensionCount = extensionCount;
    instCreateInfo.ppEnabledExtensionNames = extensionNames;

#ifdef NDEBUG
    const char *validationLayers[] = {"VK_LAYER_KHRONOS_validation"};

    uint32_t availLayerCount;
    PFN_vkEnumerateInstanceLayerProperties pfnEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties) glfwGetInstanceProcAddress(NULL, "vkEnumerateInstanceLayerProperties");
    pfnEnumerateInstanceLayerProperties(&availLayerCount, NULL);
    VkLayerProperties *availableLayers = (VkLayerProperties *)malloc(availLayerCount * sizeof(VkLayerProperties));
    pfnEnumerateInstanceLayerProperties(&availLayerCount, availableLayers);

    for (uint32_t i = 0; i < availLayerCount; ++i)
        if (!strcmp(validationLayers[0], availableLayers[i].layerName)) {
            fprintf(stderr, "Validation layer found\n");
            break;
        }

    instCreateInfo.enabledLayerCount = 1;
    instCreateInfo.ppEnabledLayerNames = validationLayers;
#endif

    VkInstance vkInst;
    PFN_vkCreateInstance pfnCreateInstance = (PFN_vkCreateInstance) glfwGetInstanceProcAddress(NULL, "vkCreateInstance");
    if (pfnCreateInstance(&instCreateInfo, NULL, &vkInst))
        return 2;

    // Create surface
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(vkInst, window, NULL, &surface))
        return 3;

    // Get phyiscal devices
    uint32_t physDeviceCount;
    PFN_vkEnumeratePhysicalDevices pfnEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices) glfwGetInstanceProcAddress(NULL, "vkEnumeratePhysicalDevices");
    if (pfnEnumeratePhysicalDevices(vkInst, &physDeviceCount, NULL) || physDeviceCount < 1)
        return 4;
    VkPhysicalDevice *physDevices = (VkPhysicalDevice *)malloc(physDeviceCount * sizeof(VkPhysicalDevice));
    pfnEnumeratePhysicalDevices(vkInst, &physDeviceCount, physDevices);

    // Select physical device with most VRAM, probably the best device
    VkPhysicalDevice physicalDevice; // If physicalDevice is undefined, search failed
    VkDeviceSize maxVram = 0;
    VkPhysicalDeviceMemoryProperties deviceMemProps;
    PFN_vkGetPhysicalDeviceMemoryProperties pfnGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties) glfwGetInstanceProcAddress(NULL, "vkGetPhysicalDeviceMemoryProperties");
    for (uint32_t i = 0; i < physDeviceCount; ++i) {
        pfnGetPhysicalDeviceMemoryProperties(physDevices[i], &deviceMemProps);
        for (uint32_t heapIndex = 0; heapIndex < deviceMemProps.memoryHeapCount; ++heapIndex)
            if (deviceMemProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                if (deviceMemProps.memoryHeaps[heapIndex].size > maxVram) {
                    maxVram = deviceMemProps.memoryHeaps[heapIndex].size;
                    physicalDevice = physDevices[i];
                }
                break;
            }
    }

    // Get physical device features
    VkPhysicalDeviceFeatures phyDevFeats;
    PFN_vkGetPhysicalDeviceFeatures pfnGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures) glfwGetInstanceProcAddress(NULL, "vkGetPhysicalDeviceFeatures");
    pfnGetPhysicalDeviceFeatures(physicalDevice, &phyDevFeats);

    // Get queue families
    uint32_t queueFamilyCount;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties pfnGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties) glfwGetInstanceProcAddress(NULL, "vkGetPhysicalDeviceQueueFamilyProperties");
    pfnGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties *queueFamilies = (VkQueueFamilyProperties *)malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    pfnGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    // Select queue family
    uint32_t queueGfxPrsntFamilyIndex = 0;
    VkBool32 presentSupport;
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR pfnGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR) glfwGetInstanceProcAddress(NULL, "vkGetPhysicalDeviceSurfaceSupportKHR");
    for (; queueGfxPrsntFamilyIndex < queueFamilyCount; ++queueGfxPrsntFamilyIndex) {
        pfnGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueGfxPrsntFamilyIndex, surface, &presentSupport);
        if ((queueFamilies[queueGfxPrsntFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
            break;
    }

    // Define device queue create info
    VkDeviceQueueCreateInfo devQueCreateInfo = {0};
    devQueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    devQueCreateInfo.queueFamilyIndex = queueGfxPrsntFamilyIndex;
    devQueCreateInfo.queueCount = 1;
    float devQuePrio = 1.0f;
    devQueCreateInfo.pQueuePriorities = &devQuePrio;

    // Define device extensions we plan on using
    const char* lDevExts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // Define logical device create info
    VkDeviceCreateInfo lDevCreateInfo = {0};
    lDevCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    lDevCreateInfo.pEnabledFeatures = &phyDevFeats;
    lDevCreateInfo.pQueueCreateInfos = &devQueCreateInfo;
    lDevCreateInfo.queueCreateInfoCount = 1;
    lDevCreateInfo.ppEnabledExtensionNames = lDevExts;
    lDevCreateInfo.enabledExtensionCount = sizeof(lDevExts) / sizeof(char*);

    // Create logical device
    VkDevice logicalDevice;
    PFN_vkCreateDevice pfnCreateDevice = (PFN_vkCreateDevice) glfwGetInstanceProcAddress(NULL, "vkCreateDevice");
    if (pfnCreateDevice(physicalDevice, &lDevCreateInfo, NULL, &logicalDevice))
        return 5;

    // Get graphics queue
    VkQueue gfxPrsntQueue;
    PFN_vkGetDeviceQueue pfnGetDeviceQueue = (PFN_vkGetDeviceQueue) glfwGetInstanceProcAddress(NULL, "vkGetDeviceQueue");
    pfnGetDeviceQueue(logicalDevice, queueGfxPrsntFamilyIndex, 0, &gfxPrsntQueue);

    // Get surface capabilities and extent
    VkSurfaceCapabilitiesKHR capabilities;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR pfnGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR) glfwGetInstanceProcAddress(NULL, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    pfnGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
    VkExtent2D surfaceExtent = capabilities.maxImageExtent;

    // Get surface formats
    uint32_t surfaceFormatCount;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR pfnGetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR) glfwGetInstanceProcAddress(NULL, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    pfnGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, NULL);
    VkSurfaceFormatKHR *surfaceFormats = (VkSurfaceFormatKHR *)malloc(surfaceFormatCount * sizeof(VkSurfaceFormatKHR));
    pfnGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats);

    // Select surface format
    // TODO: Match VK_FORMAT_B8G8R8A8_SRGB in the future, using the lightest format (usually in ascending order) is only for performance
    VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];

    // Define swapchain create info
    VkSwapchainCreateInfoKHR swpChnCreateinfo = {0};
    swpChnCreateinfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swpChnCreateinfo.surface = surface;
    swpChnCreateinfo.minImageCount = capabilities.minImageCount == capabilities.maxImageCount ? capabilities.minImageCount : capabilities.minImageCount + 1;
    swpChnCreateinfo.imageExtent = surfaceExtent;
    swpChnCreateinfo.imageFormat = surfaceFormat.format;
    swpChnCreateinfo.imageColorSpace = surfaceFormat.colorSpace;
    swpChnCreateinfo.imageArrayLayers = 1;
    swpChnCreateinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swpChnCreateinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swpChnCreateinfo.preTransform = capabilities.currentTransform;
    swpChnCreateinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swpChnCreateinfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    swpChnCreateinfo.clipped = VK_TRUE;
    swpChnCreateinfo.oldSwapchain = VK_NULL_HANDLE;

    // Create swap chain
    VkSwapchainKHR swapChain;
    PFN_vkCreateSwapchainKHR pfnCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR) glfwGetInstanceProcAddress(NULL, "vkCreateSwapchainKHR");
    if (pfnCreateSwapchainKHR(logicalDevice, &swpChnCreateinfo, NULL, &swapChain))
        return 6;

    // Get swap chain images
    uint32_t swapChainImageCount;
    PFN_vkGetSwapchainImagesKHR pfnGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR) glfwGetInstanceProcAddress(NULL, "vkGetSwapchainImagesKHR");
    pfnGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, NULL);
    VkImage *swapChainImages = (VkImage *)malloc(swapChainImageCount * sizeof(VkImage));
    pfnGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, swapChainImages);

    // Define image view create
    VkImageViewCreateInfo imageViewCreateinfo = {0};
    imageViewCreateinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateinfo.format = surfaceFormat.format;
    imageViewCreateinfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateinfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateinfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateinfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreateinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateinfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateinfo.subresourceRange.levelCount = 1;
    imageViewCreateinfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateinfo.subresourceRange.layerCount = 1;

    // Create image views
    VkImageView swapChainImageViews[swapChainImageCount];
    PFN_vkCreateImageView pfnCreateImageView = (PFN_vkCreateImageView) glfwGetInstanceProcAddress(NULL, "vkCreateImageView");
    for (uint32_t i = 0; i < swapChainImageCount; ++i) {
        imageViewCreateinfo.image = swapChainImages[i];
        if (pfnCreateImageView(logicalDevice, &imageViewCreateinfo, NULL, &swapChainImageViews[i]))
            return 7;
    }

    // Define color attachment
    VkAttachmentDescription colorAttachment = {0};
    colorAttachment.format = surfaceFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Define color attachment references
    VkAttachmentReference colorAttachmentRef = {0};
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Define subpass
    VkSubpassDescription subpass = {0};
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Define subpass dependency
    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        
    // Create render pass
    VkRenderPassCreateInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkRenderPass renderPass;
    PFN_vkCreateRenderPass pfnCreateRenderPass = (PFN_vkCreateRenderPass) glfwGetInstanceProcAddress(NULL, "vkCreateRenderPass");
    if (pfnCreateRenderPass(logicalDevice, &renderPassInfo, NULL, &renderPass))
        return 8;

    // Define common shader create info
    VkShaderModuleCreateInfo shaderCreateInfo = {0};
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    PFN_vkCreateShaderModule pfnCreateShaderModule = (PFN_vkCreateShaderModule) glfwGetInstanceProcAddress(NULL, "vkCreateShaderModule");

    // Load vertex shader
    FILE *shaderFilePtr = fopen("shader.vert.spv", "rb");
    if (!shaderFilePtr)
        return 9;
    fseek(shaderFilePtr, 0, SEEK_END);
    shaderCreateInfo.codeSize = ftell(shaderFilePtr);
    rewind(shaderFilePtr);
    shaderCreateInfo.pCode = malloc(shaderCreateInfo.codeSize * sizeof(char));
    fread((void *)shaderCreateInfo.pCode, shaderCreateInfo.codeSize, 1, shaderFilePtr);
    fclose(shaderFilePtr);
    VkShaderModule vertShaderMod;
    if (pfnCreateShaderModule(logicalDevice, &shaderCreateInfo, NULL, &vertShaderMod))
        return 10;

    // Load fragment shader
    shaderFilePtr = fopen("shader.frag.spv", "rb");
    if (!shaderFilePtr)
        return 11;
    fseek(shaderFilePtr, 0, SEEK_END);
    shaderCreateInfo.codeSize = ftell(shaderFilePtr);
    rewind(shaderFilePtr);
    shaderCreateInfo.pCode = malloc(shaderCreateInfo.codeSize * sizeof(char));
    fread((void *)shaderCreateInfo.pCode, shaderCreateInfo.codeSize, 1, shaderFilePtr);
    fclose(shaderFilePtr);
    VkShaderModule fragShaderMod;
    if (pfnCreateShaderModule(logicalDevice, &shaderCreateInfo, NULL, &fragShaderMod))
        return 12;

    // Define vertex stage create info
    VkPipelineShaderStageCreateInfo vertStageCreateInfo = {0};
    vertStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageCreateInfo.module = vertShaderMod;
    vertStageCreateInfo.pName = "main";

    // Define frag stage create info
    VkPipelineShaderStageCreateInfo fragStageCreateInfo = {0};
    fragStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageCreateInfo.module = fragShaderMod;
    fragStageCreateInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageCreateInfo, fragStageCreateInfo};

    // Define vertex input info
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // Define input assembly info
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Define viewport
    VkViewport viewport = {0};
    viewport.width = (float) surfaceExtent.width;
    viewport.height = (float) surfaceExtent.height;
    viewport.maxDepth = 1.0f;

    // Define scissor
    VkRect2D scissor = {0};
    scissor.extent = surfaceExtent;

    // Define viewport state
    VkPipelineViewportStateCreateInfo viewportState = {0};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // Define rasterization state
    VkPipelineRasterizationStateCreateInfo rasterizer = {0};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;

    // Define multisampling, required when rasterizing
    VkPipelineMultisampleStateCreateInfo multisampling = {0};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Define color blending attachment, required when rasterizing
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    // Define color blending
    VkPipelineColorBlendStateCreateInfo colorBlending = {0};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Define pipeline info
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    // Create pipeline layout
    VkPipelineLayout pipelineLayout;
    PFN_vkCreatePipelineLayout pfnCreatePipelineLayout = (PFN_vkCreatePipelineLayout) glfwGetInstanceProcAddress(NULL, "vkCreatePipelineLayout");
    if (pfnCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, NULL, &pipelineLayout))
        return 13;

    // Define pipeline info
    VkGraphicsPipelineCreateInfo pipelineInfo = {0};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;

    // Create pipeline
    VkPipeline graphicsPipeline;
    PFN_vkCreateGraphicsPipelines pfnCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines) glfwGetInstanceProcAddress(NULL, "vkCreateGraphicsPipelines");
    if (pfnCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline))
        return 14;

    // Define generic framebuffer info
    VkFramebufferCreateInfo framebufferInfo = {0};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = surfaceExtent.width;
    framebufferInfo.height = surfaceExtent.height;
    framebufferInfo.layers = 1;

    // Assign each image to a framebuffer
    VkFramebuffer *swapChainFramebuffers = malloc(swapChainImageCount * sizeof(swapChainFramebuffers));
    PFN_vkCreateFramebuffer pfnCreateFramebuffer = (PFN_vkCreateFramebuffer) glfwGetInstanceProcAddress(NULL, "vkCreateFramebuffer");
    for (uint32_t i = 0; i < swapChainImageCount; ++i) {
        VkImageView attachments[] = { swapChainImageViews[i] };
        framebufferInfo.pAttachments = attachments;

        if (pfnCreateFramebuffer(logicalDevice, &framebufferInfo, NULL, &swapChainFramebuffers[i]))
            return 15;
    }

    // Define command pool info
    VkCommandPoolCreateInfo poolInfo = {0};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueGfxPrsntFamilyIndex;

    // Create command pool
    VkCommandPool commandPool;
    PFN_vkCreateCommandPool pfnCreateCommandPool = (PFN_vkCreateCommandPool) glfwGetInstanceProcAddress(NULL, "vkCreateCommandPool");
    if (pfnCreateCommandPool(logicalDevice, &poolInfo, NULL, &commandPool))
        return 16;

    // Define command buffer allocate
    VkCommandBufferAllocateInfo allocInfo = {0};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = swapChainImageCount;

    // Create command buffer allocate
    VkCommandBuffer *commandBuffers = malloc(swapChainImageCount * sizeof(VkCommandBuffer));
    PFN_vkAllocateCommandBuffers pfnAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers) glfwGetInstanceProcAddress(NULL, "vkAllocateCommandBuffers");
    if (pfnAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers))
        return 17;
    
    // Define command buffer begin
    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    PFN_vkBeginCommandBuffer pfnBeginCommandBuffer = (PFN_vkBeginCommandBuffer) glfwGetInstanceProcAddress(NULL, "vkBeginCommandBuffer");

    // Define render pass info
    VkRenderPassBeginInfo renderPassBeginInfo = {0};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea.extent = surfaceExtent;
    VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;

    PFN_vkCmdBeginRenderPass pfnCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass) glfwGetInstanceProcAddress(NULL, "vkCmdBeginRenderPass");
    PFN_vkCmdBindPipeline pfnCmdBindPipeline = (PFN_vkCmdBindPipeline) glfwGetInstanceProcAddress(NULL, "vkCmdBindPipeline");
    PFN_vkCmdDraw pfnCmdDraw = (PFN_vkCmdDraw) glfwGetInstanceProcAddress(NULL, "vkCmdDraw");
    PFN_vkCmdEndRenderPass pfnCmdEndRenderPass = (PFN_vkCmdEndRenderPass) glfwGetInstanceProcAddress(NULL, "vkCmdEndRenderPass");
    PFN_vkEndCommandBuffer pfnEndCommandBuffer = (PFN_vkEndCommandBuffer) glfwGetInstanceProcAddress(NULL, "vkEndCommandBuffer");
    for (uint32_t i = 0; i < swapChainImageCount; ++i) {
        // Create command buffer begin info
        if (pfnBeginCommandBuffer(commandBuffers[i], &beginInfo))
            return 18;

        // Define render pass info
        renderPassBeginInfo.framebuffer = swapChainFramebuffers[i];

        // Bind command buffers to render pass
        pfnCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        pfnCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        pfnCmdDraw(commandBuffers[i], 3, 1, 0, 0);
        pfnCmdEndRenderPass(commandBuffers[i]);

        // End render pass
        if (pfnEndCommandBuffer(commandBuffers[i]))
            return 19;
    }

    // Define semaphore info
    VkSemaphoreCreateInfo semaphoreInfo = {0};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Create semaphores
    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    PFN_vkCreateSemaphore pfnCreateSemaphore = (PFN_vkCreateSemaphore) glfwGetInstanceProcAddress(NULL, "vkCreateSemaphore");
    if (pfnCreateSemaphore(logicalDevice, &semaphoreInfo, NULL, &imageAvailableSemaphore) ||
        pfnCreateSemaphore(logicalDevice, &semaphoreInfo, NULL, &renderFinishedSemaphore))
        return 20;

    // Define reusable submit info
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // Define reusable present info
    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    // Setup frame counter, output every 10000 frames
    uint frameCount = 10000;
    clock_t oldClock = clock();
    clock_t nowClock = 0;

    // Get main loop function pointers
    uint32_t imageIndex;
    PFN_vkAcquireNextImageKHR pfnAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR) glfwGetInstanceProcAddress(NULL, "vkAcquireNextImageKHR");
    PFN_vkQueueSubmit pfnQueueSubmit = (PFN_vkQueueSubmit) glfwGetInstanceProcAddress(NULL, "vkQueueSubmit");
    PFN_vkQueuePresentKHR pfnQueuePresentKHR = (PFN_vkQueuePresentKHR) glfwGetInstanceProcAddress(NULL, "vkQueuePresentKHR");
    PFN_vkQueueWaitIdle pfnQueueWaitIdle = (PFN_vkQueueWaitIdle) glfwGetInstanceProcAddress(NULL, "vkQueueWaitIdle");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Get inputs from the user
        glfwPollEvents();

        // Get image and update image index
        pfnAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
        presentInfo.pImageIndices = &imageIndex;

        // Submit and present
        if (pfnQueueSubmit(gfxPrsntQueue, 1, &submitInfo, VK_NULL_HANDLE))
            return 21;
        pfnQueuePresentKHR(gfxPrsntQueue, &presentInfo);

        // Output framerate every 10000 frames
        if (--frameCount == 0) {
            nowClock = clock();
            fprintf(stderr, "%.2fkfps\n", 10.f * CLOCKS_PER_SEC / (nowClock - oldClock));
            oldClock = nowClock;
            frameCount = 10000;
        }

        // Wait for work to finish before restarting the main loop
        pfnQueueWaitIdle(gfxPrsntQueue);
    }

    // Wait for execution to finish before cleaning up
    PFN_vkDeviceWaitIdle pfnDeviceWaitIdle = (PFN_vkDeviceWaitIdle) glfwGetInstanceProcAddress(NULL, "vkDeviceWaitIdle");
    pfnDeviceWaitIdle(logicalDevice);

    // Cleanup
    PFN_vkDestroySemaphore pfnDestroySemaphore = (PFN_vkDestroySemaphore) glfwGetInstanceProcAddress(NULL, "vkDestroySemaphore");
    pfnDestroySemaphore(logicalDevice, imageAvailableSemaphore, NULL);
    pfnDestroySemaphore(logicalDevice, renderFinishedSemaphore, NULL);
    PFN_vkDestroyCommandPool pfnDestroyCommandPool = (PFN_vkDestroyCommandPool) glfwGetInstanceProcAddress(NULL, "vkDestroyCommandPool");
    pfnDestroyCommandPool(logicalDevice, commandPool, NULL);
    PFN_vkDestroyFramebuffer pfnDestroyFramebuffer = (PFN_vkDestroyFramebuffer) glfwGetInstanceProcAddress(NULL, "vkDestroyFramebuffer");
    for(uint32_t i = 0; i < swapChainImageCount; ++i)
        pfnDestroyFramebuffer(logicalDevice, swapChainFramebuffers[i], NULL);
    PFN_vkDestroyPipeline pfnDestroyPipeline = (PFN_vkDestroyPipeline) glfwGetInstanceProcAddress(NULL, "vkDestroyPipeline");
    pfnDestroyPipeline(logicalDevice, graphicsPipeline, NULL);
    PFN_vkDestroyPipelineLayout pfnDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout) glfwGetInstanceProcAddress(NULL, "vkDestroyPipelineLayout");
    pfnDestroyPipelineLayout(logicalDevice, pipelineLayout, NULL);
    PFN_vkDestroyRenderPass pfnDestroyRenderPass = (PFN_vkDestroyRenderPass) glfwGetInstanceProcAddress(NULL, "vkDestroyRenderPass");
    pfnDestroyRenderPass(logicalDevice, renderPass, NULL);
    PFN_vkDestroyShaderModule pfnDestroyShaderModule = (PFN_vkDestroyShaderModule) glfwGetInstanceProcAddress(NULL, "vkDestroyShaderModule");
    pfnDestroyShaderModule(logicalDevice, fragShaderMod, NULL);
    pfnDestroyShaderModule(logicalDevice, vertShaderMod, NULL);
    PFN_vkDestroyImageView pfnDestroyImageView = (PFN_vkDestroyImageView) glfwGetInstanceProcAddress(NULL, "vkDestroyImageView");
    for(uint32_t i = 0; i < swapChainImageCount; ++i)
        pfnDestroyImageView(logicalDevice, swapChainImageViews[i], NULL);
    PFN_vkDestroySwapchainKHR pfnDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR) glfwGetInstanceProcAddress(NULL, "vkDestroySwapchainKHR");
    pfnDestroySwapchainKHR(logicalDevice, swapChain, NULL);
    PFN_vkDestroyDevice pfnDestroyDevice = (PFN_vkDestroyDevice) glfwGetInstanceProcAddress(NULL, "vkDestroyDevice");
    pfnDestroyDevice(logicalDevice, NULL);
    PFN_vkDestroySurfaceKHR pfnDestroySurface = (PFN_vkDestroySurfaceKHR) glfwGetInstanceProcAddress(NULL, "vkDestroySurfaceKHR");
    pfnDestroySurface(vkInst, surface, NULL);
    PFN_vkDestroyInstance pfnDestroyInstance = (PFN_vkDestroyInstance) glfwGetInstanceProcAddress(NULL, "vkDestroyInstance");
    pfnDestroyInstance(vkInst, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}