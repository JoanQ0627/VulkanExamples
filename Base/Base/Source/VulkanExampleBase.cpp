// 1. �����ϰ��յ���˳����д
// 2. �Ǻ�����Ⱦ�����ݣ����������룬cmdline���룬profile�����ڴ����ȵȣ��ܿ����ڸ�ʲô���У�����ֱ�Ӹ���ճ��
// 3. �� *** �ı�ʾ���漰������Ⱦ�ģ���Ҫ���������ص㺯��
//vulkanExample = new VulkanExample();
//vulkanExample->initVulkan();
//vulkanExample->setupWindow(hInstance, WndProc);
//vulkanExample->prepare()
			//createSurface();
			//createCommandPool();
			//createSwapChain();
			//createCommandBuffers();
			//createSynchronizationPrimitives();
			//setupDepthStencil();
			//setupRenderPass();
			//createPipelineCache();
			//setupFrameBuffer();
			// UIOverlay
//vulkanExample->renderLoop();
			// nextFrame -> render		
			// prepareFrame
			// �����Լ��ģ�buildCommandBuffer, updateUniform��
			// submitFrame
//delete(vulkanExample);

#include "VulkanExampleBase.h"

static std::vector<const char*> args;

VulkanExampleBase::VulkanExampleBase()
{
	// Command line arguments
	commandLineParser.add("help", { "--help" }, 0, "Show help");
	commandLineParser.add("validation", { "-v", "--validation" }, 0, "Enable validation layers");
	commandLineParser.add("validationlogfile", { "-vl", "--validationlogfile" }, 0, "Log validation messages to a textfile");
	commandLineParser.add("vsync", { "-vs", "--vsync" }, 0, "Enable V-Sync");
	commandLineParser.add("fullscreen", { "-f", "--fullscreen" }, 0, "Start in fullscreen mode");
	commandLineParser.add("width", { "-w", "--width" }, 1, "Set window width");
	commandLineParser.add("height", { "-h", "--height" }, 1, "Set window height");
	commandLineParser.add("shaders", { "-s", "--shaders" }, 1, "Select shader type to use (gls, hlsl or slang)");
	commandLineParser.add("gpuselection", { "-g", "--gpu" }, 1, "Select GPU to run on");
	commandLineParser.add("gpulist", { "-gl", "--listgpus" }, 0, "Display a list of available Vulkan devices");
	commandLineParser.add("benchmark", { "-b", "--benchmark" }, 0, "Run example in benchmark mode");
	commandLineParser.add("benchmarkwarmup", { "-bw", "--benchwarmup" }, 1, "Set warmup time for benchmark mode in seconds");
	commandLineParser.add("benchmarkruntime", { "-br", "--benchruntime" }, 1, "Set duration time for benchmark mode in seconds");
	commandLineParser.add("benchmarkresultfile", { "-bf", "--benchfilename" }, 1, "Set file name for benchmark results");
	commandLineParser.add("benchmarkresultframes", { "-bt", "--benchframetimes" }, 0, "Save frame times to benchmark results file");
	commandLineParser.add("benchmarkframes", { "-bfs", "--benchmarkframes" }, 1, "Only render the given number of frames");
#if (!(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT)))
	commandLineParser.add("resourcepath", { "-rp", "--resourcepath" }, 1, "Set path for dir where assets and shaders folder is present");
#endif
	commandLineParser.parse(args);
	if (commandLineParser.isSet("help")) {
#if defined(_WIN32)
		setupConsole("Vulkan example");
#endif
		commandLineParser.printHelp();
		std::cin.get();
		exit(0);
	}
	if (commandLineParser.isSet("validation")) {
		settings.validation = true;
	}
	if (commandLineParser.isSet("validationlogfile")) {
		vks::debug::logToFile = true;
	}
	if (commandLineParser.isSet("vsync")) {
		settings.vsync = true;
	}
	if (commandLineParser.isSet("height")) {
		height = commandLineParser.getValueAsInt("height", height);
	}
	if (commandLineParser.isSet("width")) {
		width = commandLineParser.getValueAsInt("width", width);
	}
	if (commandLineParser.isSet("fullscreen")) {
		settings.fullscreen = true;
	}
	if (commandLineParser.isSet("shaders")) {
		std::string value = commandLineParser.getValueAsString("shaders", "glsl");
		if ((value != "glsl") && (value != "hlsl") && (value != "slang")) {
			std::cerr << "Shader type must be one of 'glsl', 'hlsl' or 'slang'\n";
		}
		else {
			shaderDir = value;
		}
	}
	if (commandLineParser.isSet("benchmark")) {
		benchmark.active = true;
		vks::tools::errorModeSilent = true;
	}
	if (commandLineParser.isSet("benchmarkwarmup")) {
		benchmark.warmup = commandLineParser.getValueAsInt("benchmarkwarmup", 0);
	}
	if (commandLineParser.isSet("benchmarkruntime")) {
		benchmark.duration = commandLineParser.getValueAsInt("benchmarkruntime", benchmark.duration);
	}
	if (commandLineParser.isSet("benchmarkresultfile")) {
		benchmark.filename = commandLineParser.getValueAsString("benchmarkresultfile", benchmark.filename);
	}
	if (commandLineParser.isSet("benchmarkresultframes")) {
		benchmark.outputFrameTimes = true;
	}
	if (commandLineParser.isSet("benchmarkframes")) {
		benchmark.outputFrames = commandLineParser.getValueAsInt("benchmarkframes", benchmark.outputFrames);
	}
#if (!(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT)))
	if (commandLineParser.isSet("resourcepath")) {
		vks::tools::resourcePath = commandLineParser.getValueAsString("resourcepath", "");
	}
#else
	// On Apple platforms, use layer settings extension to configure MoltenVK with common project config settings
	enabledInstanceExtensions.push_back(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);

	// Configure MoltenVK to use to use a dedicated compute queue (see compute[*] and timelinesemaphore samples)
	VkLayerSettingEXT layerSetting;
	layerSetting.pLayerName = "MoltenVK";
	layerSetting.pSettingName = "MVK_CONFIG_SPECIALIZED_QUEUE_FAMILIES";
	layerSetting.type = VK_LAYER_SETTING_TYPE_BOOL32_EXT;
	layerSetting.valueCount = 1;

	// Make this static so layer setting reference remains valid after leaving constructor scope
	static const VkBool32 layerSettingOn = VK_TRUE;
	layerSetting.pValues = &layerSettingOn;
	enabledLayerSettings.push_back(layerSetting);
#endif

#if !defined(VK_USE_PLATFORM_ANDROID_KHR)
	// Check for a valid asset path
	struct stat info;
	if (stat(getAssetPath().c_str(), &info) != 0)
	{
#if defined(_WIN32)
		std::string msg = "Could not locate asset path in \"" + getAssetPath() + "\" !";
		MessageBox(NULL, msg.c_str(), "Fatal error", MB_OK | MB_ICONERROR);
#else
		std::cerr << "Error: Could not find asset path in " << getAssetPath() << "\n";
#endif
		exit(-1);
	}
#endif

	// Validation for all samples can be forced at compile time using the FORCE_VALIDATION define
#if defined(FORCE_VALIDATION)
	settings.validation = true;
#endif

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	// Vulkan library is loaded dynamically on Android
	bool libLoaded = vks::android::loadVulkanLibrary();
	assert(libLoaded);
#elif defined(_DIRECT2DISPLAY)

#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
	initWaylandConnection();
#elif defined(VK_USE_PLATFORM_XCB_KHR)
	initxcbConnection();
#endif
}

