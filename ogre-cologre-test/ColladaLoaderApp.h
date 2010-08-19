#pragma once
#ifndef __050810_COLLADALOADERAPP_H__
#define __050810_COLLADALOADERAPP_H__

#pragma once

#include <Ogre.h>
#include "ExampleApplication.h"
#include "cologre.h"           


class ColladaLoaderApp : public ExampleApplication
{
public:

    void createScene();
    void createGrid(int _units);

protected:
    void _dumpResourceList();
    void _dumpMaterialList(Ogre::Log *_log);
    void _dumpTextureList(Ogre::Log *_log);
    void _dumpMeshList(Ogre::Log* _log);
    void _dumpSubmeshList(Ogre::Log *_log, Ogre::MeshPtr);
    void _dumpSharedData(Ogre::Log *_log, Ogre::MeshPtr);

protected:
    cologreng::CColladaDatabase mColladaDatabase;
};
#endif 
