#include <Geode/Geode.hpp>
#include <Geode/modify/CCScene.hpp>
#include "Broverlay.hpp"

using namespace geode::prelude;

#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_IOS)
#include <Geode/modify/CCEGLView.hpp>
class $modify(MyCCEGLView, CCEGLView) {
    void swapBuffers() {
        Broverlay::get()->visit();
        CCEGLView::swapBuffers();
    }
};
#else
#include <Geode/modify/CCDirector.hpp>
class $modify(MyCCDirector, CCDirector) {
    void drawScene() {
        CCDirector::drawScene();
        Broverlay::get()->visit();
    }
};
#endif

class FunnyCCScene : public CCScene {
    /*CCArray* getChildren() override {
        // this can return nullptr :(
        auto children = CCScene::getChildren();
        // I don't wanna actually add them to the children array
        if (children) children = children->shallowCopy();
        else children = CCArray::create();
        for (auto persisted : SceneManager::get()->getPersistedNodes()) {
            children->addObject(persisted);
        }
        return children;
    }*/

    unsigned int getChildrenCount(void) const override {
        return CCNode::getChildrenCount();// + SceneManager::get()->getPersistedNodes().size();
    }

    /*void onEnter() override {
        CCScene::onEnter();
        Broverlay::get()->onEnter();
    }*/
};

// this is very chatgpt, hoping it works well
template <typename Base, typename Derived>
void patchVirtuals(Base* obj) {
    static void** patched = nullptr;
    if (patched) {
        *reinterpret_cast<void***>(obj) = patched;
        return;
    }

    Base baseDummy;
    Derived derivedDummy;

    void** baseVTable    = *reinterpret_cast<void***>(&baseDummy);
    void** derivedVTable = *reinterpret_cast<void***>(&derivedDummy);

    size_t numSlots = 0;
    while (numSlots < 128 && baseVTable[numSlots] != derivedVTable[numSlots])
        ++numSlots;

    constexpr size_t header_size = 2;

    void** newTable = new void*[header_size + numSlots];

    std::memcpy(newTable, baseVTable - header_size, sizeof(void*) * header_size);

    std::memcpy(newTable + header_size, baseVTable, sizeof(void*) * numSlots);
    for (size_t i = 0; i < numSlots; ++i) {
        if (baseVTable[i] != derivedVTable[i]) {
            newTable[header_size + i] = derivedVTable[i];
        }
    }

    patched = newTable + header_size;
    *reinterpret_cast<void***>(obj) = patched;
}

class $modify(MyCCScene, CCScene) {
    bool init() {
        if (!CCScene::init()) return false;
        if (!typeinfo_cast<CCTransitionScene*>(this)) {
            patchVirtuals<CCScene, FunnyCCScene>(this);
        }
        return true;
    }
};

void keepAcrossScenes_H(SceneManager* self, cocos2d::CCNode* node) {
    Broverlay::get()->addChild(node);
    self->keepAcrossScenes(node);
}

void forget_H(SceneManager* self, cocos2d::CCNode* node) {
    Broverlay::get()->removeChild(node);
    self->forget(node);
}

$on_mod(Loaded) {
    (void) Mod::get()->hook(
        reinterpret_cast<void*>(addresser::getNonVirtual(&SceneManager::keepAcrossScenes)),
        &keepAcrossScenes_H,
        "SceneManager::keepAcrossScenes"
    );

    (void) Mod::get()->hook(
        reinterpret_cast<void*>(addresser::getNonVirtual(&SceneManager::forget)),
        &forget_H,
        "SceneManager::forget"
    );

    // get rekt nerd, we don't like you
    auto geode = Loader::get()->getInstalledMod("geode.loader");
    for (auto hook : geode->getHooks()) {
        if (hook->getDisplayName() == "AppDelegate::willSwitchToScene") {
            (void)hook->disable();
            break;
        }
    }
}
