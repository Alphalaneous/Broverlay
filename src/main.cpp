#include <Geode/Geode.hpp>
#include <Geode/modify/CCScene.hpp>
#include <Geode/utils/VMTHookManager.hpp>
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

class $modify(MyCCScene, CCScene) {
    bool init() {
        if (!CCScene::init()) return false;
        if (!typeinfo_cast<CCTransitionScene*>(this)) {
            (void) VMTHookManager::get().addHook<ResolveC<MyCCScene>::func(&MyCCScene::getChildren)>(this, "cocos2d::CCScene::getChildren");
            (void) VMTHookManager::get().addHook<ResolveC<MyCCScene>::func(&MyCCScene::getChildrenCount)>(this, "cocos2d::CCScene::getChildrenCount");
            (void) VMTHookManager::get().addHook<ResolveC<MyCCScene>::func(&MyCCScene::onEnter)>(this, "cocos2d::CCScene::onEnter");
        }
        return true;
    }

    CCArray* getChildren() {
        // this can return nullptr :(
        auto children = CCNode::getChildren();
        // I don't wanna actually add them to the children array
        if (children) children = children->shallowCopy();
        else children = CCArray::create();
        for (auto persisted : SceneManager::get()->getPersistedNodes()) {
            children->addObject(persisted);
        }
        return children;
    }

    unsigned int getChildrenCount(void) const {
        return CCNode::getChildrenCount() + SceneManager::get()->getPersistedNodes().size();
    }

    void onEnter() {
        CCNode::onEnter();
        Broverlay::get()->onEnter();
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
