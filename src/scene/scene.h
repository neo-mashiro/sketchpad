#pragma once

#define PATH std::string(__FILE__)
#define CWD PATH.substr(0, PATH.rfind("\\")) + "\\"

namespace scene {

    class Scene {
      public:
        Scene() {}
        virtual ~Scene() {}

        virtual void Init(void);
        virtual void OnSceneRender(void);
        virtual void OnImGuiRender(void);
    };

}