#if defined(_WIN32)
// Win32 : Sets up a console window and redirects standard output to it
void VulkanExampleBase::setupConsole(std::string title)
{
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	FILE* stream;
	freopen_s(&stream, "CONIN$", "r", stdin);
	freopen_s(&stream, "CONOUT$", "w+", stdout);
	freopen_s(&stream, "CONOUT$", "w+", stderr);
	// Enable flags so we can color the output
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode = 0;
	GetConsoleMode(consoleHandle, &dwMode);
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(consoleHandle, dwMode);
	SetConsoleTitle(TEXT(title.c_str()));
}

void VulkanExampleBase::setupDPIAwareness()
{
	typedef HRESULT* (__stdcall* SetProcessDpiAwarenessFunc)(PROCESS_DPI_AWARENESS);

	HMODULE shCore = LoadLibraryA("Shcore.dll");
	if (shCore)
	{
		SetProcessDpiAwarenessFunc setProcessDpiAwareness =
			(SetProcessDpiAwarenessFunc)GetProcAddress(shCore, "SetProcessDpiAwareness");

		if (setProcessDpiAwareness != nullptr)
		{
			setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
		}

		FreeLibrary(shCore);
	}
}

HWND VulkanExampleBase::setupWindow(HINSTANCE hinstance, WNDPROC wndproc)
{
	this->windowInstance = hinstance;

	WNDCLASSEX wndClass{};

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = wndproc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hinstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = name.c_str();
	wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	if (!RegisterClassEx(&wndClass))
	{
		std::cout << "Could not register window class!\n";
		fflush(stdout);
		exit(1);
	}

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	if (settings.fullscreen)
	{
		if ((width != (uint32_t)screenWidth) && (height != (uint32_t)screenHeight))
		{
			DEVMODE dmScreenSettings;
			memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
			dmScreenSettings.dmSize = sizeof(dmScreenSettings);
			dmScreenSettings.dmPelsWidth = width;
			dmScreenSettings.dmPelsHeight = height;
			dmScreenSettings.dmBitsPerPel = 32;
			dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
			if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			{
				if (MessageBox(NULL, "Fullscreen Mode not supported!\n Switch to window mode?", "Error", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
				{
					settings.fullscreen = false;
				}
				else
				{
					return nullptr;
				}
			}
			screenWidth = width;
			screenHeight = height;
		}

	}

	DWORD dwExStyle;
	DWORD dwStyle;

	if (settings.fullscreen)
	{
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}

	RECT windowRect = {
		0L,
		0L,
		settings.fullscreen ? (long)screenWidth : (long)width,
		settings.fullscreen ? (long)screenHeight : (long)height
	};

	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

	std::string windowTitle = getWindowTitle();
	window = CreateWindowEx(0,
		name.c_str(),
		windowTitle.c_str(),
		dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0,
		0,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		hinstance,
		NULL);

	if (!window)
	{
		std::cerr << "Could not create window!\n";
		fflush(stdout);
		return nullptr;
	}

	if (!settings.fullscreen)
	{
		// Center on screen
		uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
		uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
		SetWindowPos(window, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}

	ShowWindow(window, SW_SHOW);
	SetForegroundWindow(window);
	SetFocus(window);

	return window;
}

void VulkanExampleBase::handleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		prepared = false;
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		ValidateRect(window, NULL);
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case KEY_P:
			paused = !paused;
			break;
		case KEY_F1:
			ui.visible = !ui.visible;
			break;
		case KEY_F2:
			if (camera.type == Camera::CameraType::lookat) {
				camera.type = Camera::CameraType::firstperson;
			}
			else {
				camera.type = Camera::CameraType::lookat;
			}
			break;
		case KEY_ESCAPE:
			PostQuitMessage(0);
			break;
		}

		if (camera.type == Camera::firstperson)
		{
			switch (wParam)
			{
			case KEY_W:
				camera.keys.up = true;
				break;
			case KEY_S:
				camera.keys.down = true;
				break;
			case KEY_A:
				camera.keys.left = true;
				break;
			case KEY_D:
				camera.keys.right = true;
				break;
			}
		}

		keyPressed((uint32_t)wParam);
		break;
	case WM_KEYUP:
		if (camera.type == Camera::firstperson)
		{
			switch (wParam)
			{
			case KEY_W:
				camera.keys.up = false;
				break;
			case KEY_S:
				camera.keys.down = false;
				break;
			case KEY_A:
				camera.keys.left = false;
				break;
			case KEY_D:
				camera.keys.right = false;
				break;
			}
		}
		break;
	case WM_LBUTTONDOWN:
		mouseState.position = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		mouseState.buttons.left = true;
		break;
	case WM_RBUTTONDOWN:
		mouseState.position = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		mouseState.buttons.right = true;
		break;
	case WM_MBUTTONDOWN:
		mouseState.position = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		mouseState.buttons.middle = true;
		break;
	case WM_LBUTTONUP:
		mouseState.buttons.left = false;
		break;
	case WM_RBUTTONUP:
		mouseState.buttons.right = false;
		break;
	case WM_MBUTTONUP:
		mouseState.buttons.middle = false;
		break;
	case WM_MOUSEWHEEL:
	{
		short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		camera.translate(glm::vec3(0.0f, 0.0f, (float)wheelDelta * 0.005f));
		break;
	}
	case WM_MOUSEMOVE:
	{
		handleMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;
	}
	case WM_SIZE:
		if ((prepared) && (wParam != SIZE_MINIMIZED))
		{
			if ((resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)))
			{
				destWidth = LOWORD(lParam);
				destHeight = HIWORD(lParam);
				windowResize();
			}
		}
		break;
	case WM_GETMINMAXINFO:
	{
		LPMINMAXINFO minMaxInfo = (LPMINMAXINFO)lParam;
		minMaxInfo->ptMinTrackSize.x = 64;
		minMaxInfo->ptMinTrackSize.y = 64;
		break;
	}
	case WM_ENTERSIZEMOVE:
		resizing = true;
		break;
	case WM_EXITSIZEMOVE:
		resizing = false;
		break;
	}

	OnHandleMessage(hWnd, uMsg, wParam, lParam);
}

