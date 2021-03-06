/*!
  @file
  @author Shin'ichiro Nakaoka
*/

#include "SceneGraph.h"
#include "SceneNodeClassRegistry.h"
#include "CloneMap.h"
#include "Exception.h"
#include <unordered_map>
#include <typeindex>
#include <mutex>

using namespace std;
using namespace cnoid;

namespace {

// Id to access the correspondingCloneMap flag
CloneMap::FlagId DisableNonNodeCloning("SgObjectDisableNonNodeCloning");

}


SgUpdate::~SgUpdate()
{

}


SgObject::SgObject()
{

}


SgObject::SgObject(const SgObject& org)
    : name_(org.name_),
      uri_(org.uri_)
{

}


Referenced* SgObject::doClone(CloneMap*) const
{
    return new SgObject(*this);
}


bool SgObject::checkNonNodeCloning(const CloneMap& cloneMap)
{
    return !cloneMap.flag(DisableNonNodeCloning);
}


void SgObject::setNonNodeCloning(CloneMap& cloneMap, bool on)
{
    cloneMap.setFlag(DisableNonNodeCloning, !on);
}


int SgObject::numChildObjects() const
{
    return 0;
}


SgObject* SgObject::childObject(int /* index */)
{
    return nullptr;
}


void SgObject::onUpdated(SgUpdate& update)
{
    update.pushNode(this);
    sigUpdated_(update);
    for(const_parentIter p = parents.begin(); p != parents.end(); ++p){
        (*p)->onUpdated(update);
    }
    update.popNode();
}


void SgObject::addParent(SgObject* parent, SgUpdate* update)
{
    parents.insert(parent);

    if(update){
        update->resetAction(SgUpdate::ADDED);
        update->clearPath();
        update->pushNode(this);
        parent->onUpdated(*update);
    }

    if(parents.size() == 1){
        sigGraphConnection_(true);
    }
}


void SgObject::addParent(SgObject* parent, SgUpdate& update)
{
    addParent(parent, &update);
}


void SgObject::addParent(SgObject* parent, bool doNotify)
{
    if(doNotify){
        SgUpdate update;
        addParent(parent, &update);
    } else {
        addParent(parent, nullptr);
    }
}


void SgObject::removeParent(SgObject* parent)
{
    parents.erase(parent);
    if(parents.empty()){
        sigGraphConnection_(false);
    }
}


int SgNode::findClassId(const std::type_info& nodeType)
{
    return SceneNodeClassRegistry::instance().classId(nodeType);
}


int SgNode::registerNodeType(const std::type_info& nodeType, const std::type_info& superType)
{
    return SceneNodeClassRegistry::instance().registerClassAsTypeInfo(nodeType, superType);
}


SgNode::SgNode()
{
    classId_ = findClassId<SgNode>();
    attributes_ = 0;
    decorationRefCounter = 0;
}


SgNode::SgNode(int classId)
    : classId_(classId)
{
    attributes_ = 0;
    decorationRefCounter = 0;
}


SgNode::SgNode(const SgNode& org)
    : SgObject(org),
      classId_(org.classId_),
      attributes_(org.attributes_)
{
    decorationRefCounter = 0;
}


SgNode::~SgNode()
{

}


Referenced* SgNode::doClone(CloneMap*) const
{
    return new SgNode(*this);
}


const BoundingBox& SgNode::boundingBox() const
{
    static const BoundingBox bbox; // empty one
    return bbox;
}


/**
   \note The current implementation of this function does not seem to return the correct T value
*/
static bool findNodeSub(SgNode* node, const std::string& name, SgNodePath& path, Affine3 T, Affine3& out_T)
{
    path.push_back(node);

    if(auto group = dynamic_cast<SgGroup*>(node)){
        if(auto transform = dynamic_cast<SgTransform*>(group)){
            Affine3 T0;
            transform->getTransform(T0);
            T = T * T0;
        }
        if(node->name() == name){
            out_T = T;
            return true;
        }
        for(auto& child : *group){
            if(findNodeSub(child, name, path, T, out_T)){
                return true;
            }
        }
    } else {
        if(node->name() == name){
            out_T = T;
            return true;
        }
    }
    
    path.pop_back();

    return false;
}


SgNodePath SgNode::findNode(const std::string& name, Affine3& out_T)
{
    SgNodePath path;
    out_T.setIdentity();
    findNodeSub(this, name, path, out_T, out_T);
    return path;
}


