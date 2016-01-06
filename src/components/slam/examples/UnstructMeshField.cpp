/*
 * Copyright (c) 2015, Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 *
 * All rights reserved.
 *
 * This source code cannot be distributed without permission and further
 * review from Lawrence Livermore National Laboratory.
 */


/**
 * \file
 *
 * \brief Simple user of the mesh api class.
 *
 * \details Loads a hex mesh from a VTK file, generates the Node to Zone relation and does simple mesh processing.
 *
 * \author T. Brunner (original)
 * \author K. Weiss (modified to use the asc toolkit mesh API)
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <set>
#include <vector>
#include <cmath>
#include <cstdlib>

#include "slic/slic.hpp"
#include "slic/UnitTestLogger.hpp"

#include "slam/FileUtilities.hpp"

#include "slam/Set.hpp"
#include "slam/RangeSet.hpp"
#include "slam/StaticVariableRelation.hpp"
#include "slam/StaticConstantRelation.hpp"
#include "slam/DynamicVariableRelation.hpp"

#include "slam/Map.hpp"


#ifndef USE_CONSTANT_RELATION
  #define USE_CONSTANT_RELATION
#endif

#ifndef SLAM_USE_DOUBLE_ARRAY_ACCESS
  #define SLAM_USE_DOUBLE_ARRAY_ACCESS
#endif


namespace slamUnstructuredHex {

  typedef asctoolkit::slam::MeshIndexType IndexType;
  typedef double                          DataType;


  struct Point
  {
    Point(const DataType& x, const DataType& y, const DataType& z) : m_x(x), m_y(y), m_z(z){}
    Point() : m_x(DataType()), m_y(DataType()), m_z(DataType()){}

    DataType                                radius() const { return std::sqrt( m_x * m_x + m_y * m_y + m_z * m_z); }
    Point& operator                         +=(const Point& pt){ m_x += pt.m_x; m_y += pt.m_y; m_z += pt.m_z; return *this; }
    Point& operator                         *=(const DataType& sc){ m_x *= sc; m_y *= sc; m_z *= sc; return *this; }
    template<typename T>    Point& operator /=(const T& sc){ return operator*=( 1. / sc); }

    DataType m_x, m_y, m_z;
  };
  Point operator                            +(const Point& pt1, const Point& pt2)  { Point pt(pt1); pt += pt2;      return pt; }

  Point operator                            *(const Point& pt1, const DataType& sc){ Point pt(pt1); pt *= sc;       return pt; }
  Point operator                            *(const DataType& sc,const Point& pt1) { Point pt(pt1); pt *= sc;       return pt; }

  template<typename T>
  Point operator                            /(const Point& pt1, const T& sc){ Point pt(pt1); pt *= (1. / sc);  return pt; }


  struct HexMesh
  {

  public:
    enum { NODES_PER_ZONE = 8 };

    // types for sets
    typedef asctoolkit::slam::PositionSet                                                               NodeSet;
    typedef asctoolkit::slam::PositionSet                                                               ZoneSet;

    // types for relations
    typedef asctoolkit::slam::StaticVariableRelation                                                    NodeToZoneRelation;
    typedef NodeToZoneRelation::RelationVecConstIterator                                                NodeZoneIterator;

#ifdef USE_CONSTANT_RELATION
    typedef asctoolkit::slam::policies::CompileTimeStrideHolder<ZoneSet::PositionType, NODES_PER_ZONE>  ZNStride;
    typedef asctoolkit::slam::StaticConstantRelation<ZNStride>                                          ZoneToNodeRelation;
#else
    typedef asctoolkit::slam::StaticVariableRelation                                                    ZoneToNodeRelation;
#endif
    typedef ZoneToNodeRelation::RelationVecConstIterator                                                ZoneNodeIterator;

    typedef NodeSet::IndexType                                                                          IndexType;
    typedef NodeSet::PositionType                                                                       PositionType;

    // types for maps
    typedef asctoolkit::slam::Map< Point >                                                              PositionsVec;
    typedef asctoolkit::slam::Map< DataType >                                                           NodeField;
    typedef asctoolkit::slam::Map< DataType >                                                           ZoneField;

  public:
    /** \brief Simple accessor for the number of nodes in the mesh  */
    PositionType  numNodes() const { return nodes.size(); }

    /** \brief Simple accessor for the number of zones in the mesh */
    PositionType  numZones() const { return zones.size(); }

  public:
    /// Sets in the mesh
    NodeSet nodes;
    ZoneSet zones;

    /// Relations in the mesh
    ZoneToNodeRelation relationZoneNode;      // storage for relation_(3,0) -- zones -> nodes
    NodeToZoneRelation relationNodeZone;      // storage for relation_(0,3) -- nodes -> zones


    /// Maps (fields) in the mesh -- positions and scalar fields

    // Node-centered position field
    PositionsVec nodePosition;
    PositionsVec zonePosition;

    // A zone field and two node fields
    ZoneField zoneField;
    NodeField nodeFieldExact;
    NodeField nodeFieldAvg;
  };


  class SimpleVTKHeshMeshReader
  {
  public:
    // uses RAII to open/close the file
    SimpleVTKHeshMeshReader(const std::string & fileName) : vtkMesh( fileName.c_str() )
    {
      // check if the file opened, if not try one directory higher
      if(!vtkMesh)
      {
        std::string pFileName = "../" + fileName;
        vtkMesh.open( pFileName.c_str() );
      }

      SLIC_ERROR_IF( !vtkMesh
          , "fstream error -- problem opening file: '"  << fileName << "' (also tried '../" << fileName << "')"
                                                        << "\nThe current working directory is: '" << asctoolkit::slam::util::getCWD() << "'");
    }
    ~SimpleVTKHeshMeshReader()
    {
      vtkMesh.close();      // Close the file.
    }

    void loadNodesAndPositions(HexMesh* mesh)
    {
      // Read some initial header stuff.  Note: this is not a robust vtkreader
      std::string junk;

      while( junk != "POINTS" )
      {
        vtkMesh >> junk;
      }

      // Read in the POINT data, (that we'll call the nodes)
      IndexType numNodes;
      vtkMesh >> numNodes;
      vtkMesh >> junk;

      std::cout << "\tNumber of nodes = " << numNodes << "\n";

      // Create the set of Nodes
      mesh->nodes = HexMesh::NodeSet(numNodes);

      // Create the nodal position field, and read positions from file
      mesh->nodePosition = HexMesh::PositionsVec( &mesh->nodes );
      Point pos;
      for(IndexType i = 0; i < numNodes; ++i)
      {
        vtkMesh >> pos.m_x >> pos.m_y >> pos.m_z;
        mesh->nodePosition[i] = pos;
      }
    }

    void loadHexToNodesRelation(HexMesh* mesh)
    {
      // Read in the CELL data, that we'll call zones.  We're going to assume hexahedra (VTK type 12)
      std::string junk;
      IndexType numZones;
      IndexType listSize;

      vtkMesh >> junk;
      vtkMesh >> numZones;
      vtkMesh >> listSize;
      IndexType numNodeZoneIndices = listSize - numZones;

      // This is only because we're assuming Hex's.  General meshes can be different.
      SLIC_ASSERT_MSG( numZones * (HexMesh::NODES_PER_ZONE) == numNodeZoneIndices
          ,  "Error in reading mesh!\n" << "  numZones = " << numZones << "\n"
                                        << "  numZones*" << (HexMesh::NODES_PER_ZONE) << " = " << numZones * (HexMesh::NODES_PER_ZONE) << "\n"
                                        << "  numNodeZoneIndices = " << numNodeZoneIndices << "\n" );
      std::cout << "\tNumber of zones = "  << numZones << "\n";

      // Create the set of Zones
      mesh->zones = HexMesh::ZoneSet(numZones);

      // Create the topological incidence relation from zones to nodes
      mesh->relationZoneNode = HexMesh::ZoneToNodeRelation(&mesh->zones, &mesh->nodes);

      // Read in and encode this as a static variable relation
      // TODO: Replace with a static constant relation when that class is developed
      typedef HexMesh::ZoneToNodeRelation::RelationVec  RelationVec;
      typedef RelationVec::iterator                     RelationVecIterator;

    #ifdef USE_CONSTANT_RELATION
      IndexType const STRIDE = HexMesh::NODES_PER_ZONE;
    #else
      // Setup the 'begins' vector  -- exploits the fact that the relation is constant
      RelationVec beginsVec( mesh->zones.size() + 1 );
      for(HexMesh::IndexType idx = 0; idx <= mesh->zones.size(); ++idx)
      {
        beginsVec[idx] = idx * HexMesh::NODES_PER_ZONE;
      }
    #endif

      // Setup the 'offsets' vector into the index space of the nodes set
      RelationVec offsetsVec ( numNodeZoneIndices );
      RelationVecIterator oIt = offsetsVec.begin();
      IndexType nodeCount;
      typedef HexMesh::ZoneSet::iterator SetIterator;
      for(SetIterator zIt = mesh->zones.begin(), zItEnd = mesh->zones.end(); zIt < zItEnd; ++zIt)
      {
        vtkMesh >> nodeCount;

        SLIC_ASSERT_MSG( nodeCount == HexMesh::NODES_PER_ZONE
            , "Unsupported mesh type with zone = " << *zIt << ", nodeCount = " << nodeCount << " (expected " << HexMesh::NODES_PER_ZONE << ")");

        for( IndexType n = 0; n < (HexMesh::NODES_PER_ZONE); ++n )
        {
          vtkMesh >> *oIt++;
        }
      }


      // Set the relation here by copying over the data buffers
    #ifdef USE_CONSTANT_RELATION
      mesh->relationZoneNode. bindRelationData( offsetsVec, STRIDE);
    #else
      mesh->relationZoneNode. bindRelationData( beginsVec,  offsetsVec);
    #endif

      // Check that the relation is valid
      SLIC_ASSERT_MSG(  mesh->relationZoneNode.isValid(), "Error creating (static) relation from zones to nodes!");
      SLIC_ASSERT_MSG(  oIt == offsetsVec.end(),          "Error reading nodes of zones!\n" << offsetsVec.end() - oIt );
    }
  private:
    std::ifstream vtkMesh;
  };

  void readHexMesh(std::string fileName, HexMesh* mesh)
  {
    SimpleVTKHeshMeshReader vtkMeshReader(fileName);

    vtkMeshReader.loadNodesAndPositions(mesh);
    vtkMeshReader.loadHexToNodesRelation(mesh);
  }

  void generateNodeZoneRelation(HexMesh* mesh)
  {
    // Now build the (variable) relation from nodes to zones
    // Strategy: We will first build this as a DynamicVariableRelation, and then linearize it to a StaticVariableRelation
    // In both cases, we are using the relation's range() function to get a pair of iterators to the inner relation

    // --- Step 1: Generate a dynamic variable relation from Nodes to Zones
    typedef asctoolkit::slam::DynamicVariableRelation DynRelation;
    typedef DynRelation::RelationVecConstIteratorPair DynRelationIteratorPair;

    DynRelation tmpZonesOfNode( &mesh->nodes, &mesh->zones );
    IndexType numZonesOfNode = 0;
    for(HexMesh::ZoneSet::iterator zIt = mesh->zones.begin(); zIt < mesh->zones.end(); ++zIt)
    {
      IndexType const& zoneIdx = *zIt;
#ifndef SLAM_USE_DOUBLE_ARRAY_ACCESS
      typedef HexMesh::ZoneToNodeRelation::RelationVecConstIteratorPair RelVecItPair;
      for(RelVecItPair znItPair  = mesh->relationZoneNode.range(zoneIdx); znItPair.first < znItPair.second; ++znItPair.first )
      {
        IndexType const& nodeIdx = *(znItPair.first);
        tmpZonesOfNode.insert( nodeIdx, zoneIdx );
        ++numZonesOfNode;
      }
#else
      HexMesh::ZoneToNodeRelation::RelationSet zNodeSet = mesh->relationZoneNode[zoneIdx];
      const IndexType numZN = mesh->relationZoneNode.size(*zIt);
      for(IndexType idx = 0; idx < numZN; ++idx)
      {
        IndexType nodeIdx = zNodeSet[idx];
        tmpZonesOfNode.insert( nodeIdx, zoneIdx );
        ++numZonesOfNode;
      }
#endif
    }
    SLIC_ASSERT_MSG( tmpZonesOfNode.isValid(), "Error creating (dynamic) relation from nodes to zones!\n");

    // -------------------------------------------------

    // --- Step 2: Convert this to a static variable relation from Nodes to Zones
    mesh->relationNodeZone = HexMesh::NodeToZoneRelation( &mesh->nodes, &mesh->zones);

    // Now, linearize the dynamic relation into a static relation here
    typedef HexMesh::NodeToZoneRelation::RelationVec RelationVec;
    RelationVec beginsVec( mesh->nodes.size() + 1 );
    RelationVec offsetsVec( numZonesOfNode );
    HexMesh::NodeSet::IndexType count = 0;
    for(HexMesh::NodeSet::iterator nIt = mesh->nodes.begin(), nEnd = mesh->nodes.end(); nIt < nEnd; ++nIt)
    {
      beginsVec[*nIt] = count;
      for(DynRelationIteratorPair sItPair = tmpZonesOfNode.range(*nIt); sItPair.first < sItPair.second; ++sItPair.first)
      {
        HexMesh::ZoneSet::IndexType const& zoneIdx = *(sItPair.first);
        offsetsVec[count++] = zoneIdx;
      }
    }
    beginsVec[mesh->nodes.size()] = count;

    // Send the data buffers over to the relation now and check the validity
    mesh->relationNodeZone.bindRelationData(beginsVec, offsetsVec);

    SLIC_ASSERT_MSG(  mesh->relationNodeZone.isValid(), "Error creating (static) relation from nodes to zones!\n");
    SLIC_ASSERT_MSG(  count == numZonesOfNode,          "Error creating zones of Node list!\n");

    std::cout << "\n\tnumZonesOfNode = " << numZonesOfNode << "\n";
  }

  void computeZoneBarycenters(HexMesh* mesh)
  {
    typedef HexMesh::ZoneNodeIterator ZNIterator;

    // Compute the zone positions as the the averages of the positions of the nodes around each zone
    mesh->zonePosition = HexMesh::PositionsVec( &mesh->zones );

    // Outer loop over each zone in the set
    const IndexType numZones = mesh->numZones();
    for(IndexType zIdx = 0; zIdx < numZones; ++zIdx )
    {
      Point zonePos;

      // Inner loop over each node of the zone
      ZNIterator nIt  = mesh->relationZoneNode.begin(zIdx);
      ZNIterator nEnd = mesh->relationZoneNode.end(zIdx);
      for(; nIt < nEnd; ++nIt)
      {
        zonePos += mesh->nodePosition[*nIt];
      }
      zonePos /= mesh->relationZoneNode.size(zIdx);

      mesh->zonePosition[ zIdx ] = zonePos;
    }
  }

  void createZoneRadiusField (HexMesh* mesh)
  {
    // Compute a zone field based on the L2 norm of their position vectors
    mesh->zoneField = HexMesh::ZoneField ( &mesh->zones );

    const IndexType numZones = mesh->numZones();
    for (IndexType zIdx = 0; zIdx < numZones; ++zIdx )
    {
      mesh->zoneField[zIdx] = mesh->zonePosition[zIdx].radius();
    }
  }

  DataType computeNodalErrors(HexMesh* mesh)
  {
    // Compute the node average version, and the maximum relative error
    //typedef HexMesh::NodeSet::iterator                NodeIter;
    typedef HexMesh::NodeToZoneRelation::RelationSet  NZRelSet;
    typedef NZRelSet::const_iterator                  NZIter;

    mesh->nodeFieldAvg   = HexMesh::NodeField(&mesh->nodes);
    mesh->nodeFieldExact = HexMesh::NodeField(&mesh->nodes);
    double errSqSum = 0.0;

    // Outer loop over each node
    const IndexType numNodes = mesh->numNodes();
    for (IndexType nIdx = 0; nIdx < numNodes; ++nIdx)
    {
      // What's the radius?
      const DataType nodalValExact = mesh->nodePosition[nIdx].radius();

      // Inner loop over each zone of the node to find average value
      const NZRelSet & zoneSet = mesh->relationNodeZone[nIdx];
      DataType nodalAvg = DataType();
      for(NZIter zIt = zoneSet.begin(), zItEnd = zoneSet.end(); zIt < zItEnd; ++zIt)
      {
        nodalAvg += mesh->zoneField[ *zIt ];
      }
      nodalAvg /= zoneSet.size();

      // Update fields and compute error
      mesh->nodeFieldAvg[nIdx] = nodalAvg;
      mesh->nodeFieldExact[nIdx] = nodalValExact;

      double const err = nodalAvg - nodalValExact;
      errSqSum += err * err;
    }

    DataType err = std::sqrt( errSqSum / mesh->numNodes() );
    std::cout << "\n\tThe L2-ish error in the node average radius was " << err << std::endl;

    return err;
  }

}   // end namespace slamUnstructuredHex