void VulkanExampleBase::OnHandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {};

#endif

std::string VulkanExampleBase::getWindowTitle() const
{
	std::string windowTitle{ title + " - " + deviceProperties.deviceName };
	if (!settings.overlay) {
		windowTitle += " - " + std::to_string(frameCounter) + " fps";
	}
	return windowTitle;
}

void VulkanExampleBase::keyPressed(uint32_t) {} // ����ʵ�֣����������֮���������

void VulkanExampleBase::mouseMoved(double x, double y, bool& handled) {} // ����ʵ�֣��Լ�������ƶ���Ӧ

void VulkanExampleBase::handleMouseMove(int32_t x, int32_t y)
{
	int32_t dx = (int32_t)mouseState.position.x - x;
	int32_t dy = (int32_t)mouseState.position.y - y;

	bool handled = false;

	if (settings.overlay) {
		ImGuiIO& io = ImGui::GetIO();
		handled = io.WantCaptureMouse && ui.visible;
	}
	mouseMoved((float)x, (float)y, handled);

	if (handled) {
		mouseState.position = glm::vec2((float)x, (float)y);
		return;
	}

	if (mouseState.buttons.left) {
		camera.rotate(glm::vec3(dy * camera.rotationSpeed, -dx * camera.rotationSpeed, 0.0f));
	}
	if (mouseState.buttons.right) {
		camera.translate(glm::vec3(-0.0f, 0.0f, dy * .005f));
	}
	if (mouseState.buttons.middle) {
		camera.translate(glm::vec3(-dx * 0.005f, -dy * 0.005f, 0.0f));
	}
	mouseState.position = glm::vec2((float)x, (float)y);
}

void VulkanExampleBase::windowResized() {}

std::string VulkanExampleBase::getShadersPath() const
{
	return getShaderBasePath() + shaderDir + "/";
}