SgGroup::SgGroup()
    : SgNode(findClassId<SgGroup>())
{
    setAttribute(GroupAttribute);
    isBboxCacheValid = false;
}


SgGroup::SgGroup(int classId)
    : SgNode(classId)
{
    setAttribute(GroupAttribute);
    isBboxCacheValid = false;
}


SgGroup::SgGroup(const SgGroup& org, CloneMap* cloneMap)
    : SgNode(org)
{
    children.reserve(org.numChildren());

    if(cloneMap){
        // deep copy
        for(auto& child : org){
            addChild(cloneMap->getClone<SgNode>(child), false);
        }
    } else {
        // shallow copy
        /**
           \todo Stop the shallow copy of the child nodes.
           Only the attributes of this node should be copied when the clone map is not used.
        */
        for(auto& child : org){
            addChild(child, false);
        }
    }

    setAttribute(GroupAttribute);
    isBboxCacheValid = true;
    bboxCache = org.bboxCache;
}


SgGroup::~SgGroup()
{
    for(const_iterator p = begin(); p != end(); ++p){
        (*p)->removeParent(this);
    }
}


Referenced* SgGroup::doClone(CloneMap* cloneMap) const
{
    return new SgGroup(*this, cloneMap);
}


int SgGroup::numChildObjects() const
{
    return children.size();
}


SgObject* SgGroup::childObject(int index)
{
    return children[index].get();
}


void SgGroup::onUpdated(SgUpdate& update)
{
    //if(update.action() & SgUpdate::BBOX_UPDATED){
    invalidateBoundingBox();
    SgNode::onUpdated(update);
    //}
}


const BoundingBox& SgGroup::boundingBox() const
{
    if(isBboxCacheValid){
        return bboxCache;
    }
    bboxCache.clear();
    for(const_iterator p = begin(); p != end(); ++p){
        bboxCache.expandBy((*p)->boundingBox());
    }
    isBboxCacheValid = true;

    return bboxCache;
}


bool SgGroup::contains(SgNode* node) const
{
    for(const_iterator p = begin(); p != end(); ++p){
        if((*p) == node){
            return true;
        }
    }
    return false;
}


void SgGroup::addChild(SgNode* node, SgUpdate* update)
{
    if(node){
        children.push_back(node);
        node->addParent(this, update);
    }
}


void SgGroup::addChild(SgNode* node, SgUpdate& update)
{
    addChild(node, &update);
}


void SgGroup::addChild(SgNode* node, bool doNotify)
{
    if(doNotify){
        SgUpdate update;
        addChild(node, &update);
    } else {
        addChild(node, nullptr);
    }
}


bool SgGroup::addChildOnce(SgNode* node, bool doNotify)
{
    if(!contains(node)){
        addChild(node, doNotify);
        return true;
    }
    return false;
}


bool SgGroup::addChildOnce(SgNode* node, SgUpdate& update)
{
    if(!contains(node)){
        addChild(node, update);
        return true;
    }
    return false;
}


void SgGroup::insertChild(int index, SgNode* node, SgUpdate* update)
{
    if(node){
        if(index > static_cast<int>(children.size())){
            index = children.size();
        }
        children.insert(children.begin() + index, node);

        node->addParent(this, update);
    }
}


void SgGroup::insertChild(int index, SgNode* node, bool doNotify)
{
    if(doNotify){
        SgUpdate update;
        insertChild(index, node, &update);
    } else {
        insertChild(index, node, nullptr);
    }
}


void SgGroup::insertChild(int index, SgNode* node, SgUpdate& update)
{
    insertChild(index, node, &update);
}


void SgGroup::setSingleChild(SgNode* node, SgUpdate* update)
{
    int n = numChildren();
    if(n > 0){
        bool found = false;
        for(int i = n - 1; i >= 0; --i){
            if(child(i) == node && !found){
                found = true;
                continue;
            }
            removeChildAt(i, update);
        }
        if(!empty()){
            return;
        }
    }
    addChild(node, update);
}


void SgGroup::setSingleChild(SgNode* node, SgUpdate& update)
{
    setSingleChild(node, &update);
}


void SgGroup::setSingleChild(SgNode* node, bool doNotify)
{
    if(doNotify){
        SgUpdate update;
        setSingleChild(node, &update);
    } else {
        setSingleChild(node, nullptr);
    }
}


