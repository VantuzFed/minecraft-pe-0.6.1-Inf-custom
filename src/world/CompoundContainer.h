#ifndef NET_MINECRAFT_WORLD__CompoundContainer_H__
#define NET_MINECRAFT_WORLD__CompoundContainer_H__

#include "Container.h"
#include <string>

class CompoundContainer : public Container {
private:
    std::string name;
    Container* c1;
    Container* c2;

public:
    CompoundContainer(const std::string& name, Container* c1, Container* c2)
    : Container(ContainerType::CONTAINER), name(name), c1(c1), c2(c2)
    {
    }

    virtual ~CompoundContainer() {}

    virtual int getContainerSize() const override {
        return (c1 ? c1->getContainerSize() : 0) + (c2 ? c2->getContainerSize() : 0);
    }

    virtual std::string getName() const override {
        return name;
    }

    virtual ItemInstance* getItem(int slot) override {
        if (!c1 || !c2) return NULL;
        if (slot >= c1->getContainerSize())
            return c2->getItem(slot - c1->getContainerSize());
        return c1->getItem(slot);
    }

    virtual void setItem(int slot, ItemInstance* item) override {
        if (!c1 || !c2) return;
        if (slot >= c1->getContainerSize())
            c2->setItem(slot - c1->getContainerSize(), item);
        else
            c1->setItem(slot, item);
    }

    virtual ItemInstance removeItem(int slot, int count) override {
        if (!c1 || !c2) return ItemInstance();
        if (slot >= c1->getContainerSize())
            return c2->removeItem(slot - c1->getContainerSize(), count);
        return c1->removeItem(slot, count);
    }

    virtual int getMaxStackSize() const override {
        return c1 ? c1->getMaxStackSize() : 64;
    }

    virtual bool stillValid(Player* player) override {
        return (c1 ? c1->stillValid(player) : false) && (c2 ? c2->stillValid(player) : false);
    }

    virtual void startOpen() override {
        if (c1) c1->startOpen();
        if (c2) c2->startOpen();
    }

    virtual void stopOpen() override {
        if (c1) c1->stopOpen();
        if (c2) c2->stopOpen();
    }
};

#endif /* NET_MINECRAFT_WORLD__CompoundContainer_H__ */