/// <summary>
/// *** ���ڱ仯ʱӦ�����Ĳ���
/// </summary>
void VulkanExampleBase::windowResize()
{
	if (!prepared)
	{
		return;
	}

	prepared = false;
	resized = true;

	// - �ȵȴ�GPU��֮ǰ�Ĺ�������
	vkDeviceWaitIdle(device);

	// - swapchain�����ٴ���
	width = destWidth;
	height = destHeight;
	// ��������ھͰ����˾ɵ����٣��µĴ���
	createSwapChain();

	// - depthStencil��frameBuffer�����ٴ���
	vkDestroyImage(device, depthStencil.image, nullptr);
	vkDestroyImageView(device, depthStencil.view, nullptr);
	vkFreeMemory(device, depthStencil.memory, nullptr);
	setupDepthStencil();
	for (auto& framebBuffer : frameBuffers)
	{
		vkDestroyFramebuffer(device, framebBuffer, nullptr);
	}
	setupFrameBuffer();


	// - ͬ��ԭ������ٴ���
	for (auto& semaphore : presentCompleteSemaphores)
	{
		vkDestroySemaphore(device, semaphore, nullptr);
	}

	for (auto& semaphore : renderCompleteSemaphores)
	{
		vkDestroySemaphore(device, semaphore, nullptr);
	}

	for (auto& fence : waitFences)
	{
		vkDestroyFence(device, fence, nullptr);
	}

	createSynchronizationPrimitives();

	// - ui resize
	if ((width > 0.0f) && (height > 0.0f)) {
		if (settings.overlay) {
			ui.resize(width, height);
		}
	}

	// - ��һ�εȴ�GPU����������Ϊʲô�ȴ�������ΪҪ�ȴ������ָ��ִ�������
	// ������Щ������cmdѽ��Ӧ����ͬ����ѽ��Ӧ������ͽ�������
	// ��D��ʦ�ķ�������������ȷʵ�Ƕ���ģ������������֤��ȥ��֮��ֱ�Ӹ������Ӧ�ò������κ�Ӱ��
	// todo
	vkDeviceWaitIdle(device);

	// - ���������߱ȣ�Ӱ��ͶӰ����

	// ����ȥ������
	windowResized();

	resized = true;
}


/// <summary>
/// *** initVulkan
/// </summary>
/// <returns></returns>
bool VulkanExampleBase::initVulkan()
{
	// �������Լ�����֤�������������initvulkan����
	// Ϊ���ڲ������������в�����ʱ�򣬲������캯���Ӳ������ҹ��캯����������Щ�߼�Ҳ��̫��
#if defined(_WIN32)
	// - Enable console if validation is active, debug message callback will output to it
	if (this->settings.validation)
	{
		setupConsole("Vulkan example");
	}
	setupDPIAwareness();
#endif

	// - д����֤��log�ļ�
	if (commandLineParser.isSet("validationlogfile")) {
		vks::debug::log("Sample: " + title);
	}

	// - Create the instance
	VkResult result = createInstance();
	if (result != VK_SUCCESS) {
		vks::tools::exitFatal("Could not create Vulkan instance : \n" + vks::tools::errorString(result), result);
		return false;
	}

	// - ׼��һ����֤����չ
	if (settings.validation)
	{
		vks::debug::setupDebugging(instance);
	}

	// - ������pick physicalDevice�ı�׼����
	uint32_t gpuCount = 0;
	// Get number of available physical devices
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
	if (gpuCount == 0) {
		vks::tools::exitFatal("No device with Vulkan support found", -1);
		return false;
	}
	// Enumerate devices
	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
	result = vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());
	if (result != VK_SUCCESS) {
		vks::tools::exitFatal("Could not enumerate physical devices : \n" + vks::tools::errorString(result), result);
		return false;
	}

	// cmdlineû�л���ûѡ������Ĭ��ѡ��һ��
	uint32_t selectedDevice = 0;

	// ͨ��cmdline ѡ����gpu
	if (commandLineParser.isSet("gpuselection")) {
		uint32_t index = commandLineParser.getValueAsInt("gpuselection", 0);
		if (index > gpuCount - 1) {
			std::cerr << "Selected device index " << index << " is out of range, reverting to device 0 (use -listgpus to show available Vulkan devices)" << "\n";
		}
		else {
			selectedDevice = index;
		}
	}
	if (commandLineParser.isSet("gpulist")) {
		std::cout << "Available Vulkan devices" << "\n";
		for (uint32_t i = 0; i < gpuCount; i++) {
			VkPhysicalDeviceProperties deviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
			std::cout << "Device [" << i << "] : " << deviceProperties.deviceName << std::endl;
			std::cout << " Type: " << vks::tools::physicalDeviceTypeString(deviceProperties.deviceType) << "\n";
			std::cout << " API: " << (deviceProperties.apiVersion >> 22) << "." << ((deviceProperties.apiVersion >> 12) & 0x3ff) << "." << (deviceProperties.apiVersion & 0xfff) << "\n";
		}
	}

	physicalDevice = physicalDevices[selectedDevice];

	// - ��ȡphysicalDevice�����Ϣ Properties�� Features�� MemoryProperties
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

	// - �����߼��豸 vkDevice
	// ����ʵ�֣�ͨ��ȥdeviceFeatures�����û����Ҫ��features
	// Ȼ��ȥ�ı�enabledFeatures������Ȼ������ȥ���� device
	getEnabledFeatures();

	// ��һ����û�д���device,ֻ�ǰ�physicalDevice�����װ�࣬������
	// ��ȡphysicalDevice�����Ϣ Properties�� Features�� MemoryProperties, Extension, FamilyQueue�Ļ�ȡ������
	vulkanDevice = new vks::VulkanDevice(physicalDevice);

	// �����������һ��
	getEnabledExtensions();

	// �����������߼��豸
	result = vulkanDevice->createLogicalDevice(enabledFeatures, enabledDeviceExtensions, deviceCreatepNextChain);
	if (result != VK_SUCCESS) {
		vks::tools::exitFatal("Could not create Vulkan device: \n" + vks::tools::errorString(result), result);
		return false;
	}
	device = vulkanDevice->logicalDevice;

	// - ��ѯ��û�к��ʵ����/ģ���ʽ
	// ����ֻ�ǲ�ѯ + ��ֵ
	VkBool32 validFormat{ false };
	if (requiresStencil)
	{
		validFormat = vks::tools::getSupportedDepthStencilFormat(physicalDevice, &depthFormat);
	}
	else
	{
		validFormat = vks::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);
	}
	assert(validFormat);


	// - swapChain��װ���ʼ��
	swapChain.setContext(instance, physicalDevice, device);

	return true;
}

