#ifndef MAIN_GLFW_H__
#define MAIN_GLFW_H__

#include "App.h"
#include "client/renderer/entity/PlayerRenderer.h"
#include "client/renderer/gles.h"
#include "GLFW/glfw3.h"

#include <cstdio>
#include <chrono>
#include <thread>
#include <csignal>
#include "platform/input/Keyboard.h"
#include "platform/input/Mouse.h"
#include "platform/input/Multitouch.h"
#include "AppPlatform_glfw.h"

static App* g_app = 0;

static void platformSignalHandler(int sig) {
	if (g_app) {
		g_app->quit();
	}
}

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

extern "C" {
EMSCRIPTEN_KEEPALIVE void web_feed_char(int codepoint) {
	if (codepoint < 128) {
		Keyboard::feedText((char)codepoint);
	} else if (codepoint >= 0x400 && codepoint <= 0x45F) {
		Keyboard::feedText((char)(0xC0 | (codepoint >> 6)));
		Keyboard::feedText((char)(0x80 | (codepoint & 0x3F)));
	}
}

EMSCRIPTEN_KEEPALIVE void web_feed_key(int key, int action) {
	Keyboard::feed(key, action);
}
}

static long touchSlotMap[Multitouch::MAX_POINTERS] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

static int getTouchSlot(long id, bool allocate) {
	for (int i = 0; i < Multitouch::MAX_POINTERS; ++i) {
		if (touchSlotMap[i] == id) return i;
	}
	if (!allocate) return -1;
	for (int i = 0; i < Multitouch::MAX_POINTERS; ++i) {
		if (touchSlotMap[i] == -1) {
			touchSlotMap[i] = id;
			return i;
		}
	}
	return -1;
}

static EM_BOOL emscripten_touch_callback(int eventType, const EmscriptenTouchEvent *e, void *userData) {
	if (!e) return EM_FALSE;

	GLFWwindow* win = NULL;
	if (g_app && g_app->platform()) {
		AppPlatform_glfw* plt = (AppPlatform_glfw*)g_app->platform();
		win = plt->window;
	}

	int winW = 0, winH = 0, fbW = 0, fbH = 0;
	if (win) {
		glfwGetWindowSize(win, &winW, &winH);
		glfwGetFramebufferSize(win, &fbW, &fbH);
	}
	float scaleX = (winW > 0) ? ((float)fbW / winW) : 1.0f;
	float scaleY = (winH > 0) ? ((float)fbH / winH) : 1.0f;

	for (int i = 0; i < e->numTouches; ++i) {
		const EmscriptenTouchPoint& tp = e->touches[i];
		if (!tp.isChanged && eventType != EMSCRIPTEN_EVENT_TOUCHSTART) continue;

		short x = (short)(tp.targetX * scaleX);
		short y = (short)(tp.targetY * scaleY);

		if (eventType == EMSCRIPTEN_EVENT_TOUCHSTART) {
			int slot = getTouchSlot(tp.identifier, true);
			if (slot != -1) {
				Multitouch::feed(1, MouseAction::DATA_DOWN, x, y, slot);
				if (slot == 0) {
					Mouse::feed(MouseAction::ACTION_LEFT, 1, x, y);
				}
			}
		} else if (eventType == EMSCRIPTEN_EVENT_TOUCHMOVE) {
			int slot = getTouchSlot(tp.identifier, false);
			if (slot != -1) {
				Multitouch::feed(0, 0, x, y, slot);
				if (slot == 0) {
					Mouse::feed(MouseAction::ACTION_MOVE, 0, x, y);
				}
			}
		} else if (eventType == EMSCRIPTEN_EVENT_TOUCHEND || eventType == EMSCRIPTEN_EVENT_TOUCHCANCEL) {
			int slot = getTouchSlot(tp.identifier, false);
			if (slot != -1) {
				Multitouch::feed(1, MouseAction::DATA_UP, x, y, slot);
				if (slot == 0) {
					Mouse::feed(MouseAction::ACTION_LEFT, 0, x, y);
				}
				touchSlotMap[slot] = -1;
			}
		}
	}
	return EM_TRUE;
}
#endif

