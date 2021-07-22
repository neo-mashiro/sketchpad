#pragma once

#include <string>
#include <memory>

#define PATH std::string(__FILE__)
#define CWD PATH.substr(0, PATH.rfind("\\")) + "\\"
#define SRC PATH.substr(0, PATH.rfind("\\src")) + "\\src\\"
#define RES PATH.substr(0, PATH.rfind("\\src")) + "\\res\\"

namespace scene {

    class Scene {
      public:
        std::string title;

        explicit Scene(const std::string& title) : title(title) {}
        virtual ~Scene() {}

        // these virtual functions should be overridden by each derived class
        // here in the base class, they are used to render the welcome screen
        virtual void Init(void);
        virtual void OnSceneRender(void);
        virtual void OnImGuiRender(void);
    };

}
