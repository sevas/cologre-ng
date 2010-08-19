#ifndef _SCENE_CONVERTER_H_
#define _SCENE_CONVERTER_H_

#include "Converter.h"
#include <dae.h>
#include <dom.h>
#include <dom/domVisual_scene.h>

#include <Ogre.h>
#include "HasLog.h"

namespace cologreng{
///Class for converting collada scenes into ogre scenegraphs. 
class CSceneConverter : public CConverter, protected HasLog
{
public:
  ///Constructor
  CSceneConverter(Ogre::Log *_log);
  ///Destructor
  virtual ~CSceneConverter(void);

  /**Reads collada "<scenes>s" and creates an Ogre scene graph accordingly. It parses all "<node>s" of a collada scene
    and tries to creates ogre scene graph nodes for each. If a skeleton reference is found within a node, it 
    creates ogre-skeletons of the corresponding nodes of type JOINT and assignes the skeleton to the mesh skin.
    For all objects, a scene node only is created when the corresponding mesh already exists in the ogre MeshManager resource pool. 
    /remarks This convert function needs to be called separately from all other converters and will not be called automatically
    if you call CColladaDatabase::convertResources(), but can be called using CColladaDatabase::convertScene() (see
    CColladaDatabase class description for details). This has two reasons, the first being that for the creation of ogre
    scene nodes, a scene manager is needed, whereas all meshes, materials and so on can be created with only an existing
    ogre root object. The second reason is gives the user the choice to ignore the collada scene definition at all
    (so just calling CColladaDatabase::convertResource()) and build his/her own scene graph using the converted
    resources.
    @param pElement daeElement of the collada DAE database to be converted, must be of type "<scene>"
    @param pOgreSceneManager Pointer to a pre-created, valid Ogre::SceneManager object.
    @return 0 if succeeded, 1 upon error, prints error message to stderr
  */
  int convert(daeElement* pElement, Ogre::SceneManager* pOgreSceneManager);

protected:
  /** builds an ogre skeleton based on the following rules :
      -the collada node pNode passed in as the first parameter must be of type JOINT 
      -if pParentBone == NULL, a parent bone will be created in the first recursion step
      -it will recusively parse all subnodes of pNode and add them to pParentNode as children
      -it will NOT YET apply any transfomations to the bones and NOT attach a mesh to the skeleton
      @param pNode Collada Dom node pointer, must be of type JOINT
      @param pParentBone ogre bone to attach all created bones to as children, one will be created if NULL
      @param pOgreSkeleton ogre skeleton pointer through which all bones will be created and therefore become part of this skeleton
  */
  void buildSkeleton(domNode* pNode, Ogre::Bone* pParentBone, Ogre::SkeletonPtr pOgreSkeleton);
  ///transforms a skeleton recursively
  void transformSkeleton(domNode* pNode, Ogre::SkeletonInstance* pOgreSkeletonInstance);
  /** takes a collada node array as starting input, loops through all elements of this array and calls itself recursively
      for all the childnodes of each node. Creates an ogre scene node for each collada node and attached them to
      pParentNode in the right hirachy
      @param pNodeArray Collada DOM node array
      @param pParentNode ogre node to attach all created node to as children
      @param pOgreSceneManager needs to be passed in to create ogre entities
  */
  void buildSceneHierarchy(domNode_Array* pNodeArray, Ogre::SceneNode* pParentNode, Ogre::SceneManager* pOgreSceneManager);
  ///transforms an Ogre scene node using Collada DOM translate-, rotate- and scale-arrays or the transformation matrix
  void transformNode(domNode* pDomNode, Ogre::Node* pOgreNode);

  ///in here because lights need a scene manager to be created and there is no resource manager for lights
  /// called automatically whenever a light is found in the scene, adds lights to the scene graph
  Ogre::Light* convertLight(domLight* pLight, Ogre::SceneManager* pOgreSceneManager);

  /// \internal stores whether there is at least one light in the scene, if not, a default light will be added depending on CConverter::m_convOptions
  bool m_hasLight;
};

} // namespace cologreng

#endif //_SCENE_CONVERTER_H_