/// <summary>
/// *** createVkInstance
/// </summary>
/// <returns></returns>
VkResult VulkanExampleBase::createInstance()
{
	// - �ȴ���instance�����extension���
	// instanceExtensions ������������Ҫ����createinfo��
	std::vector<const char*> instanceExtensions{ VK_KHR_SURFACE_EXTENSION_NAME };
#ifdef _WIN32
	instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	// ���һ��Instance֧�ֵ� supportedInstanceExtensions
	uint32_t extCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
	if (extCount > 0)
	{
		std::vector<VkExtensionProperties> extensions(extCount);
		if (vkEnumerateInstanceExtensionProperties(nullptr, &extCount, extensions.data()) == VK_SUCCESS)
		{
			for (auto& ext : extensions)
			{
				supportedInstanceExtensions.push_back(ext.extensionName);
			}
		}
	}

	// ���һ����û����Щ��չ������������Ҫ�����ģ���enabledInstanceExtensions���Ƿ��ж���
	// ���enabledInstanceExtensions�����ʱ������Ĵ���֮�󣬵���InitVulkan֮ǰ
	// ��ʵ��������Ż��ģ�����������������־ͺ�������⣬enabled��ʾ�Ѿ������ģ�����ʵ�������ⲿ��Ҫ�����ģ��ܲ��ܿ�����Ҫ������
	// �������������ʱ�������ٰ�װ��
	if (!enabledInstanceExtensions.empty())
	{
		for (const char* ext : enabledInstanceExtensions)
		{
			if (std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), ext) == supportedInstanceExtensions.end())
			{
				std::cerr << "Enabled instance extension \"" << ext << "\" is not present at instance level\n";
			}

			// ������֪����֧�ֻ��ӽ�ȥ��ʲô���� ��todo
			instanceExtensions.push_back(ext);
		}
	}

	// - ������֤�㼰EXT_debug_utils��չ
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = name.c_str();
	appInfo.pEngineName = name.c_str();
	appInfo.apiVersion = apiVersion;

	VkInstanceCreateInfo instanceCreateInfo{};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	// ������֤�������������������, ��һ������Ը����, �ڶ�����EXT��չ֧��
	bool isDebugUtilsEXTSupport = std::find(supportedInstanceExtensions.begin(), supportedInstanceExtensions.end(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
		!= supportedInstanceExtensions.end();
	if (settings.validation && isDebugUtilsEXTSupport)
	{
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{};
		vks::debug::setupDebugingMessengerCreateInfo(debugUtilsMessengerCI);
		debugUtilsMessengerCI.pNext = instanceCreateInfo.pNext;
		instanceCreateInfo.pNext = &debugUtilsMessengerCI;

		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		
		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
		// Check if this layer is available at instance level
		uint32_t instanceLayerCount;
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
		std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
		bool validationLayerPresent = false;
		for (VkLayerProperties& layer : instanceLayerProperties) {
			if (strcmp(layer.layerName, validationLayerName) == 0) {
				validationLayerPresent = true;
				break;
			}
		}
		if (validationLayerPresent) {
			instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
			instanceCreateInfo.enabledLayerCount = 1;
		}
		else {
			std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
		}
	}

	if (!instanceExtensions.empty())
	{
		instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
	}

	// - createInstance
	VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

	// - debugEXT��ʼ��
	if (settings.validation && isDebugUtilsEXTSupport)
	{
		vks::debugutils::setup(instance);
	}
}

/// <summary>
/// *** prepare
/// </summary>
void  VulkanExampleBase::prepare()
{
	// ���������ϸ���˳���,ǰ��������
	createSurface();
	createCommandPool();
	createSwapChain();
	createCommandBuffers();
	createSynchronizationPrimitives();
	setupDepthStencil();
	setupRenderPass();
	createPipelineCache();
	setupFrameBuffer();

	// ����UI����ȾȷʵҲ��Ҫ�о�һ��,����������
	settings.overlay = settings.overlay && (!benchmark.active);
	if (settings.overlay) {
		ui.maxConcurrentFrames = c_maxConcurrentFrames;
		ui.device = vulkanDevice;
		ui.queue = graphicsQueue;
		ui.shaders = {
			loadShader(getShadersPath() + "base/uioverlay.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(getShadersPath() + "base/uioverlay.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
		};
		ui.prepareResources();
		ui.preparePipeline(pipelineCache, renderPass, swapChain.colorFormat, depthFormat);
	}
}

/// <summary>
/// *** createSurface
/// </summary>
void  VulkanExampleBase::createSurface()
{
	// ȫ��������װ��ȥ���ͺ�,ʵ���������һЩ��������
	// �����ڴ��������б����˶��� ��ʽ����Ϣ,���Ժ���ò�ѯ
#if defined(_WIN32)
	swapChain.initSurface(windowInstance, window);
#endif
}

/// <summary>
/// *** createCommandPool
/// </summary>
void VulkanExampleBase::createCommandPool()
{
	VkCommandPoolCreateInfo cmdPoolInfo = vks::initializers::commandPoolCreateInfo();
	cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex; // ��һ��createSurface�õ���
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandPool));
}

