/**
   @author Shin'ichiro Nakaoka
*/

#ifndef CNOID_BASE_SCENE_WIDGET_EDITABLE_H
#define CNOID_BASE_SCENE_WIDGET_EDITABLE_H

#include "SceneWidgetEvent.h"
#include "exportdecl.h"

namespace cnoid {

class SceneWidgetEvent;
class MenuManager;
    
class CNOID_EXPORT SceneWidgetEditable
{
public:
    virtual void onSceneModeChanged(const SceneWidgetEvent& event);
    virtual bool onButtonPressEvent(const SceneWidgetEvent& event);
    virtual bool onButtonReleaseEvent(const SceneWidgetEvent& event);
    virtual bool onDoubleClickEvent(const SceneWidgetEvent& event);
    virtual bool onPointerMoveEvent(const SceneWidgetEvent& event);
    virtual void onPointerLeaveEvent(const SceneWidgetEvent& event);
    virtual bool onScrollEvent(const SceneWidgetEvent& event);
    virtual bool onKeyPressEvent(const SceneWidgetEvent& event);
    virtual bool onKeyReleaseEvent(const SceneWidgetEvent& event);
    virtual void onFocusChanged(const SceneWidgetEvent& event, bool on);
    virtual bool onContextMenuRequest(const SceneWidgetEvent& event, MenuManager& menu);
    virtual bool onUndoRequest();
    virtual bool onRedoRequest();
};

}

#endif
