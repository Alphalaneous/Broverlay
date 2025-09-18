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
    CCArray* getChildren() override {
        // this can return nullptr :(
        auto children = CCScene::getChildren();
        // I don't wanna actually add them to the children array
        if (children) children = children->shallowCopy();
        else children = CCArray::create();
        for (auto persisted : SceneManager::get()->getPersistedNodes()) {
            children->addObject(persisted);
        }
        return children;
    }

    unsigned int getChildrenCount(void) const override {
        return CCNode::getChildrenCount() + SceneManager::get()->getPersistedNodes().size();
    }

    void onEnter() override {
        CCScene::onEnter();
        Broverlay::get()->onEnter();
    }
};

// we wanna keep the typeinfo intact
template <typename Base, typename Override>
void** getPatchedVTable() {
    static void** patched = nullptr;
    if (patched) return patched;

    struct NoticesSizeOfVTable : Base {
        virtual void rawrX3NuzzlesYou() {}
    };

    Base based;
    NoticesSizeOfVTable bulgyWulgy;

    void** baseVTable = *reinterpret_cast<void***>(&based);
    void** derivedVTable = *reinterpret_cast<void***>(&bulgyWulgy);

    size_t numSlots = 0;
    while (baseVTable[numSlots] != derivedVTable[numSlots]) ++numSlots;

    Override override;
    void** overrideVTable = *reinterpret_cast<void***>(&override);

    void** table = new void*[numSlots + 2];

    std::memcpy(table, overrideVTable - 2, sizeof(void*) * (numSlots + 2));

    table[0] = (baseVTable - 2)[0];
    table[1] = (baseVTable - 2)[1];

    patched = table + 2;
    return patched;
}

class $modify(MyCCScene, CCScene) {
    bool init() {
        if (!CCScene::init()) return false;
        if (!typeinfo_cast<CCTransitionScene*>(this)) {
            *reinterpret_cast<void**>(this) = getPatchedVTable<CCScene, FunnyCCScene>();
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