int main()
{
  using namespace slamUnstructuredHex;

  asctoolkit::slic::UnitTestLogger logger;

#ifndef USE_ONE
  int const NUM_RESOLUTIONS = 4;
#else
  int const NUM_RESOLUTIONS = 1;
#endif

  int fileResolutions[] = {1,2,4,8};
  DataType expectedResults[] = {0.10736689892, 0.037977237476, 0.013251067479, 0.0046357167735};

  for(int res = 0; res < NUM_RESOLUTIONS; ++res)
  {
    std::stringstream filePath;
    filePath  << "../src/components/slam/data/"
              << "ball_" << fileResolutions[res] << ".vtk";
    std::string meshName = filePath.str();

    std::cout << "\n** Loading mesh file '" << meshName << "' and generating zone-> node relation...\n";

    HexMesh hexMesh;
    readHexMesh( meshName, &hexMesh );

    //--------------------------------------------------------------

    // Now build the node to zone relation
    std::cout << "\n** Generating node->zone relation...";
    generateNodeZoneRelation( &hexMesh );

    //--------------------------------------------------------------
    // Now that we have the mesh in memory, we can start to do things with it.
//for(int i=0; i<1000; ++i) {
    std::cout << "\n** Computing zone barycenters using zone->node relation...";
    computeZoneBarycenters(&hexMesh);

    std::cout << "\n** Generating a zone-centered radius field...";
    createZoneRadiusField(&hexMesh);

    std::cout << "\n** Computing node-based errors using node->zone relation...";
    DataType errVal = computeNodalErrors(&hexMesh);

    // Some error checking based on precomputed values
    SLIC_ASSERT_MSG( asctoolkit::utilities::compareReals(errVal, expectedResults[res]), "Error differed from expected value."
        << "\n\texpected: " << expectedResults[res]
        << "\n\tactual: "   << errVal
        << "\n\tdiff: "     << (errVal - expectedResults[res]));
//}
    std::cout << "\ndone." << std::endl;
  }

  //--------------------------------------------------------------
  return 0;
}
