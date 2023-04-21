/*
Minetest
Alexios Avrassoglou <alexiosa@umich.edu>
Ashley David <asda@umich.edu>

*/

#include "guiDroppedShadow.h"

#include "client/client.h"

#include "minetest/src/client/render.h"
#include "IGUIEnvironment.h"
#include "itemdef.h"

using namespace irr;
using namespace gui;

ShadowBlockItem::ShadowBlockItem(gui::IGUIEnvironment *environment,
                                  gui::IGUIElement *parent, s32 id,
                                  core::rect<s32> rectangle,
                                  ISimpleTextureSource *tsrc,
                                  const std::string &item,
                                  Client *client,
                                  bool noclip)
    : GUIElement(environment, parent, id, rectangle)
{
    m_image = new GUIItemImage(environment, this, id,
                               core::rect<s32>(0, 0, rectangle.getWidth(), rectangle.getHeight()),
                               item, getActiveFont(), client);

    m_shadow = new GUIImage(environment, this, core::rect<s32>(5, 5, rectangle.getWidth() + 5, rectangle.getHeight() + 5));
    m_shadow->setImage(tsrc->getTexture("shadow.png"));
    m_shadow->setColor(video::SColor(200, 0, 0, 0)); //sets the shadow to be completely grading 
    m_shadow->setDrawBorder(true);
    m_shadow->setBorderColor(video::SColor(0, 0, 0, 0));
    sendToBack(m_image);
    sendToBack(m_shadow);

    m_client = client;
}

ShadowBlockItem *ShadowBlockItem::addShadowToBlock(IGUIEnvironment *environment,
                                                    const core::rect<s32> &rectangle,
                                                    ISimpleTextureSource *tsrc,
                                                    IGUIElement *parent,
                                                    s32 id,
                                                    const std::string &item,
                                                    Client *client)
{
    ShadowBlockItem *shadowItem = new ShadowBlockItem(environment,
                                                      parent ? parent : environment->getRootGUIElement(),
                                                      id, rectangle, tsrc, item, client);

    shadowItem->drop();
    return shadowItem;
}
