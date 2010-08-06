#include "GeometryConverter.h"
#include "dom/domController.h"

CGeometryConverter::CGeometryConverter() : CResourceConverter()
{
  m_vertexCount = 0;
  m_dominantElementIndex = 0;
}

CGeometryConverter::~CGeometryConverter()
{
}

int CGeometryConverter::convert(daeDatabase* pDatabase)
{
  unsigned int numElements = pDatabase->getElementCount(NULL, "geometry", NULL);
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
      makeOgreMeshFromIntermediateMesh(&IM);
  }
  return 0;
}

conversion_errors CGeometryConverter::loadGeometryToIntermediateMesh(domGeometry* pGeo, CIntermediateMesh* pIM)
{
  domMeshRef colladaMesh = pGeo->getMesh();
  if(colladaMesh)
  {
    domTriangles_Array triArray = (*colladaMesh).getTriangles_array();
    if(triArray.getCount() == 0)
    {
      std::cerr << "No triangles found in mesh, other primitives currently not supported!" << std::endl;
      return ERROR_UNSUPPORTED;
    }
    for(unsigned int i = 0; i < triArray.getCount(); ++i)
    {
      pIM->m_subMeshCount++;
      domTrianglesRef trisRef = triArray.get(i);
      pIM->m_vSubMeshVertexCount.push_back(trisRef->getCount() * 3);
      pIM->m_vSubMeshNames.push_back(trisRef->getMaterial());
      domInputLocalOffset_Array inputArray = trisRef->getInput_array();
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
  return ALL_OK;
}

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

void CGeometryConverter::makeOgreMeshFromIntermediateMesh(CIntermediateMesh *pIM)
{
  Ogre::MeshPtr pOgreMesh = Ogre::MeshManager::getSingleton().createManual(pIM->m_meshName, "DaeCustom");
  pOgreMesh->sharedVertexData = new Ogre::VertexData();
  Ogre::VertexDeclaration* pDecl = pOgreMesh->sharedVertexData->vertexDeclaration;
  Ogre::AxisAlignedBox aaBB;

  if(pIM->m_positions.data.size())
  {
    size_t offset = pDecl->getVertexSize(0);
    pDecl->addElement(0, offset, static_cast<Ogre::VertexElementType>(pIM->m_positions.stride - 1), Ogre::VES_POSITION);
  }
  if(pIM->m_normals.data.size())
  {
    size_t offset = pDecl->getVertexSize(0);
    pDecl->addElement(0, offset, static_cast<Ogre::VertexElementType>(pIM->m_normals.stride - 1), Ogre::VES_NORMAL);
  }
  if(pIM->m_texCoords.data.size())
  {
    size_t offset = pDecl->getVertexSize(0);
    pDecl->addElement(0, offset, static_cast<Ogre::VertexElementType>(pIM->m_texCoords.stride - 1), Ogre::VES_TEXTURE_COORDINATES);
  }
  Ogre::HardwareVertexBufferSharedPtr vertexBuffer = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(pDecl->getVertexSize(0),
                                                                                                                    pIM->m_positions.data.size() / pIM->m_positions.stride,
                                                                                                                    Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
  float* pData = static_cast<float*>(vertexBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD));

  //at this point, there should be an equal number of data for each element if devided by stride
  unsigned int vertexCount = pIM->m_positions.data.size() / pIM->m_positions.stride;
  Ogre::Vector3 vec3;
  for(unsigned int i = 0; i < vertexCount; ++i)
  {
    //assuming for now that position data is always vec3
    vec3.x = pIM->m_positions.data[i * pIM->m_positions.stride];
    vec3.y = pIM->m_positions.data[i * pIM->m_positions.stride + 1];
    vec3.z = pIM->m_positions.data[i * pIM->m_positions.stride + 2];
    if(m_zUp)
      vec3 = flipAxes(&vec3);
    aaBB.merge(vec3);
    
    *pData = vec3.x;
    ++pData;
    *pData = vec3.y;
    ++pData;
    *pData = vec3.z;
    ++pData;

    if(pIM->m_normals.data.size())
    {
      vec3.x = pIM->m_normals.data[i * pIM->m_normals.stride];
      vec3.y = pIM->m_normals.data[i * pIM->m_normals.stride + 1];
      vec3.z = pIM->m_normals.data[i * pIM->m_normals.stride + 2];
      if(m_zUp)
        vec3 = flipAxes(&vec3);
      
      *pData = vec3.x;
      ++pData;
      *pData = vec3.y;
      ++pData;
      *pData = vec3.z;
      ++pData;
    }

    if(pIM->m_texCoords.data.size())
    {
      vec3.x = pIM->m_texCoords.data[i * pIM->m_texCoords.stride];
      vec3.y = pIM->m_texCoords.data[i * pIM->m_texCoords.stride + 1];
      if(pIM->m_texCoords.stride > 2)
        vec3.z = pIM->m_texCoords.data[i * pIM->m_texCoords.stride + 2];
      
      if(m_convOptions.flipTextureH == true)
        vec3.y *= -1.0;
      else if(m_convOptions.flipTextureV == true)
        vec3.x *= -1.0;
      
      *pData = vec3.x;
      ++pData;
      *pData = vec3.y;
      ++pData;
      if(pIM->m_texCoords.stride > 2)
      {
        *pData = vec3.z;
        ++pData;
      }
    }
  }
  vertexBuffer->unlock();
  pOgreMesh->sharedVertexData->vertexBufferBinding->setBinding(0, vertexBuffer);
  pOgreMesh->sharedVertexData->vertexCount = pIM->m_positions.data.size() / pIM->m_positions.stride;
  aaBB.scale(Ogre::Vector3(2.0f, 2.0f, 2.0f));
  pOgreMesh->_setBounds(aaBB);

  int totalIndexCount = 0;
  for(unsigned int i = 0; i < pIM->m_subMeshCount; ++i)
  {
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

  if(pIM->m_vertexWeights.size())
  {
    //reorganize buffers for skeletal animation
    Ogre::VertexDeclaration* pOrganizedVD = pOgreMesh->sharedVertexData->vertexDeclaration->getAutoOrganisedDeclaration(true, false);
    int maxSource = pOrganizedVD->getMaxSource();
    Ogre::BufferUsageList bul;
    for(int i = 0; i < maxSource + 1; ++i)
      bul.push_back(Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
    pOgreMesh->sharedVertexData->reorganiseBuffers(pOrganizedVD, bul); 

    for(unsigned int i = 0; i < pIM->m_vertexWeights.size(); ++i)
    {
      for(unsigned int j = 0; j < pIM->m_vertexWeights[i].boneIndices.size(); ++j)
      {
        Ogre::VertexBoneAssignment VBA;
        VBA.vertexIndex = i;
        VBA.boneIndex = pIM->m_vertexWeights[i].boneIndices[j];
        VBA.weight = pIM->m_vertexWeights[i].weights[j];
        pOgreMesh->addBoneAssignment(VBA);
      }
    }
  }
  pOgreMesh->load();
  return;
}


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
          std::cerr << "Unknown semantic " << strSem << std::endl;
          return;
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
            vec2.y *= -1.0;
          else if(m_convOptions.flipTextureV == true)
            vec2.x *= -1.0;
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
            vec3.y *= -1.0;
          else if(m_convOptions.flipTextureV == true)
            vec3.x *= -1.0;
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