/// <summary>
/// *** createSwapChain
/// </summary>
void VulkanExampleBase::createSwapChain()
{
	// һ��Ҳ�ǽ�����װ��ȥ����
	// ������û�к��ر������,����һ��swapchain�Ĵ�������,����������ڰ�����oldswapchain������
	// �Ѿ�������swapchain �� image �� imageview 
	swapChain.create(width, height, settings.vsync, settings.fullscreen);
}

/// <summary>
/// *** createCommandBuffers
/// </summary>
void VulkanExampleBase::createCommandBuffers()
{
	// ƽƽ����
	VkCommandBufferAllocateInfo cmdBufAllocateInfo = 
		vks::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, static_cast<uint32_t>(drawCmdBuffers.size()));
	VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, drawCmdBuffers.data()));
}

/// <summary>
/// *** createSynchronizationPrimitives
/// </summary>
void VulkanExampleBase::createSynchronizationPrimitives()
{
	// �и�����,Ϊʲôfence��present�ĸ����Ǹ���flight�����ߵ�
	// ��renderComplete�Ǹ���swapChainImageCount�ߵ�?
	// 
	// Wait fences to sync command buffer access
	VkFenceCreateInfo fenceCreateInfo = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	for (auto& fence : waitFences) {
		VK_CHECK_RESULT(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
	}

	// Used to ensure that image presentation is complete before starting to submit again
	for (auto& semaphore : presentCompleteSemaphores) {
		VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
	}

	// Semaphore used to ensure that all commands submitted have been finished before submitting the image to the queue
	renderCompleteSemaphores.resize(swapChain.images.size());
	for (auto& semaphore : renderCompleteSemaphores) {
		VkSemaphoreCreateInfo semaphoreCI{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &semaphore));
	}
}

/// <summary>
/// *** setupDepthStencil
/// ��ʵ�����������̶��ڴ������� image imageview �ǻ��������
/// </summary>
void VulkanExampleBase::setupDepthStencil()
{
	// - image
	VkImageCreateInfo imageCI = vks::initializers::imageCreateInfo();
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = depthFormat;
	imageCI.extent = { width, height, 1 }; // �����õ�Ϊʲô���,��������,���ȷʵ�ǻ�ʵʱ���µ�,������swapchain���Ǹ�����?
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &depthStencil.image));

	// - allocate mem + bind mem
	VkMemoryRequirements memRequirements{};
	vkGetImageMemoryRequirements(device, depthStencil.image, &memRequirements);

	VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
	memAlloc.allocationSize = memRequirements.size;
	memAlloc.memoryTypeIndex = vulkanDevice->getMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &depthStencil.memory));
	VK_CHECK_RESULT(vkBindImageMemory(device, depthStencil.image, depthStencil.memory, 0));

	// - imageview
	VkImageViewCreateInfo imageViewCI = vks::initializers::imageViewCreateInfo();
	imageViewCI.image = depthStencil.image;
	imageViewCI.format = depthFormat;
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.subresourceRange.levelCount = 1;
	imageViewCI.subresourceRange.layerCount = 1;
	imageViewCI.subresourceRange.baseMipLevel = 0;
	imageViewCI.subresourceRange.baseArrayLayer = 0;
	imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (requiresStencil)
	{
		imageViewCI.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCI, nullptr, &depthStencil.view));
}

