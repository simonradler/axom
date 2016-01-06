/*
 * Copyright (c) 2015, Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 *
 * All rights reserved.
 *
 * This source code cannot be distributed without permission and
 * further review from Lawrence Livermore National Laboratory.
 */

/**
 * \file
 *
 * \brief Subsetting policies for SLAM
 *
 * Subsetting policies encompass the type and availability of a set's parent
 *   A valid subset policy must support the following interface:
 *      [required]
 *      * indirection() : IntType  -- returns the value of the element after indirection
 *      * hasIndirection(): bool -- returns whether there is an indirection buffer
 *      [optional]
 *      * operator(): IntType -- alternate accessor for indirection
 */

#ifndef SLAM_POLICIES_SUBSET_H_
#define SLAM_POLICIES_SUBSET_H_

#include <set>

namespace asctoolkit {
namespace slam {
namespace policies {

  /**
   * \name OrderedSet_Subsetting_Policies
   * \brief A few default policies for the subsetting of an OrderedSet
   */

  /// \{

  struct NoSubset
  {
    static const NullSet s_nullSet;
    typedef const Set ParentSetType;

    NoSubset() {}
    NoSubset(ParentSetType*) {}     // This empty .ctor is here to satisfy SubsettingPolicy API

    /**
     * \brief Checks whether the set containing this policy class is a subset
     */
    bool                  isSubset() const { return false; }
    const ParentSetType*  parentSet() const { return ATK_NULLPTR; }

    template<typename OrderedSetIt>
    bool                  isValid(OrderedSetIt, OrderedSetIt, bool) const { return true; }
  };

  struct VirtualParentSubset
  {
    static NullSet s_nullSet;

    typedef Set ParentSetType;

    VirtualParentSubset(ParentSetType* parSet = &s_nullSet) : m_parentSet(parSet) {}

    /**
     * \brief Checks whether the set containing this policy class is a subset
     */
    bool        isSubset() const { return *m_parentSet != s_nullSet; }
    const Set*  parentSet() const { return m_parentSet; }
    Set*&       parentSet()       { return m_parentSet; }

    template<typename OrderedSetIt>
    bool        isValid(OrderedSetIt beg, OrderedSetIt end, bool verboseOutput = false) const
    {
      // We allow parent sets to be null (i.e. the subset feature is deactivated)
      if( !isSubset() || m_parentSet == ATK_NULLPTR)
        return true;

      // Next, check if child is empty -- null set is a subset of all sets
      bool childIsEmpty = (beg == end);
      if( childIsEmpty )
        return true;

      // Next, since child has at least one element, the parent cannot be empty
      bool bValid = ( m_parentSet->size() > 0);
      SLIC_CHECK_MSG(verboseOutput && !bValid
          , "VirtualParentSubset -- if we are a subset and input set is non-empty, then parent set must be non-empty");

      // At this point, parent and child are both non-null -- ensure that all elts of child are in parent
      std::set<typename Set::ElementType> pSet;
      for(typename Set::PositionType pos = 0; pos < m_parentSet->size(); ++pos)
        pSet.insert( m_parentSet->at(pos));
      for(; beg != end; ++beg)
      {
        if( pSet.find(*beg) == pSet.end())
        {
          // For now, we only warn about the first failure '
          // Note: in the future, we might want to show all problems.
          SLIC_CHECK_MSG(verboseOutput
              , "VirtualParentSubset :: parent set does not contain element " << *beg << " so child cannot be a subset of parent");
          return false;
        }
      }
      return true;
    }

  private:
    Set* m_parentSet;
  };

  template<typename TheParentSetType>
  struct ConcreteParentSubset
  {
    typedef TheParentSetType ParentSetType;

    ConcreteParentSubset(ParentSetType* parSet = ATK_NULLPTR) : m_parentSet(parSet) {}

    /**
     * \brief Checks whether the set containing this policy class is a subset
     */
    bool                  isSubset() const { return m_parentSet != ATK_NULLPTR; }
    const ParentSetType*  parentSet() const { return m_parentSet; }
    ParentSetType*&       parentSet()       { return m_parentSet; }


    template<typename OrderedSetIt>
    bool                  isValid(OrderedSetIt beg, OrderedSetIt end, bool verboseOutput = false) const
    {
      // We allow parent sets to be null (i.e. the subset feature is deactivated)
      if( !isSubset() )
        return true;

      // Next, check if child is empty -- null set is a subset of all sets
      bool childIsEmpty = (beg == end);
      if( childIsEmpty )
        return true;

      // Next, since child has at least one element, the parent cannot be empty
      bool bValid = (m_parentSet->size() > 0);
      SLIC_CHECK_MSG(verboseOutput && !bValid
          , "VirtualParentSubset -- if input set is non-empty, then parent set must be non-empty");

      // At this point, parent and child are both non-null
      std::set<typename Set::ElementType> pSet;
      for(typename ParentSetType::PositionType pos = 0; pos < m_parentSet->size(); ++pos)
        pSet.insert( (*m_parentSet)[pos] );
      for(; beg != end; ++beg)
      {
        if( pSet.find(*beg) == pSet.end())
        {
          // For now, we only warn about the first failure '
          // Note: in the future, we might want to show all problems.
          SLIC_CHECK_MSG(verboseOutput
              , "ConcreteParentSubset :: parent set does not contain element " << *beg << " so child cannot be a subset of parent");
          return false;
        }
      }
      return true;
    }

  private:
    ParentSetType* m_parentSet;
  };


  /// \}

} // end namespace policies
} // end namespace slam
} // end namespace asctoolkit

#endif // SLAM_POLICIES_SUBSET_H_