int transformKey(int glfwkey) {
	if (glfwkey >= GLFW_KEY_F1 && glfwkey <= GLFW_KEY_F12) {
		return glfwkey - 178;
	}

	switch (glfwkey) {
		case GLFW_KEY_ESCAPE: return Keyboard::KEY_ESCAPE;
		case GLFW_KEY_TAB: return Keyboard::KEY_TAB;
		case GLFW_KEY_BACKSPACE: return Keyboard::KEY_BACKSPACE;
		case GLFW_KEY_LEFT_SHIFT: return Keyboard::KEY_LSHIFT;
		case GLFW_KEY_ENTER: return Keyboard::KEY_RETURN;
		case GLFW_KEY_LEFT_CONTROL: return Keyboard::KEY_LEFT_CTRL;
		case GLFW_KEY_RIGHT_CONTROL: return Keyboard::KEY_LEFT_CTRL;
		default: return glfwkey;
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if(action == GLFW_REPEAT) return;

	if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
		GLFWmonitor* monitor = glfwGetWindowMonitor(window);
		if (monitor) {
			// Currently fullscreen → go windowed
			glfwSetWindowMonitor(window, NULL, 80, 80, 854, 480, 0);
		} else {
			// Currently windowed → go fullscreen on primary monitor
			GLFWmonitor* primary = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(primary);
			glfwSetWindowMonitor(window, primary, 0, 0, mode->width, mode->height, mode->refreshRate);
		}
		return;
	}

	Keyboard::feed(transformKey(key), action);
}

void character_callback(GLFWwindow* window, unsigned int codepoint) {
	// Text channel carries raw UTF-8 bytes; TextBox assembles them.
	// Only what the game font renders: ASCII plus Cyrillic U+0400-U+045F.
	if (codepoint < 128) {
		Keyboard::feedText((char)codepoint);
	} else if (codepoint >= 0x400 && codepoint <= 0x45F) {
		Keyboard::feedText((char)(0xC0 | (codepoint >> 6)));
		Keyboard::feedText((char)(0x80 | (codepoint & 0x3F)));
	}
}

static void scaleCursorToFramebuffer(GLFWwindow* window, double& xpos, double& ypos) {
	int winW = 0, winH = 0, fbW = 0, fbH = 0;
	glfwGetWindowSize(window, &winW, &winH);
	glfwGetFramebufferSize(window, &fbW, &fbH);
	if (winW > 0 && winH > 0 && (winW != fbW || winH != fbH)) {
		xpos *= ((double)fbW / winW);
		ypos *= ((double)fbH / winH);
	}
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	scaleCursorToFramebuffer(window, xpos, ypos);
	static double lastX = 0.0, lastY = 0.0;
	static bool firstMouse = true;

	if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

	double deltaX = xpos - lastX;
    double deltaY = ypos - lastY;

    lastX = xpos;
    lastY = ypos;

	if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
		Mouse::feed(0, 0, xpos, ypos, deltaX, deltaY);
	} else { 
		Mouse::feed( MouseAction::ACTION_MOVE, 0, xpos, ypos);
	}
	Multitouch::feed(0, 0, xpos, ypos, 0);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if(action == GLFW_REPEAT) return;

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	scaleCursorToFramebuffer(window, xpos, ypos);

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		Mouse::feed( MouseAction::ACTION_LEFT, action, xpos, ypos);
		Multitouch::feed(1, action, xpos, ypos, 0);
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		Mouse::feed( MouseAction::ACTION_RIGHT, action, xpos, ypos);
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	scaleCursorToFramebuffer(window, xpos, ypos);

	Mouse::feed(3, 0, xpos, ypos, 0, yoffset);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	if (g_app && width > 0 && height > 0) g_app->setSize(width, height);
}

void window_size_callback(GLFWwindow* window, int width, int height) {
	int fbW = width, fbH = height;
	glfwGetFramebufferSize(window, &fbW, &fbH);
	if (g_app && fbW > 0 && fbH > 0) g_app->setSize(fbW, fbH);
}

void error_callback(int error, const char* desc) {
	printf("Error: %s\n", desc);
}


void loop() {
	using clock = std::chrono::steady_clock;
	auto frameStart = clock::now();

	g_app->update();

	glfwSwapBuffers(((AppPlatform_glfw*)g_app->platform())->window);
	glfwPollEvents();

#ifdef __EMSCRIPTEN__
	static bool firstFrameReported = false;
	if (!firstFrameReported) {
		firstFrameReported = true;
		EM_ASM({
			try {
				if (typeof window !== 'undefined' && window.onFirstGameFrame) {
					window.onFirstGameFrame();
				}
			} catch(e) {}
		});
	}
#endif

	glfwSwapInterval(((MAIN_CLASS*)g_app)->options.getBooleanValue(OPTIONS_VSYNC) ? 1 : 0);
#ifndef __EMSCRIPTEN__
	int perfLimit = ((MAIN_CLASS*)g_app)->options.getIntValue(OPTIONS_LIMIT_FRAMERATE);
	if (perfLimit == 1) { // Balanced (60 FPS)
		auto frameEnd = clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);
		auto target = std::chrono::microseconds(16666);
		if (elapsed < target)
			std::this_thread::sleep_for(target - elapsed);
	} else if (perfLimit == 2) { // Power saver (30 FPS)
		auto frameEnd = clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);
		auto target = std::chrono::microseconds(33333);
		if (elapsed < target)
			std::this_thread::sleep_for(target - elapsed);
	}
