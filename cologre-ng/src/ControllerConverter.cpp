#include "cologre_ng_precompiled.h"
#include "ControllerConverter.h"
#include "dom/domController.h"

#ifdef _DEBUG
#include <cassert>
#endif

#include "../utility/Utility.h"

namespace cologreng{
//------------------------------------------------------------------------------
CControllerConverter::CControllerConverter(Ogre::Log *_log)
    :CResourceConverter(_log)
{
}
//------------------------------------------------------------------------------
CControllerConverter::~CControllerConverter(void)
{
}
//------------------------------------------------------------------------------
int CControllerConverter::convert(daeDatabase* pDatabase)
{
    unsigned int numElements = pDatabase->getElementCount(NULL, "controller", NULL);
    for(unsigned int i = 0; i < numElements; i++)
    {
        daeElement* pElement = NULL;
        pDatabase->getElement(&pElement, i, NULL, "controller", NULL);
        domController* pCont = (domController*)pElement;

        domSkinRef skinRef = pCont->getSkin();
        Ogre::MeshPtr pOgreMesh = Ogre::MeshManager::getSingleton().getByName(skinRef->getSource().getElement()->getID());
        if(pOgreMesh.isNull())
        {
            logMessage(utility::toString("[Warning] Mesh ", skinRef->getSource().getElement()->getID(), " not found for skinning, skipping..."));
            return 1;
        }

        if(skinRef->getBind_shape_matrix()->hasValue())
        {
            domFloat4x4 bindShapeMat = skinRef->getBind_shape_matrix()->getValue();
            Ogre::Matrix4 ogreBindShapeMat = Ogre::Matrix4(bindShapeMat.get(0), bindShapeMat.get(1), bindShapeMat.get(2), bindShapeMat.get(3),
                bindShapeMat.get(4), bindShapeMat.get(5), bindShapeMat.get(6), bindShapeMat.get(7),
                bindShapeMat.get(8), bindShapeMat.get(9), bindShapeMat.get(10), bindShapeMat.get(11),
                bindShapeMat.get(12), bindShapeMat.get(13), bindShapeMat.get(14), bindShapeMat.get(15));
            if(m_zUp)
            {
                Ogre::Quaternion quat = ogreBindShapeMat.extractQuaternion();
                Ogre::Vector3 vec = ogreBindShapeMat.getTrans();
                quat = flipAxes(&quat);
                vec = flipAxes(&vec);
                ogreBindShapeMat.makeTransform(vec, Ogre::Vector3(1.0, 1.0, 1.0), quat);
            }
            transformVertices(pOgreMesh, &ogreBindShapeMat);
            //!! Ogre doens't update the bouding box with skeletal animation. Have to think of something if it becomes neccessary.
        }
    }
    return 0;
}
//------------------------------------------------------------------------------
void CControllerConverter::transformVertices(Ogre::MeshPtr pOgreMesh, Ogre::Matrix4 *pMatTransform)
{
    int vertex_count = 0;
    int index_count = 0;

    bool added_shared = false;
    size_t current_offset = vertex_count;
    size_t shared_offset = vertex_count;
    size_t next_offset = vertex_count;
    size_t index_offset = index_count;
    size_t prev_vert = vertex_count;
    size_t prev_ind = index_count;

    // Calculate how many vertices and indices we're going to need
    for(int i = 0; i < pOgreMesh->getNumSubMeshes(); i++)
    {
        Ogre::SubMesh* submesh = pOgreMesh->getSubMesh(i);

        // We only need to add the shared vertices once
        if(submesh->useSharedVertices)
        {
            if(!added_shared)
            {
                Ogre::VertexData* vertex_data = pOgreMesh->sharedVertexData;
                vertex_count += static_cast<int>(vertex_data->vertexCount);
                added_shared = true;
            }
        }
        else
        {
            Ogre::VertexData* vertex_data = submesh->vertexData;
            vertex_count += static_cast<int>(vertex_data->vertexCount);
        }

        // Add the indices
        Ogre::IndexData* index_data = submesh->indexData;
        index_count += static_cast<int>(index_data->indexCount);
    }

    added_shared = false;

    // Run through the submeshes again, adding the data into the arrays
    for(int i = 0;i < pOgreMesh->getNumSubMeshes();i++)
    {
        Ogre::SubMesh* submesh = pOgreMesh->getSubMesh(i);

        Ogre::VertexData* vertex_data = submesh->useSharedVertices ? pOgreMesh->sharedVertexData : submesh->vertexData;
        if((!submesh->useSharedVertices)||(submesh->useSharedVertices && !added_shared))
        {
            if(submesh->useSharedVertices)
            {
                added_shared = true;
                shared_offset = current_offset;
            }

            const Ogre::VertexElement* posElem = vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
            Ogre::HardwareVertexBufferSharedPtr vbuf = vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());
            unsigned char* vertex = static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_NORMAL));
            Ogre::Real* pReal;
            for(size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
            {
                posElem->baseVertexPointerToElement(vertex, &pReal);
                Ogre::Vector3 pt;
                pt.x = (*pReal);
                pt.y = (*(pReal + 1));
                pt.z = (*(pReal + 2));

                pt = (*pMatTransform) * pt;

                (*pReal) = pt.x;
                pReal++;
                (*pReal) = pt.y;
                pReal++;
                (*pReal) = pt.z;
                pReal++;
            }

            vbuf->unlock();
        }
    }
}
//------------------------------------------------------------------------------
}// namespace cologreng