SgGroup::iterator SgGroup::removeChild(iterator childIter, SgUpdate* update)
{
    iterator next;
    SgNode* child = *childIter;
    child->removeParent(this);
    
    if(!update){
        next = children.erase(childIter);
    } else {
        SgNodePtr childHolder = child;
        next = children.erase(childIter);
        update->resetAction(SgUpdate::REMOVED);
        update->clearPath();
        update->pushNode(child);
        onUpdated(*update);
    }
    return next;
}


bool SgGroup::removeChild(SgNode* node, SgUpdate* update)
{
    bool removed = false;
    if(node){
        iterator p = children.begin();
        while(p != children.end()){
            if((*p) == node){
                p = removeChild(p, update);
                removed = true;
            } else {
                ++p;
            }
        }
    }
    return removed;
}


bool SgGroup::removeChild(SgNode* node, SgUpdate& update)
{
    return removeChild(node, &update);
}


bool SgGroup::removeChild(SgNode* node, bool doNotify)
{
    if(doNotify){
        SgUpdate update;
        return removeChild(node, update);
    } else {
        return removeChild(node, nullptr);
    }
}


void SgGroup::removeChildAt(int index, SgUpdate& update)
{
    removeChild(children.begin() + index, &update);
}


void SgGroup::removeChildAt(int index, bool doNotify)
{
    if(doNotify){
        SgUpdate update;
        removeChildAt(index, update);
    } else {
        removeChild(children.begin() + index, nullptr);
    }
}


void SgGroup::clearChildren(SgUpdate* update)
{
    iterator p = children.begin();
    while(p != children.end()){
        p = removeChild(p, update);
    }
}


void SgGroup::clearChildren(SgUpdate& update)
{
    clearChildren(&update);
}


void SgGroup::clearChildren(bool doNotify)
{
    if(doNotify){
        SgUpdate update;
        clearChildren(&update);
    } else {
        clearChildren(nullptr);
    }
}


void SgGroup::copyChildrenTo(SgGroup* group, SgUpdate& update)
{
    for(size_t i=0; i < children.size(); ++i){
        group->addChild(child(i), update);
    }
}


void SgGroup::copyChildrenTo(SgGroup* group, bool doNotify)
{
    for(size_t i=0; i < children.size(); ++i){
        group->addChild(child(i), doNotify);
    }
}


void SgGroup::moveChildrenTo(SgGroup* group, SgUpdate* update)
{
    const int destTop = group->children.size();
    
    for(size_t i=0; i < children.size(); ++i){
        group->addChild(child(i));
    }
    clearChildren(update);
    
    if(update){
        update->resetAction(SgUpdate::ADDED);
        update->clearPath();
        for(int i=destTop; i < group->numChildren(); ++i){
            group->child(i)->notifyUpdate(*update);
        }
    }
}


void SgGroup::moveChildrenTo(SgGroup* group, SgUpdate& update)
{
    moveChildrenTo(group, &update);
}


void SgGroup::moveChildrenTo(SgGroup* group, bool doNotify)
{
    if(doNotify){
        SgUpdate update;
        moveChildrenTo(group, &update);
    } else {
        moveChildrenTo(group, nullptr);
    }
}


SgGroup* SgGroup::nextChainedGroup()
{
    SgGroup* nextGroup = nullptr;
    if(children.size() == 1){
        nextGroup = dynamic_cast<SgGroup*>(children.front().get());
    }
    return nextGroup;
}


void SgGroup::insertChainedGroup(SgGroup* group)
{
    moveChildrenTo(group);
    addChild(group);
}


void SgGroup::removeChainedGroup(SgGroup* group)
{
    SgGroup* parent = this;
    auto next = nextChainedGroup();
    while(next){
        if(next == group){
            parent->removeChild(next);
            next->moveChildrenTo(parent);
            break;
        }
        next = next->nextChainedGroup();
    }
}


void SgGroup::throwTypeMismatchError()
{
    throw type_mismatch_error();
}


SgInvariantGroup::SgInvariantGroup()
    : SgGroup(findClassId<SgInvariantGroup>())
{

}


SgInvariantGroup::SgInvariantGroup(const SgInvariantGroup& org, CloneMap* cloneMap)
    : SgGroup(org, cloneMap)
{

}


Referenced* SgInvariantGroup::doClone(CloneMap* cloneMap) const
{
    return new SgInvariantGroup(*this, cloneMap);
}


SgTransform::SgTransform(int classId)
    : SgGroup(classId)
{

}


SgTransform::SgTransform(const SgTransform& org, CloneMap* cloneMap)
    : SgGroup(org, cloneMap)
{
    untransformedBboxCache = org.untransformedBboxCache;
}