#endif
}

int main(void) {
	AppContext appContext;
	memset(&appContext, 0, sizeof(appContext));
	appContext.doRender = true;

#ifndef STANDALONE_SERVER
	// Platform init.
	appContext.platform = new AppPlatform_glfw();
#if defined(__EMSCRIPTEN__)
	EM_ASM({
		try {
			try { FS.mkdir('/games'); } catch(e) {}
			try { FS.mount(IDBFS, {}, '/games'); } catch(e) {}
			try { FS.syncfs(true, function (err) {}); } catch(e) {}
			try { FS.mkdir('/games/com.mojang'); } catch(e) {}
			try { FS.mkdir('/games/com.mojang/minecraftWorlds'); } catch(e) {}
		} catch(e) {
			console.warn('IDBFS mount warning:', e);
		}
	});
#endif

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
#ifndef __EMSCRIPTEN__
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

	AppPlatform_glfw* platform = (AppPlatform_glfw*)appContext.platform;

	int initW = appContext.platform->getScreenWidth();
	int initH = appContext.platform->getScreenHeight();
	if (initW <= 0) initW = 854;
	if (initH <= 0) initH = 480;
	platform->window = glfwCreateWindow(initW, initH, "Minecraft PE 0.6.1", NULL, NULL);
	
	if (platform->window == NULL) {
		return 1;
	}

	glfwSetKeyCallback(platform->window, key_callback);
	glfwSetCharCallback(platform->window, character_callback);
	glfwSetCursorPosCallback(platform->window, cursor_position_callback);
	glfwSetMouseButtonCallback(platform->window, mouse_button_callback);
	glfwSetScrollCallback(platform->window, scroll_callback);
	glfwSetWindowSizeCallback(platform->window, window_size_callback);
	glfwSetFramebufferSizeCallback(platform->window, framebuffer_size_callback);
#ifdef __EMSCRIPTEN__
	emscripten_set_touchstart_callback("#canvas", 0, EM_TRUE, emscripten_touch_callback);
	emscripten_set_touchmove_callback("#canvas", 0, EM_TRUE, emscripten_touch_callback);
	emscripten_set_touchend_callback("#canvas", 0, EM_TRUE, emscripten_touch_callback);
	emscripten_set_touchcancel_callback("#canvas", 0, EM_TRUE, emscripten_touch_callback);
#endif

	glfwMakeContextCurrent(platform->window);
	#ifndef __EMSCRIPTEN__
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glfwSwapInterval(0);
	#endif
#endif

	App* app = new MAIN_CLASS();

	g_app = app;
#ifdef __EMSCRIPTEN__
	((MAIN_CLASS*)g_app)->externalStoragePath = "/games/com.mojang";
	((MAIN_CLASS*)g_app)->externalCacheStoragePath = "/games/com.mojang";
#else
	((MAIN_CLASS*)g_app)->externalStoragePath = ".";
	((MAIN_CLASS*)g_app)->externalCacheStoragePath = ".";
#endif
	g_app->init(appContext);
	int initFbW = 0, initFbH = 0;
	glfwGetFramebufferSize(platform->window, &initFbW, &initFbH);
	if (initFbW <= 0 || initFbH <= 0) {
		initFbW = appContext.platform->getScreenWidth();
		initFbH = appContext.platform->getScreenHeight();
	}
	g_app->setSize(initFbW, initFbH);

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(loop, 0, 1);
#else
	signal(SIGINT, platformSignalHandler);
	signal(SIGTERM, platformSignalHandler);

	// Main event loop
	while(!glfwWindowShouldClose(platform->window) && !app->wantToQuit()) {
		loop();
	}
#endif

	delete app;

#ifndef STANDALONE_SERVER
	// Exit.
	glfwDestroyWindow(platform->window);
	glfwTerminate();
#endif

	appContext.platform->finish();
	
	delete appContext.platform;

	return 0;
}

#endif /*MAIN_GLFW_H__*/
