#ifndef _COLLADA_DATABASE_H_
#define _COLLADA_DATABASE_H_

#include <iostream>
#include <set>
#include <string>

#include <Ogre.h>

#include <dae.h>

#include "common.h"

namespace cologreng{
///Base class to load collada files and convert resources and scenes to Ogre
/**
  This class serves as the entry point. In fact, if you just want to convert a whole collada scene into Ogre, this
  is the only class you'll ever need to create an instance of :)
*/
class CColladaDatabase
{

public:
    ///Constructor
    CColladaDatabase();
    ///Destructor
    virtual ~CColladaDatabase();
    ///loads a collada file into the internal database object.
    /** Use this function to load the .DAE file.
      @param filename The path and name of the file to load, path can be either absolut or relative. This needs to be an 
      rfc 2396 compliant relative or absolut path. To load a file from a local harddrive, this looks something like
      "file:///C:/myModels/collada/box.DAE" for an absolut path and "../../myModels/collada.box.DAE" for a relative path.
      @return Returns 0 upon success, 1 if the file could not be found or loaded.
    */
    int load(std::string filename);

    ///converts collada elements for meshes, materials and cotrollers (bone systems) to Ogre resources
    /** This function calls all available converters, apart from the scene converter. All converted
      resources are then available as Ogre resources via the Ogre resource manager, their names being
      those of the corresponding element id in the DAE file. An Ogre root object has to have been created
      BEFORE calling this function.
    */
    void convertResources();

    ///builds an Ogre scene graph from the collada scene element 
    /** This function calls the CSceneConverter::convert function, adds all resources to the passed sceneManagers
      scene graph and applies transformations to the created nodes. Only resources that have been previously converted
      using convertResources() (or the indivdual convert functions) will be used. Resources that appear in the collada scene
      definition but have not been converted to Ogre before calling this function will NOT be automatically converted and therefore
      will NOT be appear in the scene.
      @param pOgreSceneManager Pointer to an ogre scene Manager object that has to have been created before calling this function.
    */
    void convertScene(Ogre::SceneManager* pOgreSceneManager);

protected:
    void _initLogger();  
    void logMessage(const std::string &_msg);         

protected:
    /// the dae object
    DAE* m_pDae;
    ///the database belonging to the Dae object
    daeDatabase* m_pDatabase;
    ///parses the asset tag of the database
    void parseAsset();

    /// filename to use for the log file
    static const std::string s_LogFilename;

    /// Ogre logger
    Ogre::Log *m_pLog;
    /// Keeps track of the already added filesystem locations
    LocationsPtr m_spLocations;

};

}//namespace cologreng

#endif //_COLLADA_DATABASE_H_