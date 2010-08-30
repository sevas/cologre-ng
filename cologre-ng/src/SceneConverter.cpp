#include "cologre_ng_precompiled.h"
#include "SceneConverter.h"
#include "dom/domCollada.h"
#include "dom/domLight.h"
#include "dae/domAny.h"

#include "../utility/Utility.h"

namespace cologreng{

unsigned int CSceneConverter::sGenericNodeID = 0;
//------------------------------------------------------------------------------
CSceneConverter::CSceneConverter(Ogre::Log *_log)
    :HasLog(_log)
    ,mMaterialMode(USE_FILE_MATERIAL)
    ,mShowLights(SHOW_LIGHT_BILLBOARD)
{
    m_hasLight = false;
}
//------------------------------------------------------------------------------
CSceneConverter::~CSceneConverter()
{
}
//------------------------------------------------------------------------------
int CSceneConverter::convert(daeElement* pElement, Ogre::SceneManager* pOgreSceneManager, Ogre::SceneNode *pParentNode, LightBillBoardMode _bbMode, MaterialMode _materialMode)
{
#ifdef _DEBUG
    assert(pOgreSceneManager);
    assert(pParentNode);
#endif

    mShowLights = _bbMode;
    mMaterialMode = _materialMode;

    logMessage("Converting scene");
    logMessage("-------------------------------------------------");

    Ogre::SceneNode* pRootNode = pParentNode;
    domCOLLADA::domScene* pScene = (domCOLLADA::domScene*)pElement;
    domInstanceWithExtraRef instanceRef = pScene->getInstance_visual_scene();
    daeURI uri(instanceRef->getUrl());
    //uri.resolveURI();
    uri.getElement();
    domVisual_scene* pVisualScene = daeSafeCast<domVisual_scene>(&(*uri.getElement()));
    domNode_Array nodeArray = pVisualScene->getNode_array();

    logMessage("Building scenenodes");
    buildSceneHierarchy(&nodeArray, pRootNode, pOgreSceneManager);


    if(!m_hasLight && CConverter::m_convOptions.addDefaultLight == true)
    {
        _createDefaultLight(pOgreSceneManager);
    }
    return 0;
}
//------------------------------------------------------------------------------
void CSceneConverter::buildSkeleton(domNode* pNode, Ogre::Bone* pParentBone, Ogre::SkeletonPtr pOgreSkeleton)
{
    Ogre::Bone* pOgreBone = NULL;
    if(pNode->getType() != NODETYPE_JOINT)
        return;

    pOgreBone = pOgreSkeleton->createBone(pNode->getName());
    pOgreBone->setManuallyControlled(true);

    if(pParentBone != NULL)
        pParentBone->addChild(pOgreBone);

    domNode_Array nodeArray = pNode->getNode_array();
    for(unsigned int i = 0; i < nodeArray.getCount(); ++i)
        buildSkeleton(nodeArray.get(i), pOgreBone, pOgreSkeleton);
}
//------------------------------------------------------------------------------
void CSceneConverter::transformSkeleton(domNode* pNode, Ogre::SkeletonInstance* pOgreSkeletonInstance)
{
    if(pNode->getType() != NODETYPE_JOINT)
        return;
    Ogre::Bone* pBone = pOgreSkeletonInstance->getBone(pNode->getName());
    pBone->setManuallyControlled(true);
    transformNode(pNode, pBone);

    for(unsigned int i = 0; i < pNode->getNode_array().getCount(); ++i)
        transformSkeleton(pNode->getNode_array().get(i), pOgreSkeletonInstance);
}
//------------------------------------------------------------------------------
void CSceneConverter::buildSceneHierarchy(domNode_Array *pNodeArray, Ogre::SceneNode *pParentNode, Ogre::SceneManager* pOgreSceneManager)
{
    for(unsigned int i = 0; i < pNodeArray->getCount(); ++i)
    {
        Ogre::SceneNode* pSceneNode = NULL;
        domNodeRef nodeRef = pNodeArray->get(i);
        if(nodeRef->getType() == NODETYPE_NODE)
        {
            xsID nodeID = nodeRef->getID();
            std::string nodeName;

            if(nodeID)
            {            
                nodeName = nodeID;
            }
            else
            {
                nodeName = _makeNodeID(); 
                logMessage(utility::toString("[Warning] Current node does not have a name, using : ", nodeName, " instead"));
            }

            pSceneNode = pParentNode->createChildSceneNode(nodeName);
            logMessage(utility::toString("Adding scene node : ", nodeName));

            _instantiateGeometry(nodeRef, pSceneNode, pOgreSceneManager);
            //_instantiateSkeletons(nodeRef, pOgreSceneManager, pSceneNode);
            _instantiateLights(nodeRef, pOgreSceneManager, pSceneNode);

            // recursive call
            domNode_Array nodeArray = nodeRef->getNode_array();
            if(nodeArray.getCount() > 0)
            {
                HasLog::indent();
                buildSceneHierarchy(&nodeArray, pSceneNode, pOgreSceneManager);
                HasLog::dedent();
            }
        }
    }
}
//------------------------------------------------------------------------------
void CSceneConverter::transformNode(domNode* pDomNode, Ogre::Node *pOgreNode)
{
    Ogre::Vector3 ogreVecTranslation(0.0, 0.0, 0.0);
    //accumulate all translations into one vector
    for(unsigned int i = 0; i < pDomNode->getTranslate_array().getCount(); ++i)
    {
        domTranslateRef translateRef = pDomNode->getTranslate_array().get(i);
        domFloat3 translateVec = translateRef->getValue();
        if(m_zUp)
            ogreVecTranslation += flipAxes(&(Ogre::Vector3(translateVec.get(0), translateVec.get(1), translateVec.get(2))));
        else
            ogreVecTranslation += Ogre::Vector3(translateVec.get(0), translateVec.get(1), translateVec.get(2));
    }

    Ogre::Quaternion ogreQuatRotation;
    //now accumulate all rotations into one quaternion
    for(unsigned int i = 0; i < pDomNode->getRotate_array().getCount(); ++i)
    {
        domRotateRef rotateRef = pDomNode->getRotate_array().get(i);
        domFloat4 rotateAA = rotateRef->getValue();
        Ogre::Vector3 ogreVecRot(rotateAA.get(0), rotateAA.get(1), rotateAA.get(2));
        if(m_zUp)
            ogreQuatRotation = ogreQuatRotation * flipAxes(&(Ogre::Quaternion(Ogre::Radian(Ogre::Degree(rotateAA.get(3))), ogreVecRot)));
        else
            ogreQuatRotation = ogreQuatRotation * Ogre::Quaternion(Ogre::Radian(Ogre::Degree(rotateAA.get(3))), ogreVecRot);
    }



    // accumulate scaling factors
    Ogre::Vector3 ogreVecScale(1.0f, 1.0f, 1.0f);
    for(unsigned int i=0 ; i<pDomNode->getScale_array().getCount() ; ++i)
    {
        domScaleRef scaleRef = pDomNode->getScale_array().get(i);
        domFloat3 scaleVec = scaleRef->getValue();

        Ogre::Vector3 currentScale(scaleVec.get(0), scaleVec.get(1), scaleVec.get(2));
        ogreVecScale *= currentScale;

    }


    //now check if transforms are in matrix form rather then AA-Rotations and Vector translations 
    for(unsigned int i = 0; i < pDomNode->getMatrix_array().getCount(); ++i)
    {
        domMatrixRef matRef = pDomNode->getMatrix_array().get(i);
        domFloat4x4 mat = matRef->getValue();
        Ogre::Matrix4 matOgre(mat.get(0),  mat.get(1),  mat.get(2),  mat.get(3),
            mat.get(4),  mat.get(5),  mat.get(6),  mat.get(7),
            mat.get(8),  mat.get(9),  mat.get(10), mat.get(11),
            mat.get(12), mat.get(13), mat.get(14), mat.get(15));

        if(m_zUp)
        {
            ogreVecTranslation += flipAxes(&(matOgre.getTrans()));
            ogreQuatRotation = ogreQuatRotation * flipAxes(&(matOgre.extractQuaternion()));
        }
        else
        {
            ogreVecTranslation += matOgre.getTrans();
            ogreQuatRotation = ogreQuatRotation * matOgre.extractQuaternion();
        }
    }

    Ogre::Quaternion quat = pOgreNode->getOrientation();
    pOgreNode->rotate(quat.Inverse() * ogreQuatRotation);
    Ogre::Vector3 vec = pOgreNode->getPosition();
    pOgreNode->translate(ogreVecTranslation - vec);
    pOgreNode->setScale(ogreVecScale);
    pOgreNode->setInitialState();
}
//------------------------------------------------------------------------------
Ogre::Light* CSceneConverter::convertLight(domLight *pLight, Ogre::SceneManager *pOgreSceneManager)
{
    Ogre::Light* pOgreLight = NULL;
    domLight::domTechnique_commonRef techniqueCommonRef = pLight->getTechnique_common();
    if(domLight::domTechnique_common::domPointRef pointRef = techniqueCommonRef->getPoint())
    {
        logMessage(utility::toString("Adding point light : ", pLight->getId()));

        pOgreLight = pOgreSceneManager->createLight(pLight->getId());
        pOgreLight->setType(Ogre::Light::LT_POINT);
        pOgreLight->setVisible(true);
        domTargetableFloat3Ref colorRef = pointRef->getColor();
        pOgreLight->setDiffuseColour(colorRef->getValue().get(0), colorRef->getValue().get(1), colorRef->getValue().get(2));
        pOgreLight->setSpecularColour(colorRef->getValue().get(0), colorRef->getValue().get(1), colorRef->getValue().get(2));



        float attenuationRange = 100.0f;

        domExtra_Array extraArray = pLight->getExtra_array();
        if(extraArray.getCount())
        {
            logMessage("Getting extra parameters");
            for(unsigned int i = 0; i < extraArray.getCount(); ++i)
            {
                domExtraRef extraRef = extraArray.get(i);
                for(unsigned int j = 0; j < extraRef->getTechnique_array().getCount(); ++j)
                {
                    domTechniqueRef techniqueRef = extraRef->getTechnique_array().get(j);
                    if(techniqueRef->getProfile() == std::string("MAX3D"))
                    {
                        logMessage("MAX3D parameters");
                        int attenuationType = 0;

                        daeElementRefArray& contentsArray = techniqueRef->getContents();
                        for(unsigned int k = 0; k < contentsArray.getCount(); ++k)
                        {
                            domAny& pAny = *(domAny*)(contentsArray.get(k).cast());
                            std::string name = pAny.getElementName();
                            if(name == "decay_type")
                                attenuationType = atoi(pAny.getValue());
                            else if(name == "far_attenuation_end")
                                attenuationRange = static_cast<float>(atof(pAny.getValue()));
                        }

                        switch(attenuationType)
                        {
                        case 0:
                            pOgreLight->setAttenuation(attenuationRange, 1.0, 0.0, 0.0);
                            break;
                        case 1:
                            pOgreLight->setAttenuation(attenuationRange, 1.0, 0.0, 0.0);
                            break;
                        default:
                            pOgreLight->setAttenuation(attenuationRange, 1.0, 0.0, 0.0);
                            break;
                        }
                        return pOgreLight;
                    }
                }
            }
        }

        else
        {
            _setLightAttenuation(pointRef, pOgreLight, attenuationRange);
        }
    }

    else if(domLight::domTechnique_common::domDirectionalRef directRef = techniqueCommonRef->getDirectional())
    {
        logMessage(utility::toString("Adding directional light : ", pLight->getId()));

        pOgreLight = pOgreSceneManager->createLight(pLight->getId());
        pOgreLight->setType(Ogre::Light::LT_DIRECTIONAL);
        pOgreLight->setVisible(true);
        domTargetableFloat3Ref colorRef = directRef->getColor();
        pOgreLight->setDiffuseColour(colorRef->getValue().get(0), colorRef->getValue().get(1), colorRef->getValue().get(2));
    }
    return pOgreLight;
}
//------------------------------------------------------------------------------
Ogre::MeshPtr CSceneConverter::_getOgreMesh( domInstance_geometryRef instanceGeometryRef)
{
    daeURI uri(instanceGeometryRef->getUrl());
    uri.getElement();
    domGeometry* pGeometry = daeSafeCast<domGeometry>(&(*uri.getElement()));
    logMessage(utility::toString("Found mesh ID to load : ", pGeometry->getId()));

    Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().getByName(pGeometry->getId());

    if(mesh.isNull())
        logMessage(utility::toString("[Warning] Mesh resource ", pGeometry->getId(), " does not exist"));
    else
        logMessage(utility::toString("Ogre mesh loaded : ", mesh->getName()));

    return mesh;
}
//------------------------------------------------------------------------------
void CSceneConverter::_bindMaterialsToMesh( domBind_materialRef bindMaterialRef, Ogre::MeshPtr ogreMesh )
{
    domBind_material::domTechnique_commonRef techniqueCommonRef = bindMaterialRef->getTechnique_common();
    domInstance_material_Array instanceMaterialArray = techniqueCommonRef->getInstance_material_array();

    for(unsigned int j = 0; j < instanceMaterialArray.getCount(); ++j)
    {
        if(ogreMesh->getSubMeshNameMap().find(instanceMaterialArray.get(j)->getSymbol()) != ogreMesh->getSubMeshNameMap().end())
        {
            Ogre::SubMesh* pSubMesh = ogreMesh->getSubMesh(instanceMaterialArray.get(j)->getSymbol());
            if(pSubMesh)
            {
                if(mMaterialMode == USE_FILE_MATERIAL)
                {
                    daeString materialName = instanceMaterialArray.get(j)->getTarget().getElement()->getID();
                    _setSubmeshMaterial(pSubMesh, materialName, ogreMesh->getName(), j);
                }
                else if(mMaterialMode == USE_GENERIC_MATERIAL)
                {
                    _setSubmeshMaterial(pSubMesh, "Dae/Blue", ogreMesh->getName(), j);
                }
            } 
            else
                logMessage(utility::toString("[Warning] SubMesh ", instanceMaterialArray.get(j)->getSymbol(), " not found in mesh ", ogreMesh->getName()));
        }
    }
}
//------------------------------------------------------------------------------
void CSceneConverter::_setSubmeshMaterial(Ogre::SubMesh *_subMesh, const std::string &_materialName, const std::string &_meshName, unsigned int _submeshIndex)
{
    logMessage(utility::toString("Binding material ", _materialName, " to submesh ",  _meshName, "[", _submeshIndex,"]" ));                    
    _subMesh->setMaterialName(_materialName);                        
}
//------------------------------------------------------------------------------
void CSceneConverter::_instantiateLights( domNodeRef &nodeRef, Ogre::SceneManager* pOgreSceneManager, Ogre::SceneNode* pSceneNode )
{
    //create scene instances from lights
    domInstance_light_Array instanceLightArray = nodeRef->getInstance_light_array();

    if(instanceLightArray.getCount() > 0)
        logMessage("Instantiating lights");
    else
        logMessage("No light instance found");

   for(unsigned int i = 0; i < instanceLightArray.getCount(); ++i)
    {
        domInstance_lightRef instanceLightRef = instanceLightArray.get(i);
        daeURI uri(instanceLightRef->getUrl());
        //uri.resolveURI();
        uri.getElement();
        domLight* pLight = daeSafeCast<domLight>(&(*uri.getElement()));
        Ogre::Light* pOgreLight = convertLight(pLight, pOgreSceneManager);
        if(pOgreLight)
        {
            _addLightToScene(pOgreLight, pSceneNode, pOgreSceneManager);
            transformNode(nodeRef.cast(), pSceneNode);
        }
    }
}
//------------------------------------------------------------------------------
void CSceneConverter::_addLightToScene(Ogre::Light *_light, Ogre::SceneNode *_node, Ogre::SceneManager *_sceneMgr)
{
    m_hasLight = true;
    _node->attachObject(_light);

    if(mShowLights == SHOW_LIGHT_BILLBOARD)
    {
        Ogre::BillboardSet *bbset = _sceneMgr->createBillboardSet(_light->getName() + "flare");
        bbset->setMaterialName("Objects/Flare");
        bbset->createBillboard(Ogre::Vector3::ZERO);
        _node->attachObject(bbset);
    }
}
//------------------------------------------------------------------------------
void CSceneConverter::_instantiateGeometry( domNodeRef &nodeRef, Ogre::SceneNode* pSceneNode, Ogre::SceneManager* pOgreSceneManager)
{
    //create scene instances from geometry
    domInstance_geometry_Array instanceGeometryArray = nodeRef->getInstance_geometry_array();

    if(instanceGeometryArray.getCount() > 0)
        logMessage("Instantiating geometry");
    else
        logMessage("No geometry instance found");

    for(unsigned int i = 0; i < instanceGeometryArray.getCount(); ++i)
    {
        //transform scene node
        transformNode(nodeRef.cast(), pSceneNode);

        //find the right mesh
        domInstance_geometryRef instanceGeometryRef = instanceGeometryArray.get(i);
        Ogre::MeshPtr ogreMesh = _getOgreMesh(instanceGeometryRef);

        if(!ogreMesh.isNull())
        {
            //bind the right material to the mesh
            domBind_materialRef bindMaterialRef = instanceGeometryRef->getBind_material();
            if(bindMaterialRef)
            {
                _bindMaterialsToMesh(bindMaterialRef, ogreMesh);
            }

            //create the scene entity from the mesh and attach it to the transformed scene node
            Ogre::Entity* pEntity = pOgreSceneManager->createEntity(pSceneNode->getName()+"/entity/"+utility::toString(i), ogreMesh->getName());
            if(pEntity)
            {
                pSceneNode->attachObject(pEntity);
#if OGRE_VERSION_MAJOR == 1 &&   OGRE_VERSION_MINOR == 4
                pEntity->setNormaliseNormals(true);
#endif
            }
            else
                logMessage(utility::toString("[ERROR] Error adding entity ", pEntity->getName())); 
        }
    }
    logMessage("");
}
//------------------------------------------------------------------------------
void CSceneConverter::_instantiateSkeletons( domNodeRef nodeRef, Ogre::SceneManager* pOgreSceneManager, Ogre::SceneNode* pSceneNode )
{
    //create scene instances from controllers (bones, etc...)
    domInstance_controller_Array instanceControllerArray = nodeRef->getInstance_controller_array();

    if(instanceControllerArray.getCount() > 0)
        logMessage("Instantiating skeletons");
    else
        logMessage("No skeleton found");

    for(unsigned int i = 0; i < instanceControllerArray.getCount(); ++i)
    {
        //find the right controller
        domInstance_controllerRef instanceControllerRef = instanceControllerArray.get(i);
        daeURI uri(instanceControllerRef->getUrl());
        //uri.resolveURI();
        uri.getElement();
        domController* pController = daeSafeCast<domController>(&(*uri.getElement()));

        //find the skin
        domSkinRef skinRef = pController->getSkin();

        //bind material to the mesh, which includes building the skeleton from the bones
        domBind_materialRef bindMaterialRef = instanceControllerRef->getBind_material();
        domInstance_controller::domSkeletonRef skeletonRef = instanceControllerRef->getSkeleton_array().get(0);
        domNode* pNode = daeSafeCast<domNode>(skeletonRef->getValue().getElement());
        std::string strSkeletonName(pNode->getID());
        if(Ogre::SkeletonManager::getSingleton().getByName(strSkeletonName).isNull())
        {
            Ogre::SkeletonPtr pOgreSkeleton = Ogre::SkeletonManager::getSingleton().create(pNode->getId(), "DaeCustom");
            if(bindMaterialRef)
            {
                //create all the bones, but do not yet transform them, because the mesh is not bound to the skeleton yet
                buildSkeleton(pNode, NULL, pOgreSkeleton);
                //find the mesh
                Ogre::MeshPtr pOgreMesh = Ogre::MeshManager::getSingleton().getByName(skinRef->getSource().getElement()->getID());
                if(!pOgreMesh.isNull())
                {
                    pOgreMesh->_notifySkeleton(pOgreSkeleton);
                    domBind_material::domTechnique_commonRef techniqueCommonRef = bindMaterialRef->getTechnique_common();
                    domInstance_material_Array instanceMaterialArray = techniqueCommonRef->getInstance_material_array();
                    for(unsigned int j = 0; j < instanceMaterialArray.getCount(); ++j)
                    {
                        Ogre::SubMesh* pSubMesh = pOgreMesh->getSubMesh(instanceMaterialArray.get(j)->getSymbol());
                        if(pSubMesh)
                            pSubMesh->setMaterialName(instanceMaterialArray.get(j)->getTarget().getElement()->getID());
                        else
                            logMessage(utility::toString("[Warning] SubMesh ", instanceMaterialArray.get(j)->getSymbol(), " not found in Mesh ", skinRef->getSource().getElement()->getID()));
                    }
                }
                else
                    logMessage(utility::toString("[Warning] Mesh resource ", skinRef->getSource().getElement()->getID(), " not loaded."));
            }

            //now check for the bind pose
            domSkin::domJointsRef jointsRef = skinRef->getJoints();
            if(jointsRef)
            { 
                domInputLocal_Array inputArray = jointsRef->getInput_array();
                for(unsigned int i = 0; i < inputArray.getCount(); ++i)
                {
                    domInputLocalRef inputRef = inputArray.get(i);
                    domURIFragmentType source = inputRef->getSource();
                    daeElementRef sourceElementRef = source.getElement();
                    std::string strSem = inputRef->getSemantic();
                    domName_arrayRef nameArrayRef;
                    if(strSem == "INV_BIND_MATRIX")
                    {
                        //move all bones to their initial position using the inverse bind pose matrices
                        domSource* pSource = (domSource*)(&(*sourceElementRef));
                        domFloat_arrayRef floatArrayRef = pSource->getFloat_array(); 
                        Ogre::Skeleton::BoneIterator iterBones = pOgreSkeleton->getBoneIterator();
                        for(unsigned int i = 0; i < floatArrayRef->getCount() / 16; ++i)
                        {
                            Ogre::Matrix4 matBindPose(floatArrayRef->getValue().get(i * 16 + 0), floatArrayRef->getValue().get(i * 16 + 1), floatArrayRef->getValue().get(i * 16 + 2), floatArrayRef->getValue().get(i * 16 + 3),
                                floatArrayRef->getValue().get(i * 16 + 4), floatArrayRef->getValue().get(i * 16 + 5), floatArrayRef->getValue().get(i * 16 + 6), floatArrayRef->getValue().get(i * 16 + 7),
                                floatArrayRef->getValue().get(i * 16 + 8), floatArrayRef->getValue().get(i * 16 + 9), floatArrayRef->getValue().get(i * 16 + 10), floatArrayRef->getValue().get(i * 16 + 11),
                                floatArrayRef->getValue().get(i * 16 + 12), floatArrayRef->getValue().get(i * 16 + 13), floatArrayRef->getValue().get(i * 16 + 14), floatArrayRef->getValue().get(i * 16 + 15));

                            matBindPose = matBindPose.inverse();
                            Ogre::Quaternion quatDerived = iterBones.peekNext()->_getDerivedOrientation();
                            if(m_zUp)
                            {
                                Ogre::Quaternion quatBindPose = flipAxes(&matBindPose.extractQuaternion());
                                iterBones.peekNext()->rotate(quatDerived.Inverse() * quatBindPose, Ogre::Node::TS_LOCAL);
                                Ogre::Vector3 vecDerived = iterBones.peekNext()->_getDerivedPosition();
                                Ogre::Vector3 vec = flipAxes(&matBindPose.getTrans()) - vecDerived;
                                iterBones.getNext()->translate(vec, Ogre::Node::TS_WORLD);
                            }
                            else
                            {
                                iterBones.peekNext()->rotate(quatDerived.Inverse() * matBindPose.extractQuaternion(), Ogre::Node::TS_WORLD);
                                Ogre::Vector3 vecDerived = iterBones.peekNext()->_getDerivedPosition();
                                iterBones.getNext()->translate((matBindPose[0])[3] - vecDerived.x,
                                    (matBindPose[1])[3] - vecDerived.y,
                                    (matBindPose[2])[3] - vecDerived.z, Ogre::Node::TS_WORLD);
                            }
                        }
                    }
                }
            }
            pOgreSkeleton->setBindingPose();

            //bind the mesh with it's skeleton to the scene node. !!HERE, OGRE CREATES A NEW INSTANCE OF THE SKELETON, SO THAT EACH MESH
            //HAS ITS OWN COPY. AFTER THAT, THE MESH CAN BE TRANSFORMED USING THE BONES
            std::string id(skinRef->getSource().getElement()->getID());
            Ogre::Entity* pEntity = pOgreSceneManager->createEntity(pSceneNode->getName(), id);
            if(pEntity)
            {
                pSceneNode->attachObject(pEntity);      
                pEntity->setNormaliseNormals(true);
                //pEntity->setDisplaySkeleton(true);
            }
            Ogre::SkeletonInstance* pOgreSkeletonInstance = pEntity->getSkeleton();
            //now, transform all the bones to their correct location in the scene
            transformSkeleton(pNode, pOgreSkeletonInstance);
        }
    }
    logMessage("");
}
//------------------------------------------------------------------------------
void CSceneConverter::_createDefaultLight( Ogre::SceneManager* pOgreSceneManager )
{
    Ogre::Light* light = NULL;
    light = pOgreSceneManager->createLight("default_light");
    light->setType(Ogre::Light::LT_POINT);
    light->setVisible(true);
    light->setDiffuseColour(Ogre::ColourValue(1.0f, 1.0f, 1.0f, 1.0f));
    light->setSpecularColour(Ogre::ColourValue(1.0f, 1.0f, 1.0f, 1.0f));

    Ogre::SceneNode *node = pOgreSceneManager->getRootSceneNode()->createChildSceneNode("Generated Default Light", Ogre::Vector3(0, 200, 0));
    _addLightToScene(light, node, pOgreSceneManager);

}
//------------------------------------------------------------------------------
void CSceneConverter::_setLightAttenuation( domLight::domTechnique_common::domPointRef pointRef, Ogre::Light* pOgreLight, float attenuationRange )
{
    if(pointRef->getConstant_attenuation() && 
        pointRef->getLinear_attenuation() && 
        pointRef->getQuadratic_attenuation())
    {
        pOgreLight->setAttenuation(attenuationRange, pointRef->getConstant_attenuation()->getValue(),
            pointRef->getLinear_attenuation()->getValue(),
            pointRef->getQuadratic_attenuation()->getValue());
    }
}
//------------------------------------------------------------------------------
std::string CSceneConverter::_makeNodeID()
{
    return utility::toString("Collada Unnamed Node ", ++sGenericNodeID);
}
//------------------------------------------------------------------------------
} // namespace cologreng