#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

class Broverlay : public CCNode {
public:
    static Broverlay* get();
    void onEnter() override;
protected:
    void recursiveTouchFix(CCNode* node);	
};