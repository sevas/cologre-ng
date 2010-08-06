#pragma once

#include <Ogre.h>

class GridBuilder
{
public:
    static void buildInScene(Ogre::SceneManager *_scene, int _units);


private:
    GridBuilder(void);
    ~GridBuilder(void);
};