const BoundingBox& SgTransform::untransformedBoundingBox() const
{
    if(!isBboxCacheValid){
        boundingBox();
    }
    return untransformedBboxCache;
}


SgPosTransform::SgPosTransform(int classId)
    : SgTransform(classId),
      T_(Affine3::Identity())
{

}


SgPosTransform::SgPosTransform()
    : SgPosTransform(findClassId<SgPosTransform>())
{

}


SgPosTransform::SgPosTransform(const Affine3& T)
    : SgTransform(findClassId<SgPosTransform>()),
      T_(T)
{

}


SgPosTransform::SgPosTransform(const SgPosTransform& org, CloneMap* cloneMap)
    : SgTransform(org, cloneMap),
      T_(org.T_)
{

}


Referenced* SgPosTransform::doClone(CloneMap* cloneMap) const
{
    return new SgPosTransform(*this, cloneMap);
}


const BoundingBox& SgPosTransform::boundingBox() const
{
    if(isBboxCacheValid){
        return bboxCache;
    }
    bboxCache.clear();
    for(const_iterator p = begin(); p != end(); ++p){
        bboxCache.expandBy((*p)->boundingBox());
    }
    untransformedBboxCache = bboxCache;
    bboxCache.transform(T_);
    isBboxCacheValid = true;
    return bboxCache;
}


void SgPosTransform::getTransform(Affine3& out_T) const
{
    out_T = T_;
}


SgScaleTransform::SgScaleTransform(int classId)
    : SgTransform(classId)
{
    scale_.setOnes();
}


SgScaleTransform::SgScaleTransform()
    : SgScaleTransform(findClassId<SgScaleTransform>())
{

}


SgScaleTransform::SgScaleTransform(double scale)
    : SgTransform(findClassId<SgScaleTransform>()),
      scale_(scale, scale, scale)
{

}


SgScaleTransform::SgScaleTransform(const Vector3& scale)
    : SgTransform(findClassId<SgScaleTransform>()),
      scale_(scale)
{

}


SgScaleTransform::SgScaleTransform(const SgScaleTransform& org, CloneMap* cloneMap)
    : SgTransform(org, cloneMap),
      scale_(org.scale_)
{

}


Referenced* SgScaleTransform::doClone(CloneMap* cloneMap) const
{
    return new SgScaleTransform(*this, cloneMap);
}
        

const BoundingBox& SgScaleTransform::boundingBox() const
{
    if(isBboxCacheValid){
        return bboxCache;
    }
    bboxCache.clear();
    for(const_iterator p = begin(); p != end(); ++p){
        bboxCache.expandBy((*p)->boundingBox());
    }
    untransformedBboxCache = bboxCache;
    bboxCache.transform(Affine3(scale_.asDiagonal()));
    isBboxCacheValid = true;
    return bboxCache;
}


void SgScaleTransform::getTransform(Affine3& out_T) const
{
    out_T = scale_.asDiagonal();
}


SgAffineTransform::SgAffineTransform(int classId)
    : SgTransform(classId),
      T_(Affine3::Identity())
{

}


SgAffineTransform::SgAffineTransform()
    : SgAffineTransform(findClassId<SgAffineTransform>())
{

}


SgAffineTransform::SgAffineTransform(const Affine3& T)
    : SgTransform(findClassId<SgAffineTransform>()),
      T_(T)
{

}


SgAffineTransform::SgAffineTransform(const SgAffineTransform& org, CloneMap* cloneMap)
    : SgTransform(org, cloneMap),
      T_(org.T_)
{

}


Referenced* SgAffineTransform::doClone(CloneMap* cloneMap) const
{
    return new SgAffineTransform(*this, cloneMap);
}


const BoundingBox& SgAffineTransform::boundingBox() const
{
    if(isBboxCacheValid){
        return bboxCache;
    }
    bboxCache.clear();
    for(const_iterator p = begin(); p != end(); ++p){
        bboxCache.expandBy((*p)->boundingBox());
    }
    untransformedBboxCache = bboxCache;
    bboxCache.transform(T_);
    isBboxCacheValid = true;
    return bboxCache;
}


void SgAffineTransform::getTransform(Affine3& out_T) const
{
    out_T = T_;
}


SgFixedPixelSizeGroup::SgFixedPixelSizeGroup()
    : SgFixedPixelSizeGroup(1.0)
{

}