/// <summary>
/// ******** setupRenderPass 
/// ����RT
/// Ҫ�úõ�������renderpass��setup����
/// </summary>
void VulkanExampleBase::setupRenderPass()
{
	// һ������¾��� color �� depth����RT
	// ���Ի����϶��õĻ���,
	// �����Ҫ���rendpass,�����ڻ����prepare��,�ٴ����Լ���
	std::array<VkAttachmentDescription, 2> attachments{};

	// - ����������VkAttachmentDescription��
	// ������������Ⱦ������ʹ�õ�ͼ����Դ����ɫ����ȡ�ģ�壩����Ϊ��״̬ת����
	// color attachments
	attachments[0].format = swapChain.colorFormat;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// depth attachments
	attachments[1].format = depthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// - �������ã�VkAttachmentReference��
	// �������ý������󶨵������̵��ض�λ�ã���ָ��ʹ��ʱ��ͼ�񲼾֡�
	// color attachments ref
	VkAttachmentReference colorReference{};
	colorReference.attachment = 0; // color attachment index
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// depth attachments ref
	VkAttachmentReference depthReference{};
	depthReference.attachment = 1; // depth attachment index
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // ����Ϊʲô����Ҫ�ж��ǲ�����Ҫstencil��?

	// - ������������VkSubpassDescription��
	// �����̶�������Ⱦ������һ���׶Σ�ָ�������ʹ�ø�����
	// û��������ָ�������ʹ�ø���������
	// Ŷ��Ӧ�þ��� pColorAttachments = &colorReference  pDepthStencilAttachment = &depthReference
	// ֮ǰ�������Լ������������color��depth���������˼ҹ涨��
	// subpassDescription
	VkSubpassDescription subpassDescription{};
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pDepthStencilAttachment = &depthReference;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;

	// - ������������VkSubpassDependency��
	// ���������������˲�ͬ�����̼�����ⲿ�������ͬ����ϵ
	// ע���ص���������֮�䡢�ⲿ�����䡢 ͬ����ϵ
	// ����srcSubpass dstSubpass���������subpassDescription������
	// ��ָ��Ӧ���ǣ��� srcSubpass -> dstSubpass������Ҫ�������ͬ����ϵ
	// ����������ʽ�ء���vulkan������mem barrier
	// VkSubpassDependency �����еġ�ͬ���������������ڿ���GPU�в�ͬ����֮���ִ��˳����ڴ�ɼ���
	// ���ǰ������������������⣺
	// ִ������ : ����ͨ�� srcStageMask �� dstStageMask ���Ƶ�		��λA�����������ȫ�����꣬��λB���ܿ�ʼ�����Ļ
	// �ڴ����� : ����ͨ�� srcAccessMask �� dstAccessMask ���Ƶ�	��λA�����󣬱���Ѳ�Ʒ�ӡ�����̨�������棩��ʽ�ᵽ���ֿ⡱�����棩����֪ͨ��λB��������λB���ܴӲֿ��õ������ʡ���ȷ�Ĳ�Ʒ
	// subpassDependencies
	std::array<VkSubpassDependency, 2> subpassDependencies{};

	// color subpass dependency
	// depth
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL; // ������Ⱦ�����ⲿ // �� srcSubpass = VK_SUBPASS_EXTERNAL ʱ��������˼�ǣ���ȴ����RenderPass֮���������ز������
	subpassDependencies[0].dstSubpass = 0;					// �����ǵ�������

	// ���壺��ȴ����С�����/����Ƭ�β��ԡ��׶εĲ�����ɡ�
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

	// ���壺�ڽ������ġ�����/����Ƭ�β��ԡ��׶ο�ʼǰ������������������...
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

	// ����1��ȷ��֮ǰ���ж���ȸ����ġ�д�롱�����Ѿ���ɡ�
	subpassDependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// ����2��ȷ��֮�����ȸ����ġ���ȡ��д�롱�������õ����µ����ݡ�

	// �����й�����Ȳ��ԵĻ��������ͣһͣ����ǰ����˰����ǡ�д��ȡ��Ļ�����׸��ꡢ���ѳɹ��Ǽ���⣨�ڴ�ɼ���֮�����ǲ��ܿ�ʼ�����ǡ�����д��ȡ��Ļ������
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	subpassDependencies[0].dependencyFlags = 0;

	// color
	subpassDependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].dstSubpass = 0;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].srcAccessMask = 0;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	subpassDependencies[1].dependencyFlags = 0;

	// renderPassCreateInfo
	VkRenderPassCreateInfo renderPassCreateInfo = vks::initializers::renderPassCreateInfo();
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassCreateInfo.pAttachments = attachments.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass));
}

/// <summary>
/// ��������
/// </summary>
void VulkanExampleBase::createPipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = vks::initializers::pipelineCacheCreateInfo();
	VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
}

/// <summary>
/// *** setupFrameBuffer
/// </summary>
void VulkanExampleBase::setupFrameBuffer()
{
	frameBuffers.resize(swapChain.imageCount);
	for (size_t i = 0; i < frameBuffers.size(); i++)
	{
		const VkImageView attachments[2] = {swapChain.imageViews[i], depthStencil.view};

		VkFramebufferCreateInfo frameBufferCreateInfo = vks::initializers::framebufferCreateInfo();
		frameBufferCreateInfo.attachmentCount = 2;
		frameBufferCreateInfo.pAttachments = attachments;
		frameBufferCreateInfo.renderPass = renderPass;
		frameBufferCreateInfo.width = width;
		frameBufferCreateInfo.height = height;
		frameBufferCreateInfo.layers = 1;
		VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
	}
}

/// <summary>
/// *** renderLoop
/// </summary>
void VulkanExampleBase::renderLoop()
{
	// benchmark��ص����Թ�
	destWidth = width;
	destHeight = height;
	lastTimestamp = std::chrono::high_resolution_clock::now();
	tPrevEnd = lastTimestamp;
#if defined(_WIN32)
	MSG msg;
	bool quitMessageReceived = false;
	while (!quitMessageReceived) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				quitMessageReceived = true;
				break;
			}
		}
		if (prepared && !IsIconic(window)) {
			nextFrame();
		}
	}
#endif
}

/// <summary>
/// *** nextFrame
/// </summary>
void VulkanExampleBase::nextFrame()
{
	// �ƺ��Ƚ���Ҫ����fps��صĴ�����Ⱦ��ص�ûʲô
	// û̫���������FPS���㣬�������ɣ��ȿ���Ⱦ��ص�

	auto tStart = std::chrono::high_resolution_clock::now();
	render();
	frameCounter++;
	auto tEnd = std::chrono::high_resolution_clock::now();
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK) || defined(VK_USE_PLATFORM_METAL_EXT)) && !defined(VK_EXAMPLE_XCODE_GENERATED)
	// SRS - Calculate tDiff as time between frames vs. rendering time for iOS/macOS displayLink-driven examples project
	auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tPrevEnd).count();
