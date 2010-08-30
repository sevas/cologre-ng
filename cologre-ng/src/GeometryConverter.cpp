#include "cologre_ng_precompiled.h"
#include "GeometryConverter.h"

#ifdef _DEBUG
#include <cassert>
#endif
#include <sstream>

#include <dom/domController.h>
#include "../Utility/Utility.h"

namespace cologreng{
//------------------------------------------------------------------------------
CGeometryConverter::CGeometryConverter(Ogre::Log *_log) 
    :CResourceConverter(_log)
{
    m_vertexCount = 0;
    m_dominantElementIndex = 0;
}
//------------------------------------------------------------------------------
CGeometryConverter::~CGeometryConverter()
{
}
//------------------------------------------------------------------------------
int CGeometryConverter::convert(daeDatabase* pDatabase)
{
    logMessage("Converting geometry");
    logMessage("-------------------------------------------------") ;

    unsigned int numElements = pDatabase->getElementCount(NULL, "geometry", NULL);

    logMessage(cologreng::utility::toString("Loading ", numElements, " meshes"));

    for(unsigned int i = 0; i < numElements; i++)
    {
        daeElement* pElement = NULL;
        pDatabase->getElement(&pElement, i, NULL, "geometry", NULL);
        domGeometry* pGeo = (domGeometry*)pElement;
        CIntermediateMesh IM;
        IM.m_meshName = pGeo->getID();

        //see if this mesh is a skin
        unsigned int numControllerElements = pDatabase->getElementCount(NULL, "controller", NULL);
        for(unsigned int i = 0; i < numControllerElements; i++)
        {
            pDatabase->getElement(&pElement, i, NULL, "controller", NULL);
            domController* pCont = (domController*)pElement;
            domSkinRef skinRef = pCont->getSkin();
            if(IM.m_meshName == skinRef->getSource().getElement()->getID())
                addVertexWeights(skinRef.cast());
        }
    
        if(loadGeometryToIntermediateMesh(pGeo, &IM) == ALL_OK)
        {
            indent();
            makeOgreMeshFromIntermediateMesh(&IM);
            dedent();
        } 
        else
        {
            logMessage("Error occured while converting to intermediate mesh");
        }

   }
    logMessage("\n\n\n");
    return 0;
}
//------------------------------------------------------------------------------
conversion_errors CGeometryConverter::loadGeometryToIntermediateMesh(domGeometry* pGeo, CIntermediateMesh* pIM)
{
    std::string meshName;

    if(pGeo->getName())
        meshName = pGeo->getName();
    else
        meshName = pGeo->getId();

    logMessage(cologreng::utility::toString("Loading mesh : ", meshName, " to intermediate mesh"));
    HasLog::indent();

    domMeshRef colladaMesh = pGeo->getMesh();

    if(colladaMesh)
    {
        domTriangles_Array triArray = (*colladaMesh).getTriangles_array();
        if(triArray.getCount() == 0)
        {
            logMessage("No triangles found in mesh, other primitives currently not supported!");
            return ERROR_UNSUPPORTED;
        }

        logMessage(cologreng::utility::toString("Loading ", triArray.getCount(), " triangle(s)"));
        
        for(unsigned int i = 0; i < triArray.getCount(); ++i)
        {
            pIM->m_subMeshCount++;
            domTrianglesRef trisRef = triArray.get(i);
            pIM->m_vSubMeshVertexCount.push_back(trisRef->getCount() * 3);
            pIM->m_vSubMeshNames.push_back(trisRef->getMaterial());
            domInputLocalOffset_Array inputArray = trisRef->getInput_array();

            logMessage(cologreng::utility::toString("Loading ", inputArray.getCount(), " vertices(s)"));

            for(unsigned int i = 0; i < inputArray.getCount(); i++)
            {
                domInputLocalOffsetRef inputRef = inputArray.get(i);
                domURIFragmentType source = ((*inputRef).getSource());
                daeElementRef sourceElementRef = source.getElement();
                std::string strSem((*inputRef).getSemantic());

                //VERTICES will always appear first, change sourceElementRef to point to the vertex source
                if((*sourceElementRef).getElementType() == COLLADA_TYPE::VERTICES)
                {
                    domVertices* pVert = (domVertices*)(&(*sourceElementRef));
                    domInputLocal_Array inputArray = pVert->getInput_array();
                    domInputLocalRef inputRef = inputArray.get(i);
                    strSem = (*inputRef).getSemantic();
                    domURIFragmentType source = ((*inputRef).getSource());
                    sourceElementRef = source.getElement();
                }

                if((*sourceElementRef).getElementType() == COLLADA_TYPE::SOURCE)
                {
                    domSource* pSource = (domSource*)(&(*sourceElementRef));
                    domSource::domTechnique_commonRef techCommonRef = pSource->getTechnique_common();
                    domAccessorRef accessorRef = (*techCommonRef).getAccessor();
                    unsigned int stride = (*accessorRef).getStride();

                    domListOfFloats dataList = pSource->getFloat_array()->getValue();
                    domListOfUInts indexList = trisRef->getP()->getValue();
                    if(strSem == "POSITION")
                    {  
                        pIM->m_positions.stride = stride;
                        std::vector<Ogre::VertexBoneAssignment> vVBATemp;
                        for(unsigned int j = i; j < indexList.getCount(); j += inputArray.getCount())
                        {
                            for(unsigned int k = 0; k < stride; ++k)
                                pIM->m_positions.data.push_back(dataList[indexList[j] * stride + k]);
                            if(m_vVBAssignments.size())
                            {
                                vertexWeight vW;
                                vW.boneIndices = m_vVBAssignments[indexList[j]].boneIndices;
                                vW.weights = m_vVBAssignments[indexList[j]].weights;
                                pIM->m_vertexWeights.push_back(vW);
                            }
                        }
                    }
                    else if(strSem == "NORMAL")
                    {  
                        pIM->m_normals.stride = stride;
                        for(unsigned int j = i; j < indexList.getCount(); j += inputArray.getCount())
                        {
                            for(unsigned int k = 0; k < stride; ++k)
                                pIM->m_normals.data.push_back(dataList[indexList[j] * stride + k]);
                        }
                    }
                    else if(strSem == "TEXCOORD")
                    {  
                        pIM->m_texCoords.stride = stride;
                        for(unsigned int j = i; j < indexList.getCount(); j += inputArray.getCount())
                        {
                            for(unsigned int k = 0; k < stride; ++k)
                                pIM->m_texCoords.data.push_back(dataList[indexList[j] * stride + k]);
                        }
                    }
                }
            }
        }
    }

    HasLog::dedent();

    return ALL_OK;
}
//------------------------------------------------------------------------------
void CGeometryConverter::addVertexWeights(domSkin *pSkin)
{ 
    m_vVBAssignments.clear();
    for(unsigned int i = 0; i < pSkin->getVertex_weights()->getInput_array().getCount(); ++i)
    {
        domInputLocalOffsetRef inputRef = pSkin->getVertex_weights()->getInput_array().get(i);
        domURIFragmentType source = inputRef->getSource();
        daeElementRef sourceElementRef = source.getElement();
        std::string strSem = inputRef->getSemantic();
        domName_arrayRef nameArrayRef;
        if(strSem == "WEIGHT")
        {
            domSource* pSource = (domSource*)(&(*sourceElementRef));
            domListOfFloats weightList = pSource->getFloat_array()->getValue();
            domListOfInts vList = pSkin->getVertex_weights()->getV()->getValue();
            domListOfUInts vCountList = pSkin->getVertex_weights()->getVcount()->getValue();
            unsigned int valuesCopied = 0;
            for(unsigned int j = 0; j < vCountList.getCount(); ++j)
            {
                vertexWeight VW;
                for(unsigned int k = 0; k < vCountList.get(j); ++k)
                {
                    VW.boneIndices.push_back(vList.get(valuesCopied + (k * 2)));
                    VW.weights.push_back(weightList.get(vList.get(valuesCopied + (k * 2 + 1))));
                }
                m_vVBAssignments.push_back(VW);
                valuesCopied += vCountList.get(j) * 2;
            }
        }
    }
}
//------------------------------------------------------------------------------
void CGeometryConverter::makeOgreMeshFromIntermediateMesh(CIntermediateMesh *pIM)
{
    logMessage(cologreng::utility::toString(
        "Making Ogre mesh from intermediate mesh : "
        , pIM->m_meshName
        , " [#submeshes : " , pIM->m_subMeshCount
        , " #vertices : " , pIM->m_positions.data.size()
        , " #normals : " , pIM->m_normals.data.size()
        ,  "]"));

    HasLog::indent();
    

    Ogre::MeshPtr pOgreMesh = Ogre::MeshManager::getSingleton().createManual(pIM->m_meshName, "DaeCustom");
    

    pOgreMesh->sharedVertexData = new Ogre::VertexData();
    Ogre::VertexDeclaration* pDecl = pOgreMesh->sharedVertexData->vertexDeclaration;
    _setVertexDataSemanticsFromIntermediateMesh(pDecl, pIM);          

    VertexBufferAndAABB vertexBufferAndAABB;
    vertexBufferAndAABB = _createVertexDataFromIntermediateMesh(pDecl, pIM);

    pOgreMesh->sharedVertexData->vertexBufferBinding->setBinding(0, vertexBufferAndAABB.first);
    pOgreMesh->sharedVertexData->vertexCount = pIM->m_positions.data.size() / pIM->m_positions.stride;
    pOgreMesh->_setBounds(vertexBufferAndAABB.second);

    _createSubMeshIndexes(pIM, pOgreMesh);

    if(pIM->m_vertexWeights.size())
    {
        _reorganizeBuffersForSkeletalAnimation(pOgreMesh, pIM);
    }

    pOgreMesh->load();

    HasLog::dedent();

    return;
}
//------------------------------------------------------------------------------
void CGeometryConverter::_setVertexDataSemanticsFromIntermediateMesh(Ogre::VertexDeclaration *_vertexDecl, CIntermediateMesh *_intermediateMesh)
{
    logMessage("Setting vertex data semantics : ");
    indent();
    if(_intermediateMesh->m_positions.data.size())
    {
        size_t offset = _vertexDecl->getVertexSize(0);
        _vertexDecl->addElement(0, offset, static_cast<Ogre::VertexElementType>(_intermediateMesh->m_positions.stride - 1), Ogre::VES_POSITION);
        logMessage("Has Ogre::VES_POSITION");
    }
    if(_intermediateMesh->m_normals.data.size())
    {
        size_t offset = _vertexDecl->getVertexSize(0);
        _vertexDecl->addElement(0, offset, static_cast<Ogre::VertexElementType>(_intermediateMesh->m_normals.stride - 1), Ogre::VES_NORMAL);
        logMessage("Has Ogre::VES_NORMAL");
    }
    if(_intermediateMesh->m_texCoords.data.size())
    {
        size_t offset = _vertexDecl->getVertexSize(0);
        _vertexDecl->addElement(0, offset, static_cast<Ogre::VertexElementType>(_intermediateMesh->m_texCoords.stride - 1), Ogre::VES_TEXTURE_COORDINATES);
        logMessage("Has Ogre::VES_TEXTURE_COORDINATES");
    }
    dedent();
}
//------------------------------------------------------------------------------
CGeometryConverter::VertexBufferAndAABB CGeometryConverter::_createVertexDataFromIntermediateMesh( 
    Ogre::VertexDeclaration* _vertexDecl, 
    CIntermediateMesh * _intermediateMesh)
{
    Ogre::HardwareVertexBufferSharedPtr vertexBuffer = 
        Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
            _vertexDecl->getVertexSize(0)
            , _intermediateMesh->m_positions.data.size() / _intermediateMesh->m_positions.stride
            , Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    Ogre::AxisAlignedBox aaBB;


    float* pData = static_cast<float*>(vertexBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD));
    //at this point, there should be an equal number of data for each element if devided by stride
    unsigned int vertexCount = _intermediateMesh->m_positions.data.size() / _intermediateMesh->m_positions.stride;
    logMessage(utility::toString("Copying vertexdata (", vertexCount, " vertices)"));
    for(unsigned int i = 0; i < vertexCount; ++i)
    {
        // copy position data
        {
            Ogre::Vector3 pos;
            //assuming for now that position data is always vec3
            pos.x = _intermediateMesh->m_positions.data[i * _intermediateMesh->m_positions.stride];
            pos.y = _intermediateMesh->m_positions.data[i * _intermediateMesh->m_positions.stride + 1];
            pos.z = _intermediateMesh->m_positions.data[i * _intermediateMesh->m_positions.stride + 2];
            if(m_zUp)
                pos = flipAxes(&pos);
            aaBB.merge(pos);

            *pData = pos.x;
            ++pData;
            *pData = pos.y;
            ++pData;
            *pData = pos.z;
            ++pData;
        }

        // copy normal data
        if(_intermediateMesh->m_normals.data.size())
        {
            Ogre::Vector3 normal;            

            normal.x = _intermediateMesh->m_normals.data[i * _intermediateMesh->m_normals.stride];
            normal.y = _intermediateMesh->m_normals.data[i * _intermediateMesh->m_normals.stride + 1];
            normal.z = _intermediateMesh->m_normals.data[i * _intermediateMesh->m_normals.stride + 2];
            if(m_zUp)
                normal = flipAxes(&normal);

            *pData = normal.x;
            ++pData;
            *pData = normal.y;
            ++pData;
            *pData = normal.z;
            ++pData;
        }
        else
        {
            logMessage(utility::toString("No normal data found in mesh", _intermediateMesh->m_meshName));
        }

        // copy texcoords data
        if(_intermediateMesh->m_texCoords.data.size())
        {
            Ogre::Vector3 tc;

            tc.x = _intermediateMesh->m_texCoords.data[i * _intermediateMesh->m_texCoords.stride];
            tc.y = _intermediateMesh->m_texCoords.data[i * _intermediateMesh->m_texCoords.stride + 1];
            if(_intermediateMesh->m_texCoords.stride > 2)
                tc.z = _intermediateMesh->m_texCoords.data[i * _intermediateMesh->m_texCoords.stride + 2];

            if(m_convOptions.flipTextureH == true)
                tc.y = 1.0 - tc.y;
            if(m_convOptions.flipTextureV == true)
                tc.x = 1.0 - tc.x;

            *pData = tc.x;
            ++pData;
            *pData = tc.y;
            ++pData;
            if(_intermediateMesh->m_texCoords.stride > 2)
            {
                *pData = tc.z;
                ++pData;
            }
        }
        else
        {
            logMessage(utility::toString("No texcoord data found in mesh", _intermediateMesh->m_meshName));
        }


    }
    vertexBuffer->unlock();
    return VertexBufferAndAABB(vertexBuffer, aaBB);;
}
//------------------------------------------------------------------------------
void CGeometryConverter::buildVertexDeclFromInputArray(const daeElementRef elemRef, Ogre::MeshPtr pOgreMesh)
{
    pOgreMesh->sharedVertexData = new Ogre::VertexData();
    if((*elemRef).getElementType() == COLLADA_TYPE::TRIANGLES)
    {
        domTriangles* pTris = daeSafeCast<domTriangles>(&(*elemRef));
        domInputLocalOffset_Array inputArray = pTris->getInput_array();

        Ogre::SubMesh* pSubMesh = pOgreMesh->createSubMesh(pTris->getMaterial());
        Ogre::VertexDeclaration* pDecl = Ogre::HardwareBufferManager::getSingleton().createVertexDeclaration();
        pSubMesh->useSharedVertices = true;

        //store data source references for later copying the data to the right spots
        std::vector<domSource*> vTmpSource;

        for(unsigned int i = 0; i < inputArray.getCount(); i++)
        {
            domInputLocalOffsetRef inputRef = inputArray.get(i);
            domURIFragmentType source = ((*inputRef).getSource());
            daeElementRef sourceElementRef = source.getElement();
            std::string strSem((*inputRef).getSemantic());

            //VERTICES will always appear first, if so, change sourceElementRef to point to the vertex source
            if((*sourceElementRef).getElementType() == COLLADA_TYPE::VERTICES)
            {
                domVertices* pVert = (domVertices*)(&(*sourceElementRef));
                domInputLocal_Array inputArray = pVert->getInput_array();
                domInputLocalRef inputRef = inputArray.get(i);
                strSem = (*inputRef).getSemantic();
                domURIFragmentType source = ((*inputRef).getSource());
                sourceElementRef = source.getElement();
            }

            if((*sourceElementRef).getElementType() == COLLADA_TYPE::SOURCE)
            {
                domSource* pSource = (domSource*)(&(*sourceElementRef));
                vTmpSource.push_back(pSource);
                domSource::domTechnique_commonRef techCommonRef = pSource->getTechnique_common();
                domAccessorRef accessorRef = (*techCommonRef).getAccessor();
                unsigned int stride = (*accessorRef).getStride();

                Ogre::VertexElementType type = Ogre::VET_FLOAT2;
                if(stride == 3)
                    type = Ogre::VET_FLOAT3;
                else if(stride == 4)
                    type = Ogre::VET_FLOAT4;

                Ogre::VertexElementSemantic sem;
                if(strSem == "POSITION")
                    sem = Ogre::VES_POSITION;
                else if(strSem == "NORMAL")
                    sem = Ogre::VES_NORMAL;
                else if(strSem == "TEXCOORD")
                    sem = Ogre::VES_TEXTURE_COORDINATES;

                else
                {
                    OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED, utility::toString("Unknown semantic ", strSem), "cologre::CGeometryConverter::buildVertexDeclFromInputArray");
                }

                size_t offset = pDecl->getVertexSize(0);
                pDecl->addElement(0, offset, type, sem);
            }
        }
        //is this a declaration that is already there?
        bool bFound = false;
        for(unsigned int i = 0; i < m_vDeclarations.size(); ++i)
        {
            if(*(m_vDeclarations[i]) == (*pDecl))
            {
                pSubMesh->indexData->indexStart = m_vIVD[i].vertexCount;
                m_vIVD[i].vertexCount += pTris->getCount() * 3;
                pSubMesh->indexData->indexCount = pTris->getCount() * 3;
                copyData(pTris->getP(), vTmpSource, &(m_vIVD[i]), m_vDeclarations[i], &(const_cast<Ogre::AxisAlignedBox&>(pOgreMesh->getBounds())));
                bFound = true;    
                break;
            }
        }
        if(!bFound)
        {
            m_vDeclarations.push_back(pDecl);
            IntermediateVertexData IVD;
            IVD.vertexCount = pTris->getCount() * 3;
            pSubMesh->indexData->indexCount = IVD.vertexCount;
            m_vIVD.push_back(IVD);
            copyData(pTris->getP(), vTmpSource, &(m_vIVD[m_vIVD.size() - 1]), m_vDeclarations[m_vDeclarations.size() - 1], &(const_cast<Ogre::AxisAlignedBox&>(pOgreMesh->getBounds())));
        }
    }
    pOgreMesh->sharedVertexData->vertexDeclaration = m_vDeclarations[0];
}
//------------------------------------------------------------------------------
void CGeometryConverter::copyData(domPRef pRef, const std::vector<domSource*> &vSource, IntermediateVertexData* pDest, Ogre::VertexDeclaration* pVertexDecl, Ogre::AxisAlignedBox* pAABB)
{
    domListOfUInts indexList = (*pRef).getValue();
    //cache some values to save time in the copy loop
    domAccessorRef* dAR = new domAccessorRef[pVertexDecl->getElementCount()];
    for(unsigned int i = 0; i < pVertexDecl->getElementCount(); ++i)
        dAR[i] = vSource[i]->getTechnique_common()->getAccessor();

    for(unsigned int j = 0; j < indexList.getCount(); ++j)
    {
        unsigned int index = indexList[j];
        switch(pVertexDecl->getElement(j % pVertexDecl->getElementCount())->getType())
        {
        case Ogre::VET_FLOAT2:
            {
                Ogre::Vector2 vec2;
                vec2.x = vSource[j % pVertexDecl->getElementCount()]->getFloat_array()->getValue().get(index * 2);
                vec2.y = vSource[j % pVertexDecl->getElementCount()]->getFloat_array()->getValue().get(index * 2 + 1);
                if(pVertexDecl->getElement(j % pVertexDecl->getElementCount())->getSemantic() == Ogre::VES_TEXTURE_COORDINATES)
                {
                    if(m_convOptions.flipTextureH == true)
                        vec2.y = 1.0 - vec2.y;
                    if(m_convOptions.flipTextureV == true)
                        vec2.x = 1.0 - vec2.x;
                }
                pDest->data.push_back(vec2.x);
                pDest->data.push_back(vec2.y);
            }
        case Ogre::VET_FLOAT3:
            {
                Ogre::Vector3 vec3;
                vec3.x = vSource[j % pVertexDecl->getElementCount()]->getFloat_array()->getValue().get(index * 3);
                vec3.y = vSource[j % pVertexDecl->getElementCount()]->getFloat_array()->getValue().get(index * 3 + 1);
                vec3.z = vSource[j % pVertexDecl->getElementCount()]->getFloat_array()->getValue().get(index * 3 + 2);
                if(pVertexDecl->getElement(j % pVertexDecl->getElementCount())->getSemantic() == Ogre::VES_TEXTURE_COORDINATES)
                {
                    if(m_convOptions.flipTextureH == true)
                        vec3.y = 1.0 - vec3.y;
                    else if(m_convOptions.flipTextureV == true)
                        vec3.x = 1.0 - vec3.x;
                }
                else
                {
                    if(m_zUp)
                        vec3 = flipAxes(&vec3);
                    if(pVertexDecl->getElement(j % pVertexDecl->getElementCount())->getSemantic() == Ogre::VES_POSITION)
                        pAABB->merge(vec3);
                }
                pDest->data.push_back(vec3.x);
                pDest->data.push_back(vec3.y);
                pDest->data.push_back(vec3.z);
            }
        }
    }
}
//------------------------------------------------------------------------------
void CGeometryConverter::_createSubMeshIndexes( CIntermediateMesh * pIM, Ogre::MeshPtr pOgreMesh )
{
    logMessage(utility::toString("Creating ", pIM->m_subMeshCount, " submeshes"));
    int totalIndexCount = 0;
    for(unsigned int i = 0; i < pIM->m_subMeshCount; ++i)
    {
        logMessage(utility::toString("Adding submesh ", i, " to ", pOgreMesh->getName(), " : ", pIM->m_vSubMeshNames[i]));

        Ogre::SubMesh* pSubMesh = pOgreMesh->createSubMesh(pIM->m_vSubMeshNames[i]);
        pSubMesh->indexData->indexCount = pIM->m_vSubMeshVertexCount[i];
        pSubMesh->indexData->indexStart = 0;
        pSubMesh->useSharedVertices = true;
        pSubMesh->indexData->indexBuffer = Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT, 
            pSubMesh->indexData->indexCount, 
            Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY); 
        Ogre::uint16* idata = static_cast<Ogre::uint16*>(pSubMesh->indexData->indexBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD)); 
        for(unsigned int j = 0; j < pSubMesh->indexData->indexCount; ++j)
        {
            *idata = j + totalIndexCount;
            ++idata;
        }
        totalIndexCount += pIM->m_vSubMeshVertexCount[i];
        pSubMesh->indexData->indexBuffer->unlock();
    }
}
//------------------------------------------------------------------------------
void CGeometryConverter::_reorganizeBuffersForSkeletalAnimation( Ogre::MeshPtr pOgreMesh, CIntermediateMesh *_intermediateMesh )
{
    logMessage("Reorganizing buffers for skeletal animation");
    Ogre::VertexDeclaration* pOrganizedVD = pOgreMesh->sharedVertexData->vertexDeclaration->getAutoOrganisedDeclaration(true, false);
    int maxSource = pOrganizedVD->getMaxSource();
    Ogre::BufferUsageList bul;
    for(int i = 0; i < maxSource + 1; ++i)
        bul.push_back(Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
    pOgreMesh->sharedVertexData->reorganiseBuffers(pOrganizedVD, bul); 

    for(unsigned int i = 0; i < _intermediateMesh->m_vertexWeights.size(); ++i)
    {
        for(unsigned int j = 0; j < _intermediateMesh->m_vertexWeights[i].boneIndices.size(); ++j)
        {
            Ogre::VertexBoneAssignment VBA;
            VBA.vertexIndex = i;
            VBA.boneIndex = _intermediateMesh->m_vertexWeights[i].boneIndices[j];
            VBA.weight = _intermediateMesh->m_vertexWeights[i].weights[j];
            pOgreMesh->addBoneAssignment(VBA);
        }
    }
}
//------------------------------------------------------------------------------

} // namespace cologreng