SgFixedPixelSizeGroup::SgFixedPixelSizeGroup(double pixelSizeRatio)
    : SgGroup(findClassId<SgFixedPixelSizeGroup>())
{
    pixelSizeRatio_ = pixelSizeRatio;
}
      

SgFixedPixelSizeGroup::SgFixedPixelSizeGroup(int classId)
    : SgGroup(classId)
{
    pixelSizeRatio_ = 1.0;
}


SgFixedPixelSizeGroup::SgFixedPixelSizeGroup(const SgFixedPixelSizeGroup& org, CloneMap* cloneMap)
    : SgGroup(org, cloneMap)
{
    pixelSizeRatio_ = org.pixelSizeRatio_;
}


Referenced* SgFixedPixelSizeGroup::doClone(CloneMap* cloneMap) const
{
    return new SgFixedPixelSizeGroup(*this, cloneMap);
}


SgSwitch::SgSwitch(bool on)
{
    isTurnedOn_ = on;
}


SgSwitch::SgSwitch(const SgSwitch& org)
    : SgObject(org)
{
    isTurnedOn_ = org.isTurnedOn_;
}


Referenced* SgSwitch::doClone(CloneMap* cloneMap) const
{
    return new SgSwitch(*this);
}


void SgSwitch::setTurnedOn(bool on, bool doNotify)
{
    if(on != isTurnedOn_){
        isTurnedOn_ = on;
        if(doNotify){
            notifyUpdate();
        }
    }
}    


SgSwitchableGroup::SgSwitchableGroup()
    : SgGroup(findClassId<SgSwitchableGroup>())
{
    isTurnedOn_ = true;
}


SgSwitchableGroup::SgSwitchableGroup(SgSwitch* switchObject)
    : SgSwitchableGroup()
{
    setSwitch(switchObject);
}


SgSwitchableGroup::SgSwitchableGroup(const SgSwitchableGroup& org, CloneMap* cloneMap)
    : SgGroup(org, cloneMap)
{
    if(org.switchObject){
        if(cloneMap){
            switchObject = cloneMap->getClone<SgSwitch>(org.switchObject);
        } else {
            switchObject = org.switchObject;
        }
    } 
    isTurnedOn_ = org.isTurnedOn_;
}


SgSwitchableGroup::~SgSwitchableGroup()
{
    if(switchObject){
        switchObject->removeParent(this);
    }
}


void SgSwitchableGroup::setSwitch(SgSwitch* newSwitchObject)
{
    if(switchObject){
        switchObject->removeParent(this);
    }
    switchObject = newSwitchObject;
    if(newSwitchObject){
        newSwitchObject->addParent(this);
    }
}
        
        
Referenced* SgSwitchableGroup::doClone(CloneMap* cloneMap) const
{
    return new SgSwitchableGroup(*this, cloneMap);
}


void SgSwitchableGroup::setTurnedOn(bool on, bool doNotify)
{
    if(switchObject){
        switchObject->setTurnedOn(on, doNotify);

    } else if(on != isTurnedOn_){
        isTurnedOn_ = on;
        if(doNotify){
            notifyUpdate();
        }
    }
}


SgUnpickableGroup::SgUnpickableGroup()
    : SgGroup(findClassId<SgUnpickableGroup>())
{

}


SgUnpickableGroup::SgUnpickableGroup(const SgUnpickableGroup& org, CloneMap* cloneMap)
    : SgGroup(org, cloneMap)
{

}


Referenced* SgUnpickableGroup::doClone(CloneMap* cloneMap) const
{
    return new SgUnpickableGroup(*this, cloneMap);
}


SgPreprocessed::SgPreprocessed(int classId)
    : SgNode(classId)
{

}


SgPreprocessed::SgPreprocessed(const SgPreprocessed& org)
    : SgNode(org)
{

}


namespace {

struct NodeClassRegistration {
    NodeClassRegistration() {
        SceneNodeClassRegistry::instance()
            .registerClass<SgGroup, SgNode>()
            .registerClass<SgInvariantGroup, SgGroup>()
            .registerClass<SgTransform, SgGroup>()
            .registerClass<SgAffineTransform, SgTransform>()
            .registerClass<SgPosTransform, SgTransform>()
            .registerClass<SgScaleTransform, SgTransform>()
            .registerClass<SgFixedPixelSizeGroup, SgGroup>()
            .registerClass<SgSwitchableGroup, SgGroup>()
            .registerClass<SgUnpickableGroup, SgGroup>()
            .registerClass<SgPreprocessed, SgNode>();
    }
} registration;

}
