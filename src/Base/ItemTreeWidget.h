#ifndef CNOID_BASE_ITEM_TREE_WIDGET_H
#define CNOID_BASE_ITEM_TREE_WIDGET_H

#include <QWidget>
#include "exportdecl.h"

namespace cnoid {

class Item;
class RootItem;
class Archive;

class CNOID_EXPORT ItemTreeWidget : public QWidget
{
public:
    ItemTreeWidget(RootItem* rootItem);
    ~ItemTreeWidget();

    void addTopLevelItem(Item* item);
    void setFontSizeZoom(int zoom);

    bool storeState(Archive& archive);
    bool restoreState(const Archive& archive);

    class Impl;

private:
    Impl* impl;
};

}

#endif