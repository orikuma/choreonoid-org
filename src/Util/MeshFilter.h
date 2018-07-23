/*!
  @file
  @author Shin'ichiro Nakaoka
*/

#ifndef CNOID_UTIL_MESH_FILTER_H
#define CNOID_UTIL_MESH_FILTER_H

#include "exportdecl.h"

namespace cnoid {

class SgNode;
class SgMesh;
class MeshFilterImpl;

class CNOID_EXPORT MeshFilter
{
public:
    MeshFilter();
    MeshFilter(const MeshFilter& org);
    ~MeshFilter();

    void integrateMeshes(SgMesh* mesh1, SgMesh* mesh2);

    void removeRedundantVertices(SgNode* scene);
    void removeRedundantVertices(SgMesh* mesh);

    /**
       The following functions can only be applied to the meshes that do not have redundant vertices
    */
    void removeRedundantFaces(SgNode* scene);
    void removeRedundantFaces(SgMesh* mesh);
    
    void removeRedundantNormals(SgNode* scene);
    void removeRedundantNormals(SgMesh* mesh);

    bool generateNormals(SgMesh* mesh, float creaseAngle = 3.14159f, bool removeRedundantVertices = false);
    void setNormalOverwritingEnabled(bool on);
    void setMinCreaseAngle(float angle);
    void setMaxCreaseAngle(float angle);
    
    // Deprecated. Use enableNormalOverwriting()
    void setOverwritingEnabled(bool on);

private:
    MeshFilterImpl* impl;
};

// for the backward compatibility
typedef MeshFilter MeshNormalGenerator;

}

#endif