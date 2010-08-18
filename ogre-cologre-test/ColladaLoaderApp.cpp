#include "ColladaLoaderApp.h"

#include "GridBuilder.h"
#include <sstream>
#include <cassert>


//-----------------------------------------------------------------------------
void ColladaLoaderApp::createScene()
{
    createGrid(500);

    CColladaDatabase db;
    int ret = 0;

    //
    //if(ret = db.load("../../../media/indochine.DAE"))
    //exit(-1);
    ret = db.load("../../media/indochine.DAE");
    //ret = db.load("../../media/cube.dae");
    //assert(ret != 0);              

    db.convertResources();
    _dumpResourceList();

    db.convertScene(mSceneMgr);

}
//-----------------------------------------------------------------------------
void ColladaLoaderApp::createGrid(int _units){
    GridBuilder::buildInScene(mSceneMgr, _units);
}
//-----------------------------------------------------------------------------
void ColladaLoaderApp::_dumpResourceList()
{
    Ogre::Log *log = Ogre::LogManager::getSingletonPtr()->createLog("resources.log");

    log->logMessage("--------------- MATERIALS");
    _dumpMaterialList(log);
    log->logMessage("--------------- TEXTURES");
    _dumpTextureList(log);
    log->logMessage("--------------- MODELS");
    _dumpMeshList(log);
}
//-----------------------------------------------------------------------------
void ColladaLoaderApp::_dumpMaterialList(Ogre::Log *_log)
{
    Ogre::MaterialManager *allMaterials = Ogre::MaterialManager::getSingletonPtr();
    Ogre::MaterialManager::ResourceMapIterator it = allMaterials->getResourceIterator();

    while(it.hasMoreElements())
    {
        Ogre::MaterialPtr currentMaterial = it.getNext();

        std::stringstream s;
        s << "Material [name=" << currentMaterial->getName() << " num_tech=" << currentMaterial->getNumTechniques();
        s << " group=" << currentMaterial->getGroup() << "]";            
        _log->logMessage(s.str());
    }

};
//-----------------------------------------------------------------------------
void ColladaLoaderApp::_dumpTextureList(Ogre::Log *_log)
{
    Ogre::TextureManager *allTextures = Ogre::TextureManager::getSingletonPtr();
    Ogre::TextureManager::ResourceMapIterator it = allTextures->getResourceIterator();

    while(it.hasMoreElements())
    {
        Ogre::TexturePtr currentTex = it.getNext();

        std::string name = currentTex->getName();
        int w, h;
        w = currentTex->getWidth();
        h = currentTex->getHandle();

        std::stringstream s;
        s << "Texture [name=" << name << " size=" << w << "x" << h ;
        s << " group=" << currentTex->getGroup(); 
        s <<  "]";
                                
        _log->logMessage(s.str());
    }
};
//-----------------------------------------------------------------------------
void ColladaLoaderApp::_dumpMeshList(Ogre::Log* _log)
{
    Ogre::MeshManager *allMeshes = Ogre::MeshManager::getSingletonPtr();
    Ogre::MeshManager::ResourceMapIterator it = allMeshes->getResourceIterator();

    while(it.hasMoreElements())
    {
        Ogre::MeshPtr currentMesh = it.getNext();

        std::stringstream s;
        s << "Mesh [name=" << currentMesh->getName() << " n_submeshes=" <<  currentMesh->getNumSubMeshes();
        s << " group=" << currentMesh->getGroup() <<  "]";                          
        _log->logMessage(s.str());
                                
        _dumpSharedData(_log, currentMesh);
        _dumpSubmeshList(_log, currentMesh);                                            
    }
};
//-----------------------------------------------------------------------------
void ColladaLoaderApp::_dumpSharedData(Ogre::Log *_log, Ogre::MeshPtr _mesh)
{
    _log->logMessage("  Shared Data : "+Ogre::StringConverter::toString(_mesh->sharedVertexData->vertexCount)+" vertices");


}
//-----------------------------------------------------------------------------
void ColladaLoaderApp::_dumpSubmeshList(Ogre::Log *_log, Ogre::MeshPtr _mesh)
{
    _log->logMessage("  Submeshes : ");

    Ogre::Mesh::SubMeshIterator it = _mesh->getSubMeshIterator();
    int i = 0;
    while(it.hasMoreElements())
    {
        Ogre::SubMesh *currentSubmesh = it.getNext();
        if(!currentSubmesh->useSharedVertices)
        {
            std::stringstream s;
            s << "    [" << i << "]";
            s << " n_vert=" << currentSubmesh->vertexData->vertexCount;
            s << " n_idx=" << currentSubmesh->indexData->indexCount;
            _log->logMessage(s.str());
        }

    }
    _log->logMessage(" ");
}
//-----------------------------------------------------------------------------

