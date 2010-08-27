#include "ColladaLoaderApp.h"

#include "GridBuilder.h"
#include <sstream>
#include <cassert>


//-----------------------------------------------------------------------------
void ColladaLoaderApp::createScene()
{
    createGrid(500);

    createLight();

    int ret = 0;

    //
    //if(ret = db.load("../../../media/indochine.DAE"))
    //exit(-1);
    //ret = mColladaDatabase.load("../../media/indochine_tris.DAE");
    //ret = mColladaDatabase.load("../../media/Ducati_Monster.dae");
    //ret = mColladaDatabase.load("../../media/plane.DAE");
    //ret = mColladaDatabase.load("../../media/Dining_Room/diningroomC.dae");
    //ret = mColladaDatabase.load("../../media/plane_box_light.dae");
    //ret = mColladaDatabase.load("../../media/shapes.dae");
    //ret = mColladaDatabase.load("../../media/operalyon_3_tris.dae");
    ret = mColladaDatabase.load("../../media/seymour_tris.dae");
    assert(ret == 0);              

    mColladaDatabase.convertResources();
    _dumpResourceList();

    Ogre::SceneNode *node = mSceneMgr->getRootSceneNode()->createChildSceneNode("collada stuff");
    mColladaDatabase.convertScene(mSceneMgr, node);
    //node->scaleq(10, 10, 10);
}
//-----------------------------------------------------------------------------
void ColladaLoaderApp::createGrid(int _units){
    GridBuilder::buildInScene(mSceneMgr, _units);
}
//-----------------------------------------------------------------------------
void ColladaLoaderApp::createLight()
{
    Ogre::SceneNode *lightNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("light node", Vector3(300, 300, 300));
    Light *light = mSceneMgr->createLight("light");

    light->setSpecularColour(Ogre::ColourValue::White);
    Ogre::BillboardSet *bbset = mSceneMgr->createBillboardSet("light billboard");
    bbset->setMaterialName("Objects/Flare");
    Ogre::Billboard *flare = bbset->createBillboard(Vector3::ZERO);



    lightNode->attachObject(light);                                      
    lightNode->attachObject(bbset);

}
//-----------------------------------------------------------------------------
void ColladaLoaderApp::_dumpResourceList()
{
    Ogre::Log *log = Ogre::LogManager::getSingletonPtr()->createLog("resources.log");

    _dumpMaterialList(log);
    _dumpTextureList(log);
    _dumpMeshList(log);
}
//-----------------------------------------------------------------------------
void ColladaLoaderApp::_dumpMaterialList(Ogre::Log *_log)
{
    _log->logMessage("--------------- MATERIALS");
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
}
//-----------------------------------------------------------------------------
void ColladaLoaderApp::_dumpTextureList(Ogre::Log *_log)
{
    _log->logMessage("--------------- TEXTURES");
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
}
//-----------------------------------------------------------------------------
void ColladaLoaderApp::_dumpMeshList(Ogre::Log* _log)
{
    _log->logMessage("--------------- MODELS");
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
}
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