#else
	auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
#endif
	frameTimer = (float)tDiff / 1000.0f; /// Ϊʲô�����delta time��
	camera.update(frameTimer);
	// Convert to clamped timer value
	if (!paused)
	{
		timer += timerSpeed * frameTimer;
		if (timer > 1.0)
		{
			timer -= 1.0f;
		}
	}
	float fpsTimer = (float)(std::chrono::duration<double, std::milli>(tEnd - lastTimestamp).count());
	if (fpsTimer > 1000.0f)
	{
		lastFPS = static_cast<uint32_t>((float)frameCounter * (1000.0f / fpsTimer));
#if defined(_WIN32)
		if (!settings.overlay) {
			std::string windowTitle = getWindowTitle();
			SetWindowText(window, windowTitle.c_str());
		}
#endif
		frameCounter = 0;
		lastTimestamp = tEnd;
	}
	tPrevEnd = tEnd;
}

/// <summary>
/// *** prepareFrame
/// </summary>
/// <param name="waitForFence"></param>
void  VulkanExampleBase::prepareFrame(bool waitForFence)
{
	if (waitForFence) {
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &waitFences[currentBuffer], VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(device, 1, &waitFences[currentBuffer]));
	}
	updateOverlay();
	VkResult result = swapChain.acquireNextImage(presentCompleteSemaphores[currentBuffer], currentImageIndex);
	if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) 
	{
		if (result == VK_ERROR_OUT_OF_DATE_KHR) // ����ֵĴ��룬������ֻ��Ϊ�˴���VK_ERROR_OUT_OF_DATE_KHR
		{
			windowResize();
		}
		return;
	}
	else 
	{
		VK_CHECK_RESULT(result);
	}
}

/// <summary>
/// *** ui��ص��Ȳ���
/// </summary>
void VulkanExampleBase::updateOverlay()
{
	if (!settings.overlay)
		return;

	ImGuiIO& io = ImGui::GetIO();

	io.DisplaySize = ImVec2((float)width, (float)height);
	io.DeltaTime = frameTimer;

	io.MousePos = ImVec2(mouseState.position.x, mouseState.position.y);
	io.MouseDown[0] = mouseState.buttons.left && ui.visible;
	io.MouseDown[1] = mouseState.buttons.right && ui.visible;
	io.MouseDown[2] = mouseState.buttons.middle && ui.visible;

	ImGui::NewFrame();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::SetNextWindowPos(ImVec2(10 * ui.scale, 10 * ui.scale));
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Vulkan Example", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::TextUnformatted(title.c_str());
	ImGui::TextUnformatted(deviceProperties.deviceName);
	ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / lastFPS), lastFPS);

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 5.0f * ui.scale));
#endif
	ImGui::PushItemWidth(110.0f * ui.scale);
	OnUpdateUIOverlay(&ui);
	ImGui::PopItemWidth();
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	ImGui::PopStyleVar();
#endif

	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::Render();

	ui.update(currentBuffer);

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
	if (mouseState.buttons.left) {
		mouseState.buttons.left = false;
	}
#endif
}

/// <summary>
/// *** ui��ص��Ȳ���
/// </summary>
void VulkanExampleBase::drawUI(const VkCommandBuffer commandBuffer)
{
	if (settings.overlay && ui.visible) {
		const VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		const VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		ui.draw(commandBuffer, currentBuffer);
	}
}

/// <summary>
/// load����õ�shader
/// ���� shaderModule �� shaderStageCreateInfo
/// </summary>
/// <param name="fileName"></param>
/// <param name="stage"></param>
/// <returns></returns>
VkPipelineShaderStageCreateInfo VulkanExampleBase::loadShader(std::string fileName, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStageInfo = vks::initializers::pipelineShaderStageCreateInfo();
	shaderStageInfo.stage = stage;
	shaderStageInfo.module = vks::tools::loadShader(fileName.c_str(), device);
	shaderStageInfo.pName = "main";
	assert(shaderStageInfo.module != VK_NULL_HANDLE);
	shaderModules.push_back(shaderStageInfo.module);
	return shaderStageInfo;
}

/// <summary>
/// *** submitFrame
/// </summary>
/// <param name="waitForFence"></param>
void  VulkanExampleBase::submitFrame(bool skipQueueSubmit)
{
	if (!skipQueueSubmit)
	{
		VkPipelineStageFlags waitPipelineStage{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSubmitInfo submitInfo = vks::initializers::submitInfo();
		submitInfo.pWaitDstStageMask = &waitPipelineStage;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &presentCompleteSemaphores[currentBuffer];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &renderCompleteSemaphores[currentImageIndex];
		VK_CHECK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, waitFences[currentBuffer]));
	}

	VkPresentInfoKHR presentInfo{ .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderCompleteSemaphores[currentImageIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain.swapChain;
	presentInfo.pImageIndices = &currentImageIndex;
	VkResult result = vkQueuePresentKHR(graphicsQueue, &presentInfo);
	// Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
	if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
		windowResize();
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			return;
		}
	}
	else {
		VK_CHECK_RESULT(result);
	}
	// Select the next frame to render to, based on the max. no. of concurrent frames
	currentBuffer = (currentBuffer + 1) % c_maxConcurrentFrames;
}