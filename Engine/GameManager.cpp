#include "GameManager.h"

#include "../JSON Serializer/JsonSerializer.h"
#include "../Logger/StaticLogger.h"

#include <iostream>
#include <sstream>

GameManager::constructor GameManager::cons;
Platform GameManager::platform;
GameWindow* GameManager::mainWindow = nullptr;
std::thread* GameManager::updateLoop = nullptr;
std::string GameManager::resFolder = "";
GameManager::GameTime GameManager::time;

GameManager::constructor::constructor() {
    initializePlatform();
}

GameManager::constructor::~constructor() {
    // Destroy the updateloop thread.
    if(updateLoop != nullptr) {
        updateLoop->join();
        delete updateLoop;
    }

    delete mainWindow;
}

void GameManager::cleanupResources() {
}

void GameManager::createWindow(const WindowConfig& windowConfig) {
    // Init glfw and create window.
    if(!glfwInit()) {
        StaticLogger::instance.critical("Could not initialize [^'window]");
    }
    else {
        bool fullScreen = windowConfig.fullscreen;
        int flags = 0;
        flags |= (fullScreen)? (int)WindowCreateFlags::WINDOW_FULL_SCREEN : 0;

        mainWindow = new GameWindow(windowConfig.width, windowConfig.height, windowConfig.xPos, windowConfig.yPos, windowConfig.centered, windowConfig.gameName, flags);
        GLenum err = glewInit();

        if(err != 0) {
            StaticLogger::instance.critical("Could not initialize [^'OpenGL]: {string}", glewGetErrorString(err));
        }
        else {
            StaticLogger::instance.trace("Initialized [^'graphics instance] on [^'main thread]");
        }
    }
}

void GameManager::createWindow(const std::string& settingsPath) {
    WindowConfig windowConf(800, 600);
    JsonFile file(settingsPath);

    if(!file.isLoadSuccessful()) {
        StaticLogger::instance.warning("Settings file provided but failed to load");
        createWindow(windowConf);
        return;
    }

    JsonObject* head = file.getHead()->objectValue;

    StaticLogger::instance.trace("Loading settings from {string}", settingsPath.c_str());

    //check for window preferences
    JsonValue* windowSettings = head->lookupNode("window");
    JsonValue* resPath = head->lookupNode("respath");

    //load required window settings
    if(windowSettings == nullptr || windowSettings->type != JsonValueType::Object) {
        StaticLogger::instance.warning("Settings file does not contain valid window data");
    }
    else {
        //height, width, title, are required
        JsonValue* height, *width, *posx, *posy, *title, *fullscreen, *vSync, *center;
        height = windowSettings->objectValue->lookupNode("height");
        width = windowSettings->objectValue->lookupNode("width");
        posx = windowSettings->objectValue->lookupNode("posx");
        posy = windowSettings->objectValue->lookupNode("posy");
        title = windowSettings->objectValue->lookupNode("title");
        fullscreen = windowSettings->objectValue->lookupNode("fullscreen");
        center = windowSettings->objectValue->lookupNode("center");
        vSync = windowSettings->objectValue->lookupNode("vsync");

        if(height == nullptr || height->type != JsonValueType::Number) {
            StaticLogger::instance.warning("height attribute must be of type Number");
        }
        else {
            windowConf.height = height->numberValue;
        }

        if(width == nullptr || width->type != JsonValueType::Number) {
            StaticLogger::instance.warning("width attribute must be of type Number");
        }
        else {
            windowConf.width = width->numberValue;
        }

        if(title == nullptr || title->type != JsonValueType::String) {
            StaticLogger::instance.warning("title attribute must be of type string");
        }
        else {
            windowConf.gameName = title->stringValue;
        }

        if(posx != nullptr) {
            if(posx->type == JsonValueType::Number) {
                windowConf.xPos = posx->numberValue;
            }
            else {
                StaticLogger::instance.warning("posx attribute provided, but is not of type number");
            }
        }

        if(posy != nullptr) {
            if(posy->type == JsonValueType::Number) {
                windowConf.yPos = posy->numberValue;
            }
            else {
                StaticLogger::instance.warning("posy attribute provided, but is not of type number");
            }
        }

        if(fullscreen != nullptr) {
            if(fullscreen->type == JsonValueType::Boolean) {
                windowConf.fullscreen = fullscreen->booleanValue;
            }
            else {
                StaticLogger::instance.warning("fullscreen attribute provided, but is not of type boolean");
            }
        }

        if(center != nullptr) {
            if(center->type == JsonValueType::Boolean) {
                //center the window
                windowConf.centered = center->booleanValue;
            }
            else {
                StaticLogger::instance.warning("center attribute provided, but is not of type boolean");
            }
        }

        if(vSync != nullptr) {
            if(vSync->type == JsonValueType::Boolean) {
                //center the window
                windowConf.vSync = vSync->booleanValue;
            }
            else {
                StaticLogger::instance.warning("vsync attribute provided, but is not of type boolean");
            }
        }
    }

    // Load the resources path.
    if(resPath != nullptr) {
        if(resPath->type == JsonValueType::String) {
            setResPath(resPath->stringValue);
        }
        else {
            StaticLogger::instance.warning("respath attribute provided, but is not of type string");
        }
    }

    createWindow(windowConf);
}

void GameManager::initializePlatform() {
    // Load the platform information.
    #ifdef OS_LINUX
    platform.os = OperatingSystem::LINUX;
    #endif

    #ifdef OS_WINDOWS
    GameManager::platform.os = OperatingSystem::WINDOWS;
    #endif

    #ifdef OS_MAC
    GameManager::platform.os = OperatingSystem::MAC;
    #endif
}
#include "Keyboard.h"

void GameManager::executeGameLoop() {
    glfwSwapInterval(1);
    glEnable(GL_DEPTH_TEST);

    init();

    // Right after init, start the gametime.
    time.start();

    while(!mainWindow->isClosing()) {
        render();
        mainWindow->swapBuffers();
        mainWindow->pollEvents();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Add a frame and output the FPS if applicable.
        if(time.addFrame(1000000000)) {
            StaticLogger::instance.trace("Frames per second: {d}", time.getFPS());
        }
    }

    StaticLogger::instance.trace("Closing window");
}

void GameManager::load() {
}

void GameManager::init() {
}

void GameManager::update(){
}

void GameManager::render() {
}

//return program runtime given requested timeunit
float GameManager::getProgramRuntime(TimeUnit unit) {
    switch(unit) {
        case TimeUnit::HOURS:
            return time.getRuntimeHours();
        break;
        case TimeUnit::MINUTES:
            return time.getRuntimeMinutes();
        break;
        case TimeUnit::SECONDS:
            return time.getRuntimeSeconds();
        break;
        case TimeUnit::MILLISECONDS:
            return time.getRuntimeMillis();
        break;
        case TimeUnit::MICROSECONDS:
            return time.getRuntimeMicros();
        break;
        case TimeUnit::NANOSECONDS:
            return (float)time.getRuntimeNanoseconds();
        break;
    }

    return 0;
}
